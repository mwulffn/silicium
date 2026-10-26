-- -----------------------------------------------------------------------------
-- This file is part of Silicium
--
-- Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
-- Written by Michael Wulff Nielsen
-- Licensed under the Mozilla Public License v2
--
-- See https://mozilla.org/MPL/2.0 for license information
-- -----------------------------------------------------------------------------

-- Tests the keyboard and the joystick with the Amiga Test Kit, which boots on
-- the AROS ROM. Each step waits until the Test Kit shows the expected picture.
--
-- Usage: SiAmigaScript Tests/testkit.lua

local dir = arg[0]:match("(.*[/\\])") or "./"
local roms = dir .. "../../Shared/Assets/Roms/"

-- Waits until a cutout of the picture has the expected fingerprint. If it
-- does not appear, the picture is saved, and the error names the fingerprint
-- which was seen instead (which is how the fingerprints below were found).
local function expect(step, fingerprint, cutout, max_frames)
    local seen
    for frame = 1, max_frames do
        emu.wait_frames(1)
        seen = video.fingerprint(table.unpack(cutout))
        if seen == fingerprint then return end
    end
    video.screenshot("testkit-" .. step .. ".png")
    error(string.format("%s: expected fingerprint %08x, but saw %08x (see testkit-%s.png)",
        step, fingerprint, seen, step), 2)
end

-- Presses a key (or a combination of keys) and lets it go
local function press(...)
    for _, key in ipairs({...}) do keyboard.press(key) end
    emu.wait_frames(3)
    keyboard.release_all()
end

-- The cutouts of the screens which are checked (the mouse pointer is left out)
local menuArea = { 590, 1300, 87, 218 }
local keyboardArea = { 430, 1620, 77, 218 }
local joystickArea = { 1140, 1500, 135, 203 }

-- Set up an Amiga 500 with the AROS ROM and boot the Test Kit
emu.shell("amiga init A500_OCS_1MB")
emu.shell("mem load rom " .. roms .. "aros-20260820-rom.bin")
emu.shell("mem load ext " .. roms .. "aros-20260820-ext.bin")
emu.shell("amiga set WARP_MODE ALWAYS")
emu.shell("denise set FRAME_SKIPPING 0")
emu.shell("df0 insert " .. dir .. "AmigaTestKit.adf")
expect("boot", 0x8f73b887, menuArea, 1000)

-- The keyboard test shows the keys and lists the raw keycodes it receives
press("f2")
expect("keyboard-screen", 0x081aa6b1, keyboardArea, 200)
emu.wait_frames(keyboard.type("Hello"))
expect("keyboard", 0x70f3d88b, keyboardArea, 100)

-- Back to the main menu
press("ctrl", "lalt")
expect("menu", 0x8f73b887, menuArea, 100)

-- The controller test shows the directions and the button of the joystick
press("f4")
expect("joystick-screen", 0x4d301209, joystickArea, 200)
local joy = joystick[2]
joy.x, joy.y, joy.fire = -1, -1, true
expect("joystick-up-left-fire", 0x2a82d62d, joystickArea, 100)

joy.x, joy.y, joy.fire = 1, 1, false
assert(joy.x == 1 and joy.y == 1 and not joy.fire)
expect("joystick-down-right", 0xe5733aed, joystickArea, 100)

print("testkit.lua: PASS")
