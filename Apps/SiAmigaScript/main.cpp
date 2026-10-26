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
#include <iostream>

int main(int argc, char *argv[])
{
    try {

        return vamiga::ScriptHost().main(argc, argv);

    } catch (vamiga::UsageError &e) {

        std::cout << "Usage: SiAmigaScript [-vm] [-e <command>]..." << std::endl;
        std::cout << std::endl;
        std::cout << "       -e or --exec <command>  Execute a RetroShell command at startup" << std::endl;
        std::cout << "       -v or --verbose         Print the RetroShell output" << std::endl;
        std::cout << "       -m or --messages        Observe the message queue" << std::endl;
        std::cout << std::endl;
        std::cout << "       Example: SiAmigaScript -e \"amiga init A500_OCS_1MB\" -e \"mem load rom kick.rom\"" << std::endl;
        std::cout << std::endl;
        std::cout << e.what() << std::endl;

    } catch (vamiga::CoreError &e) {

        std::cout << "Emulator Error: " << e.what() << std::endl;

    } catch (std::exception &e) {

        std::cout << "System Error: " << e.what() << std::endl;

    } catch (...) {

        std::cout << "Error" << std::endl;
    }

    return 1;
}
