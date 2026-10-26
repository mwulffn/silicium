# SiAmigaScript

A command line app which runs the vAmiga core under the control of a Lua
script. The emulator is paused while the script runs, and only runs when the
script lets frames pass. The script presses keys, moves joysticks, reads memory
and CPU registers, executes RetroShell commands, and checks or saves the
picture.

## Building

SiAmigaScript needs no Qt and is built with the cores:

```
cmake -S . -B build -DSILICIUM_BUILD_APPS=OFF
cmake --build build --target SiAmigaScript
```

## Usage

```
SiAmigaScript [-vm] [-e <command>]... [<script>]

-e or --exec <command>  Execute a RetroShell command at startup
-v or --verbose         Print the RetroShell output
-m or --messages        Observe the message queue
<script>                Run a Lua script after the commands
```

The app ends with exit code 1 if a command or the script fails.

## Example

```lua
local roms = "Apps/Shared/Assets/Roms/"

emu.shell("amiga init A500_OCS_1MB")
emu.shell("mem load rom " .. roms .. "aros-20260820-rom.bin")
emu.shell("mem load ext " .. roms .. "aros-20260820-ext.bin")

emu.wait_frames(300)
print(string.format("pc = %08x", cpu.pc))
video.screenshot("boot.png")
```

## Files

| File | Contents |
| --- | --- |
| `LuaAPI.md` | The functions a script can use |
| `MissingAPI.md` | What the public API of the core lacks, and how the app works around it |
| `Tests/` | The test which runs in CI |
| `ThirdParty/lua/` | Lua 5.4.8 (MIT license) |
