// -----------------------------------------------------------------------------
// This file is part of Silicium
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Written by Michael Wulff Nielsen
// Licensed under the Mozilla Public License v2
//
// See https://mozilla.org/MPL/2.0 for license information
// -----------------------------------------------------------------------------

#include "Keys.h"
#include <map>

namespace vamiga {

// The keys of the Amiga keyboard and their raw keycodes (stored as int, as
// MSVC warns about the conversion of the literals to KeyCode in std::pair)
static const std::map<string, int> keys = {

    // Main block, top row
    { "`", 0x00 }, { "1", 0x01 }, { "2", 0x02 }, { "3", 0x03 }, { "4", 0x04 },
    { "5", 0x05 }, { "6", 0x06 }, { "7", 0x07 }, { "8", 0x08 }, { "9", 0x09 },
    { "0", 0x0A }, { "-", 0x0B }, { "=", 0x0C }, { "\\", 0x0D },

    // Main block, second row
    { "q", 0x10 }, { "w", 0x11 }, { "e", 0x12 }, { "r", 0x13 }, { "t", 0x14 },
    { "y", 0x15 }, { "u", 0x16 }, { "i", 0x17 }, { "o", 0x18 }, { "p", 0x19 },
    { "[", 0x1A }, { "]", 0x1B },

    // Main block, third row
    { "a", 0x20 }, { "s", 0x21 }, { "d", 0x22 }, { "f", 0x23 }, { "g", 0x24 },
    { "h", 0x25 }, { "j", 0x26 }, { "k", 0x27 }, { "l", 0x28 }, { ";", 0x29 },
    { "'", 0x2A },

    // Main block, fourth row
    { "z", 0x31 }, { "x", 0x32 }, { "c", 0x33 }, { "v", 0x34 }, { "b", 0x35 },
    { "n", 0x36 }, { "m", 0x37 }, { ",", 0x38 }, { ".", 0x39 }, { "/", 0x3A },

    // Other keys of the main block
    { "space", 0x40 }, { "backspace", 0x41 }, { "tab", 0x42 }, { "return", 0x44 },
    { "esc", 0x45 }, { "del", 0x46 }, { "help", 0x5F },

    // Cursor keys
    { "up", 0x4C }, { "down", 0x4D }, { "right", 0x4E }, { "left", 0x4F },

    // Function keys
    { "f1", 0x50 }, { "f2", 0x51 }, { "f3", 0x52 }, { "f4", 0x53 }, { "f5", 0x54 },
    { "f6", 0x55 }, { "f7", 0x56 }, { "f8", 0x57 }, { "f9", 0x58 }, { "f10", 0x59 },

    // Modifier keys
    { "lshift", 0x60 }, { "rshift", 0x61 }, { "capslock", 0x62 }, { "ctrl", 0x63 },
    { "lalt", 0x64 }, { "ralt", 0x65 }, { "lamiga", 0x66 }, { "ramiga", 0x67 },

    // Numeric keypad
    { "kp0", 0x0F }, { "kp1", 0x1D }, { "kp2", 0x1E }, { "kp3", 0x1F },
    { "kp4", 0x2D }, { "kp5", 0x2E }, { "kp6", 0x2F }, { "kp7", 0x3D },
    { "kp8", 0x3E }, { "kp9", 0x3F }, { "kp.", 0x3C }, { "kp-", 0x4A },
    { "kp(", 0x5A }, { "kp)", 0x5B }, { "kp/", 0x5C }, { "kp*", 0x5D },
    { "kp+", 0x5E }, { "kpenter", 0x43 }
};

std::optional<KeyCode>
keyCode(const string &name)
{
    if (auto it = keys.find(name); it != keys.end()) return KeyCode(it->second);
    return std::nullopt;
}

// The characters which are typed with shift on a US keyboard, and the names of
// their keys
static const std::map<char, string> shifted = {

    { '~', "`" }, { '!', "1" }, { '@', "2" }, { '#', "3" }, { '$', "4" },
    { '%', "5" }, { '^', "6" }, { '&', "7" }, { '*', "8" }, { '(', "9" },
    { ')', "0" }, { '_', "-" }, { '+', "=" }, { '|', "\\" }, { '{', "[" },
    { '}', "]" }, { ':', ";" }, { '"', "'" }, { '<', "," }, { '>', "." },
    { '?', "/" }
};

std::optional<TypedKey>
typedKey(char c)
{
    if (c == '\n') return TypedKey { *keyCode("return"), false };
    if (c == '\t') return TypedKey { *keyCode("tab"), false };
    if (c == ' ') return TypedKey { *keyCode("space"), false };

    if (c >= 'A' && c <= 'Z') {
        return TypedKey { *keyCode(string(1, char(c - 'A' + 'a'))), true };
    }
    if (auto it = shifted.find(c); it != shifted.end()) {
        return TypedKey { *keyCode(it->second), true };
    }
    if (auto code = keyCode(string(1, c))) {
        return TypedKey { *code, false };
    }
    return std::nullopt;
}

}
