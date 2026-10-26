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

        std::cout << "Usage: SiAmigaScript --rom <file> [options]" << std::endl;
        std::cout << std::endl;
        std::cout << "       --model <scheme>    A1000_OCS_1MB, A500_OCS_1MB (default), A500_ECS_1MB," << std::endl;
        std::cout << "                           A500_PLUS_1MB or A1200_2MB" << std::endl;
        std::cout << "       --rom <file>        Kickstart Rom" << std::endl;
        std::cout << "       --ext <file>        Extension Rom (needed by the AROS Rom)" << std::endl;
        std::cout << "       --frames <n>        Number of frames to run" << std::endl;
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
