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

#include "VAmiga.h"
#include "LuaEngine.h"
#include "utl/concurrency.h"
#include <atomic>
#include <map>
#include <memory>
#include <vector>

namespace vamiga {

struct UsageError : public std::runtime_error {
    using runtime_error::runtime_error;
};

// The message listener
void process(const void *listener, Message msg, std::exception_ptr exception);

// The host runs the emulator on behalf of the scripts
class ScriptHost : Wakeable {

    // Parsed command line arguments
    std::map<string,string> keys;

    // RetroShell commands to execute at startup (option -e)
    std::vector<string> commands;

    // The emulator (created when the command line has been checked, as it
    // cannot be destroyed without having been launched, see MissingAPI.md)
    std::unique_ptr<VAmiga> emulator;

    // The interpreter which runs the script
    LuaEngine lua = LuaEngine(*this);

    // Return code
    int returnCode = 0;

    // What the host is waiting for (see process())
    enum class Waiting { Nothing, Commands, Frame };
    std::atomic<Waiting> waiting = Waiting::Nothing;


    //
    // Launching
    //

public:

    // Main entry point
    int main(int argc, char *argv[]);

private:

    // Parses the command line arguments (may throw)
    void parseArguments(int argc, char *argv[]);

    // Executes the RetroShell commands given on the command line
    void execCommands();


    //
    // Running
    //

public:

    // Processes an incoming message
    void process(Message msg);

    // Lets the paused emulator run until the end of the current frame
    void runFrame();

    // Returns the emulator (for the functions of the Lua API)
    VAmiga &getEmulator() { return *emulator; }
};

}
