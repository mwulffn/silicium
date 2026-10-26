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
#include "lua.hpp"

namespace vamiga {

LuaEngine::LuaEngine()
{
    L = luaL_newstate();
    if (!L) throw std::bad_alloc();

    // Scripts get the standard libraries, as in the stand-alone interpreter
    luaL_openlibs(L);
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
