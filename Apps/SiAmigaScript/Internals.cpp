// -----------------------------------------------------------------------------
// This file is part of Silicium
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Written by Michael Wulff Nielsen
// Licensed under the Mozilla Public License v2
//
// See https://mozilla.org/MPL/2.0 for license information
// -----------------------------------------------------------------------------

#include "vaconfig.h"
#include "Internals.h"
#include "Amiga.h"
#include <future>

namespace vamiga::internals {

string
execShellCommand(VAmiga &vamiga, const string &command)
{
    const auto timeout = std::chrono::seconds(10);

    auto promise = std::make_shared<std::promise<string>>();
    auto future = promise->get_future();

    // The completion handler gets the output of the command (as for the RPC
    // server), and runs on the emulator thread
    auto line = InputLine {

        .id = 0,
        .type = InputLine::Source::USER,
        .input = command,
        .completion = [promise](const string &output, std::exception_ptr error) {

            if (error) { promise->set_exception(error); } else { promise->set_value(output); }
        }
    };

    // asyncExec() does not lock the shell, so the emulator thread is kept out
    // of the way while the command is queued
    vamiga.suspend();
    vamiga.amiga.amiga->retroShell.asyncExec(line);
    vamiga.resume();

    if (future.wait_for(timeout) != std::future_status::ready) {
        throw std::runtime_error("The RetroShell command did not finish");
    }
    return future.get();
}

}
