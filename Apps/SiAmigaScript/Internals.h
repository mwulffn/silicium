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

/* Everything SiAmigaScript does with the internals of the core because the
 * public API does not offer it. Nothing else in the app uses the internals.
 * MissingAPI.md describes each function: what is missing, and why the app
 * needs it.
 */
namespace vamiga::internals {

// Executes a RetroShell command and returns its output. If the command fails,
// its error is thrown.
string execShellCommand(VAmiga &vamiga, const string &command);

}
