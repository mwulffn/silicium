// -----------------------------------------------------------------------------
// This file is part of Silicium
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Written by Michael Wulff Nielsen
// Licensed under the Mozilla Public License v2
//
// See https://mozilla.org/MPL/2.0 for license information
// -----------------------------------------------------------------------------

#include "ScriptHost.h"
#include "utl/support.h"
#include <iostream>
#include <sstream>

namespace vamiga {

int
ScriptHost::main(int argc, char *argv[])
{
    // Parse all command line arguments
    parseArguments(argc, argv);

    // Create the emulator and launch the emulator thread
    emulator = std::make_unique<VAmiga>();
    emulator->launch(this, vamiga::process);

    // Redirect shell output to the console in verbose mode
    if (keys.find("verbose") != keys.end()) emulator->retroShell.setStream(std::cout);

    // Set up the Amiga
    execCommands();

    // Run the script
    if (returnCode == 0 && keys.find("script") != keys.end()) lua.runFile(keys["script"]);

    emulator->removeListener();
    emulator->halt();
    return returnCode;
}

void
ScriptHost::parseArguments(int argc, char *argv[])
{
    for (isize i = 1; i < argc; i++) {

        auto arg = string(argv[i]);

        if (arg == "-v" || arg == "--verbose")   { keys["verbose"] = "1"; continue; }
        if (arg == "-m" || arg == "--messages")  { keys["messages"] = "1"; continue; }

        if (arg == "-e" || arg == "--exec") {

            if (i + 1 >= argc) throw UsageError("Option '" + arg + "' needs a command");
            commands.push_back(argv[++i]);
            continue;
        }

        if (arg[0] == '-') throw UsageError("Invalid option '" + arg + "'");

        // The only argument which is not an option is the script
        if (keys.find("script") != keys.end()) throw UsageError("More than one script is given");
        if (!utl::fileExists(arg)) throw UsageError("File " + arg + " does not exist");
        keys["script"] = arg;
    }
}

void
ScriptHost::execCommands()
{
    if (commands.empty()) return;

    std::stringstream script;
    for (auto &command : commands) script << command << std::endl;

    // Wait until the shell reports that it has finished or failed
    const auto timeout = utl::Time::seconds(500.0);
    waiting = Waiting::Commands;
    emulator->retroShell.execScriptAsync(script);
    waitForWakeUp(timeout);
    waiting = Waiting::Nothing;

    if (returnCode != 0 && keys.find("verbose") == keys.end()) {
        std::cerr << "A RetroShell command failed (use -v to see the output)" << std::endl;
    }
}

void
process(const void *listener, Message msg, std::exception_ptr)
{
    ((ScriptHost *)listener)->process(msg);
}

void
ScriptHost::process(Message msg)
{
    static bool messages = keys.find("messages") != keys.end();

    if (messages) {

        std::cout << MsgEnum::key(msg.type);
        std::cout << "(" << msg.value << ")";
        std::cout << std::endl;
    }

    /* The shell and the emulator send these messages for other reasons, too
     * (e.g., for a command a script executes with emu.shell()), so they only
     * count while the host is waiting for them.
     */
    switch (msg.type) {

        case Msg::RSH_ERROR:

            if (waiting == Waiting::Commands) { returnCode = 1; wakeUp(); }
            break;

        case Msg::RSH_DONE:

            // The commands run in the main shell (id 0)
            if (waiting == Waiting::Commands && msg.value == 0) wakeUp();
            break;

        case Msg::PAUSE:

            // The emulator has finished a frame (see runFrame)
            if (waiting == Waiting::Frame) wakeUp();
            break;

        case Msg::ABORT:

            if (waiting != Waiting::Nothing) wakeUp();
            break;

        default:
            break;
    }
}

void
ScriptHost::runFrame()
{
    const auto timeout = utl::Time::seconds(10.0);

    /* The emulator pauses at the end of the frame. Note that finishFrame()
     * stops before the last line of the frame has been drawn and before the
     * picture has been handed to the video port. Until then, the texture shows
     * the previous frame (see MissingAPI.md).
     */
    waiting = Waiting::Frame;
    emulator->finishFrame();
    waitForWakeUp(timeout);
    waiting = Waiting::Nothing;

    if (emulator->isRunning()) throw std::runtime_error("The frame did not finish");
}

}
