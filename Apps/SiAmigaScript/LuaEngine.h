// -----------------------------------------------------------------------------
// This file is part of Silicium
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Written by Michael Wulff Nielsen
// Licensed under the Mozilla Public License v2
//
// See https://mozilla.org/MPL/2.0 for license information
// -----------------------------------------------------------------------------

#pragma once

#include <filesystem>
#include <stdexcept>

struct lua_State;

namespace vamiga {

namespace fs = std::filesystem;

class ScriptHost;

struct ScriptError : public std::runtime_error {
    using runtime_error::runtime_error;
};

// The Lua interpreter which runs the scripts
class LuaEngine {

    lua_State *L = nullptr;

public:

    explicit LuaEngine(ScriptHost &host);
    ~LuaEngine();

    LuaEngine(const LuaEngine &) = delete;
    LuaEngine &operator=(const LuaEngine &) = delete;

    // Runs a script file (throws ScriptError if it fails)
    void runFile(const fs::path &path);
};

}
