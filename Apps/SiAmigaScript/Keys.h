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
#include <optional>

namespace vamiga {

/* Returns the raw Amiga keycode of a key name. The names are the labels of the
 * keys in lower case ("return", "esc", "f1", "lshift", ...), the character on
 * the key of a US keyboard ("a", "1", "-", ...), or "kp" followed by the label
 * for the keys of the numeric keypad ("kp7", "kp-", "kpenter").
 */
std::optional<KeyCode> keyCode(const string &name);

// A key which types a character on a US keyboard, with or without shift
struct TypedKey {

    KeyCode code;
    bool shift;
};

// Returns the key which types a character, if there is one. Besides the
// printable characters, '\n' is typed with Return and '\t' with Tab.
std::optional<TypedKey> typedKey(char c);

}
