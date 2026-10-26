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
#include "Keys.h"
#include "lua.hpp"
#include "utl/abilities/Hashable.h"
#include <cctype>
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
// The keyboard table
//

// Returns the keycode of a key given by its name or as a raw keycode
static KeyCode
checkKey(lua_State *L, int arg)
{
    if (lua_type(L, arg) == LUA_TNUMBER) {

        auto code = luaL_checkinteger(L, arg);
        luaL_argcheck(L, code >= 0 && code <= 0x7F, arg, "not a keycode");
        return KeyCode(code);
    }

    if (lua_type(L, arg) != LUA_TSTRING) return KeyCode(luaL_typeerror(L, arg, "key name or keycode"));

    auto name = lua_tostring(L, arg);
    if (auto code = keyCode(name)) return *code;
    return KeyCode(luaL_argerror(L, arg, lua_pushfstring(L, "unknown key '%s'", name)));
}

// keyboard.press(key) holds a key down
static int
keyboard_press(lua_State *L)
{
    auto key = checkKey(L, 1);
    call(L, [&] { host(L).getEmulator().keyboard.press(key); });
    return 0;
}

// keyboard.release(key) lets a key go
static int
keyboard_release(lua_State *L)
{
    auto key = checkKey(L, 1);
    call(L, [&] { host(L).getEmulator().keyboard.release(key); });
    return 0;
}

// keyboard.release_all() lets all keys go
static int
keyboard_release_all(lua_State *L)
{
    call(L, [&] { host(L).getEmulator().keyboard.releaseAll(); });
    return 0;
}

// keyboard.pressed(key) tells if a key is down
static int
keyboard_pressed(lua_State *L)
{
    auto key = checkKey(L, 1);
    bool pressed = false;

    call(L, [&] {

        // Key events are queued; wait until the keyboard has seen them
        auto &vamiga = host(L).getEmulator();
        vamiga.sync();
        pressed = vamiga.keyboard.isPressed(key);
    });

    lua_pushboolean(L, pressed);
    return 1;
}

/* keyboard.type(text) types the text on a US keyboard and returns the number
 * of frames this takes. The keys are scheduled in emulated time and pressed
 * while the emulator runs, so a script continues with
 * emu.wait_frames(keyboard.type(text)).
 */
static int
keyboard_type(lua_State *L)
{
    // Each character takes five frames: shift (if needed) goes down, then the
    // key one frame later, which is held for two frames, then shift goes up
    constexpr isize framesPerKey = 5;

    size_t length;
    auto text = luaL_checklstring(L, 1, &length);

    // Check the whole text before anything is typed
    for (size_t i = 0; i < length; i++) {
        if (!typedKey(text[i])) {

            auto c = (unsigned char)text[i];
            auto message = isprint(c) ?
                lua_pushfstring(L, "cannot type '%c'", c) :
                lua_pushfstring(L, "cannot type the character with code %d", c);
            return luaL_argerror(L, 1, message);
        }
    }

    call(L, [&] {

        auto &vamiga = host(L).getEmulator();
        auto frame = vamiga.amiga.getConfig().type == TV::PAL ? 1.0 / 50 : 1.0 / 60;
        auto shift = *keyCode("lshift");

        for (size_t i = 0; i < length; i++) {

            auto key = *typedKey(text[i]);
            auto start = double(i * framesPerKey) * frame;

            if (key.shift) vamiga.keyboard.press(shift, start, 4 * frame);
            vamiga.keyboard.press(key.code, start + frame, 2 * frame);
        }
    });

    // One frame more, so that the last key has been released
    lua_pushinteger(L, lua_Integer(length) * framesPerKey + (length ? 1 : 0));
    return 1;
}

static const luaL_Reg keyboardFunctions[] = {

    { "press", keyboard_press },
    { "release", keyboard_release },
    { "release_all", keyboard_release_all },
    { "pressed", keyboard_pressed },
    { "type", keyboard_type },
    { nullptr, nullptr }
};


//
// The joystick table
//

// Returns the joystick of the control port the joystick[n] table belongs to.
// The number of the port is an upvalue of the metamethods.
static JoystickAPI &
joystick(lua_State *L)
{
    auto &vamiga = host(L).getEmulator();
    auto port = lua_tointeger(L, lua_upvalueindex(1));
    return port == 1 ? vamiga.controlPort1.joystick : vamiga.controlPort2.joystick;
}

// The state of a joystick is given by the fields x, y (-1, 0, or 1) and fire
// (a boolean). They are read from the core when they are accessed.
static int
joystick_index(lua_State *L)
{
    if (lua_type(L, 2) != LUA_TSTRING) return 0;
    auto key = lua_tostring(L, 2);

    JoystickInfo info;
    call(L, [&] {

        // Joystick events are queued; wait until the joystick has seen them
        host(L).getEmulator().sync();
        info = joystick(L).getInfo();
    });

    if (strcmp(key, "x") == 0)           { lua_pushinteger(L, info.axisX);
    } else if (strcmp(key, "y") == 0)    { lua_pushinteger(L, info.axisY);
    } else if (strcmp(key, "fire") == 0) { lua_pushboolean(L, info.button);
    } else {
        return 0;
    }
    return 1;
}

// Returns the value of an axis (-1, 0, or 1) which is assigned to a field
static lua_Integer
checkAxis(lua_State *L, const char *key)
{
    int valid;
    auto value = lua_tointegerx(L, 3, &valid);
    if (!valid || value < -1 || value > 1) luaL_error(L, "joystick.%s must be -1, 0, or 1", key);
    return value;
}

