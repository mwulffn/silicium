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
#include "utl/concurrency.h"
#include <map>
#include <memory>

namespace vamiga {

struct UsageError : public std::runtime_error {
    using runtime_error::runtime_error;
};

// The message listener
void process(const void *listener, Message msg, std::exception_ptr exception);

/* The host runs the emulator on behalf of the scripts. It keeps the emulator
 * paused, and lets it run one frame at a time.
 */
class ScriptHost : Wakeable {

    // Parsed command line arguments
    std::map<string,string> keys;

    // The emulator (created when the command line has been checked, as it
    // cannot be destroyed without having been launched)
    std::unique_ptr<VAmiga> emulator;


    //
    // Launching
    //

public:

    // Main entry point
    int main(int argc, char *argv[]);

private:

    // Parses the command line arguments (may throw)
    void parseArguments(int argc, char *argv[]);

    // Configures the Amiga as specified on the command line (may throw)
    void configure();


    //
    // Running
    //

public:

    // Processes an incoming message
    void process(Message msg);

private:

    // Lets the emulator run until the end of the current frame
    void runFrame();
};

}
