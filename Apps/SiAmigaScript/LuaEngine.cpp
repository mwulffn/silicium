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

static const luaL_Reg emuFunctions[] = {

    { "wait_frames", emu_wait_frames },
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
