// -----------------------------------------------------------------------------
// This file is part of Silicium
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Written by Michael Wulff Nielsen
// Licensed under the Mozilla Public License v2
//
// See https://mozilla.org/MPL/2.0 for license information
// -----------------------------------------------------------------------------

#include "LuaEngine.h"
#include "ScriptHost.h"
#include "Internals.h"
#include "lua.hpp"
#include <cstring>

namespace vamiga {

// Returns the host the Lua state belongs to
static ScriptHost &
host(lua_State *L)
{
    return **(ScriptHost **)lua_getextraspace(L);
}

/* Calls into the host. A C++ exception must not pass through the Lua
 * interpreter, which is written in C, so it is turned into a Lua error which
 * names the line of the script.
 */
template <class F> static void
call(lua_State *L, F &&f)
{
    bool failed = false;

    try {
        f();
    } catch (std::exception &e) {
        luaL_where(L, 1);
        lua_pushstring(L, e.what());
        lua_concat(L, 2);
        failed = true;
    }
    if (failed) lua_error(L);
}


//
// The emu table
//

// emu.wait_frames([n]) lets the emulator run n frames (default 1). The script
// continues when they have been emulated.
static int
emu_wait_frames(lua_State *L)
{
    auto frames = luaL_optinteger(L, 1, 1);
    luaL_argcheck(L, frames >= 0, 1, "must not be negative");

    call(L, [&] { for (lua_Integer i = 0; i < frames; i++) host(L).runFrame(); });
    return 0;
}

// emu.shell(command) executes a RetroShell command and returns its output
static int
emu_shell(lua_State *L)
{
    auto command = luaL_checkstring(L, 1);

    call(L, [&] {

        auto output = internals::execShellCommand(host(L).getEmulator(), command);
        lua_pushlstring(L, output.data(), output.size());
    });
    return 1;
}

static const luaL_Reg emuFunctions[] = {

    { "wait_frames", emu_wait_frames },
    { "shell", emu_shell },
    { nullptr, nullptr }
};


//
// The cpu table
//

// The registers of the CPU are fields of the cpu table (cpu.d0, cpu.pc, ...).
// They are read from the core when they are accessed.
static int
cpu_index(lua_State *L)
{
    if (lua_type(L, 2) != LUA_TSTRING) return 0;
    auto key = lua_tostring(L, 2);

    const CPUInfo *info = nullptr;
    call(L, [&] { info = &host(L).getEmulator().cpu.getInfo(); });

    if ((key[0] == 'd' || key[0] == 'a') && key[1] >= '0' && key[1] <= '7' && key[2] == 0) {

        auto nr = key[1] - '0';
        lua_pushinteger(L, key[0] == 'd' ? info->d[nr] : info->a[nr]);

    } else if (strcmp(key, "pc") == 0)  { lua_pushinteger(L, info->pc0);
    } else if (strcmp(key, "sr") == 0)  { lua_pushinteger(L, info->sr);
    } else if (strcmp(key, "usp") == 0) { lua_pushinteger(L, info->usp);
    } else if (strcmp(key, "isp") == 0) { lua_pushinteger(L, info->isp);
    } else if (strcmp(key, "msp") == 0) { lua_pushinteger(L, info->msp);
    } else if (strcmp(key, "vbr") == 0) { lua_pushinteger(L, info->vbr);
    } else {
        return 0;
    }
    return 1;
}

static int
cpu_newindex(lua_State *L)
{
    return luaL_error(L, "cpu.%s cannot be written", luaL_tolstring(L, 2, nullptr));
}

static const luaL_Reg cpuMetamethods[] = {

    { "__index", cpu_index },
    { "__newindex", cpu_newindex },
    { nullptr, nullptr }
};


//
// The mem table
//

// The largest block mem.read_range() returns
static constexpr lua_Integer maxRange = 16 * 1024 * 1024;

static u32
checkAddress(lua_State *L, int arg)
{
    auto addr = luaL_checkinteger(L, arg);
    luaL_argcheck(L, addr >= 0 && addr <= 0xFFFFFFFF, arg, "not an address");
    return u32(addr);
}

/* Reads a value of the given size in bytes the way the CPU sees memory, but
 * without side effects (spypeek). Values are composed of single bytes, big
 * endian, so that any address can be read.
 */
static lua_Integer
peek(lua_State *L, u32 addr, isize size)
{
    lua_Integer value = 0;

    call(L, [&] {

        auto &debugger = host(L).getEmulator().mem.debugger;
        for (isize i = 0; i < size; i++) {
            value = value << 8 | debugger.spypeek8(Accessor::CPU, u32(addr + i));
        }
    });
    return value;
}

// mem.peek_u8(addr), mem.peek_u16(addr), mem.peek_u32(addr)
static int mem_peek_u8(lua_State *L)  { lua_pushinteger(L, peek(L, checkAddress(L, 1), 1)); return 1; }
static int mem_peek_u16(lua_State *L) { lua_pushinteger(L, peek(L, checkAddress(L, 1), 2)); return 1; }
static int mem_peek_u32(lua_State *L) { lua_pushinteger(L, peek(L, checkAddress(L, 1), 4)); return 1; }

// mem.read_range(addr, len) returns len bytes as a string
static int
mem_read_range(lua_State *L)
{
    auto addr = checkAddress(L, 1);
    auto len = luaL_checkinteger(L, 2);
    luaL_argcheck(L, len >= 0 && len <= maxRange, 2, "invalid length");

    luaL_Buffer buffer;
    auto data = luaL_buffinitsize(L, &buffer, size_t(len));

    call(L, [&] {

        auto &debugger = host(L).getEmulator().mem.debugger;
        for (lua_Integer i = 0; i < len; i++) {
            data[i] = char(debugger.spypeek8(Accessor::CPU, u32(addr + i)));
        }
    });

    luaL_pushresultsize(&buffer, size_t(len));
    return 1;
}

static const luaL_Reg memFunctions[] = {

    { "peek_u8", mem_peek_u8 },
    { "peek_u16", mem_peek_u16 },
    { "peek_u32", mem_peek_u32 },
    { "read_range", mem_read_range },
    { nullptr, nullptr }
};


//
// The engine
//

LuaEngine::LuaEngine(ScriptHost &host)
{
    L = luaL_newstate();
    if (!L) throw std::bad_alloc();

    // The functions of the API find the host here
    *(ScriptHost **)lua_getextraspace(L) = &host;

    // Scripts get the standard libraries, as in the stand-alone interpreter
    luaL_openlibs(L);

    // And the functions for the Amiga
    luaL_newlib(L, emuFunctions);
    lua_setglobal(L, "emu");

    lua_newtable(L);
    luaL_newlib(L, cpuMetamethods);
    lua_setmetatable(L, -2);
    lua_setglobal(L, "cpu");

    luaL_newlib(L, memFunctions);
    lua_setglobal(L, "mem");
}

LuaEngine::~LuaEngine()
{
    lua_close(L);
}

void
LuaEngine::runFile(const fs::path &path)
{
    // Compile and run the file. The error message names the file and the line.
    if (luaL_loadfile(L, path.string().c_str()) != LUA_OK || lua_pcall(L, 0, 0, 0) != LUA_OK) {

        auto text = lua_tostring(L, -1);
        auto message = std::string(text ? text : "(the error is not a string)");
        lua_pop(L, 1);
        throw ScriptError(message);
    }
}

}