// Assigning to x, y, or fire moves the joystick or presses the button
static int
joystick_newindex(lua_State *L)
{
    auto key = lua_type(L, 2) == LUA_TSTRING ? lua_tostring(L, 2) : "";
    GamePadAction action;

    if (strcmp(key, "x") == 0) {

        auto x = checkAxis(L, key);
        action = x < 0 ? GamePadAction::PULL_LEFT : x > 0 ? GamePadAction::PULL_RIGHT : GamePadAction::RELEASE_X;

    } else if (strcmp(key, "y") == 0) {

        auto y = checkAxis(L, key);
        action = y < 0 ? GamePadAction::PULL_UP : y > 0 ? GamePadAction::PULL_DOWN : GamePadAction::RELEASE_Y;

    } else if (strcmp(key, "fire") == 0) {

        if (!lua_isboolean(L, 3)) luaL_error(L, "joystick.fire must be true or false");
        action = lua_toboolean(L, 3) ? GamePadAction::PRESS_FIRE : GamePadAction::RELEASE_FIRE;

    } else {
        return luaL_error(L, "a joystick has no field %s", luaL_tolstring(L, 2, nullptr));
    }

    call(L, [&] { joystick(L).trigger(action); });
    return 0;
}

static const luaL_Reg joystickMetamethods[] = {

    { "__index", joystick_index },
    { "__newindex", joystick_newindex },
    { nullptr, nullptr }
};


//
// The video table
//

/* video.screenshot(path [, x1, x2, y1, y2]) saves the latest picture as a PNG
 * file: the entire texture, or the cutout given in texels. The picture is the
 * one of the frame before the current frame (see MissingAPI.md).
 */
static int
video_screenshot(lua_State *L)
{
    auto path = luaL_checkstring(L, 1);
    auto cutout = !lua_isnone(L, 2);

    isize x1 = 0, x2 = 0, y1 = 0, y2 = 0;
    if (cutout) {

        x1 = isize(luaL_checkinteger(L, 2));
        x2 = isize(luaL_checkinteger(L, 3));
        y1 = isize(luaL_checkinteger(L, 4));
        y2 = isize(luaL_checkinteger(L, 5));
    }

    call(L, [&] {

        auto &videoPort = host(L).getEmulator().videoPort;
        if (cutout) {
            videoPort.saveTexture(path, x1, x2, y1, y2);
        } else {
            videoPort.saveTexture(path);
        }
    });
    return 0;
}

/* video.fingerprint([x1, x2, y1, y2]) returns a hash (32 bit FNV-1a) of the
 * latest picture: of the entire texture, or of the cutout given in texels as
 * for video.screenshot(). A script recognizes a picture by its fingerprint.
 */
static int
video_fingerprint(lua_State *L)
{
    isize x1 = 0, x2 = Tex::width, y1 = 0, y2 = Tex::height;
    if (!lua_isnone(L, 1)) {

        x1 = isize(luaL_checkinteger(L, 1));
        x2 = isize(luaL_checkinteger(L, 2));
        y1 = isize(luaL_checkinteger(L, 3));
        y2 = isize(luaL_checkinteger(L, 4));
    }
    if (x1 < 0 || x2 > Tex::width || x1 >= x2 || y1 < 0 || y2 > Tex::height || y1 >= y2) {
        return luaL_error(L, "invalid texture cutout");
    }

    u32 hash = utl::Hashable::fnvInit32();
    call(L, [&] {

        auto &videoPort = host(L).getEmulator().videoPort;
        videoPort.lockTexture();
        try {

            auto texels = videoPort.getTexture();
            for (isize y = y1; y < y2; y++) {
                for (isize x = x1; x < x2; x++) {
                    hash = utl::Hashable::fnvIt32(hash, texels[y * Tex::width + x]);
                }
            }
        } catch (...) { videoPort.unlockTexture(); throw; }
        videoPort.unlockTexture();
    });

    lua_pushinteger(L, hash);
    return 1;
}

static const luaL_Reg videoFunctions[] = {

    { "screenshot", video_screenshot },
    { "fingerprint", video_fingerprint },
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

    luaL_newlib(L, keyboardFunctions);
    lua_setglobal(L, "keyboard");

    // joystick[1] and joystick[2] are the joysticks in the two control ports
    lua_createtable(L, 2, 0);
    for (lua_Integer port = 1; port <= 2; port++) {

        lua_newtable(L);
        lua_createtable(L, 0, 2);
        lua_pushinteger(L, port);
        luaL_setfuncs(L, joystickMetamethods, 1);
        lua_setmetatable(L, -2);
        lua_rawseti(L, -2, port);
    }
    lua_setglobal(L, "joystick");

    luaL_newlib(L, videoFunctions);
    lua_setglobal(L, "video");
}

LuaEngine::~LuaEngine()
{
    lua_close(L);
}

void
LuaEngine::runFile(const fs::path &path)
{
    // As in the stand-alone interpreter, arg[0] is the script as it was given,
    // so that the script finds the files next to it
    lua_createtable(L, 0, 1);
    lua_pushstring(L, path.string().c_str());
    lua_rawseti(L, -2, 0);
    lua_setglobal(L, "arg");

    // Compile and run the file. The error message names the file and the line.
    if (luaL_loadfile(L, path.string().c_str()) != LUA_OK || lua_pcall(L, 0, 0, 0) != LUA_OK) {

        auto text = lua_tostring(L, -1);
        auto message = std::string(text ? text : "(the error is not a string)");
        lua_pop(L, 1);
        throw ScriptError(message);
    }
}

}
