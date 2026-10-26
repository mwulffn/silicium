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
#include <cstdio>
#include <iostream>

namespace vamiga {

int
ScriptHost::main(int argc, char *argv[])
{
    // Parse all command line arguments
    parseArguments(argc, argv);

    // Create the emulator and launch the emulator thread
    emulator = std::make_unique<VAmiga>();
    auto &vamiga = *emulator;
    vamiga.launch(this, vamiga::process);

    // Set up the Amiga and switch it on
    configure();
    vamiga.powerOn();

    // Run the requested number of frames
    auto frames = std::stol(keys["frames"]);
    for (long i = 0; i < frames; i++) runFrame();

    auto &cpu = vamiga.cpu.getInfo();
    auto &agnus = vamiga.agnus.getInfo();
    printf("frame %lld  pc $%06x\n", (long long)agnus.frame, cpu.pc0);

    vamiga.removeListener();
    vamiga.halt();
    return 0;
}

void
ScriptHost::parseArguments(int argc, char *argv[])
{
    // Defaults
    keys["model"] = "A500_OCS_1MB";
    keys["frames"] = "0";

    for (isize i = 1; i < argc; i++) {

        auto arg = string(argv[i]);

        // All options take a value
        auto value = [&]() {

            if (i + 1 >= argc) throw UsageError("Option '" + arg + "' needs a value");
            return string(argv[++i]);
        };

        if (arg == "--model")   { keys["model"] = value(); continue; }
        if (arg == "--rom")     { keys["rom"] = value(); continue; }
        if (arg == "--ext")     { keys["ext"] = value(); continue; }
        if (arg == "--frames")  { keys["frames"] = value(); continue; }

        throw UsageError("Invalid option '" + arg + "'");
    }

    if (!ConfigSchemeEnum::parseEnum(keys["model"])) {
        throw UsageError("Unknown model '" + keys["model"] + "'");
    }
    if (keys.find("rom") == keys.end()) {
        throw UsageError("No Kickstart Rom given");
    }
    if (keys["frames"].find_first_not_of("0123456789") != string::npos) {
        throw UsageError("Invalid number of frames '" + keys["frames"] + "'");
    }
}

void
ScriptHost::configure()
{
    auto &vamiga = *emulator;

    vamiga.set(*ConfigSchemeEnum::parseEnum(keys["model"]));

    // The host decides when the emulator runs, so the core never waits
    vamiga.set(Opt::AMIGA_WARP_MODE, (i64)Warp::ALWAYS);

    // Every frame is drawn, so that every frame can be inspected
    vamiga.set(Opt::DENISE_FRAME_SKIPPING, 0);

    vamiga.mem.loadRom(keys["rom"]);
    if (keys.find("ext") != keys.end()) vamiga.mem.loadExt(keys["ext"]);
}

void
ScriptHost::runFrame()
{
    const auto timeout = utl::Time::seconds(10.0);

    // The emulator pauses at the end of the frame
    emulator->finishFrame();
    waitForWakeUp(timeout);
}

void
process(const void *listener, Message msg, std::exception_ptr)
{
    ((ScriptHost *)listener)->process(msg);
}

void
ScriptHost::process(Message msg)
{
    switch (msg.type) {

        case Msg::PAUSE:

            wakeUp();
            break;

        default:
            break;
    }
}

}
