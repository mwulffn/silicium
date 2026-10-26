# The Lua API of SiAmigaScript

```
SiAmigaScript [-vm] [-e <command>]... [<script>]
```

The script runs after the RetroShell commands given with `-e`, which set up the
Amiga. It gets the standard libraries of Lua 5.4 and the functions below. While
the script runs, the emulator is paused; it only runs when the script asks for
it. Where RetroShell has a name for something, the API uses the same name.

An error in the script, or an error reported by the core, ends the app with
exit code 1.

## emu

| Function | Description |
| --- | --- |
| `emu.wait_frames([n])` | Lets the emulator run `n` frames (default 1), and returns when they have been emulated. An Amiga which is switched off is switched on first. |
| `emu.shell(command)` | Executes a RetroShell command and returns its output as a string. A command which fails raises an error with the message of RetroShell. The shell keeps its state between calls: `emu.shell("debugger")` switches to the debugger commands, `emu.shell("commander")` back. |

## cpu

The registers of the CPU are fields of the `cpu` table. They are read from the
core when they are accessed, and cannot be written yet.

| Field | Register |
| --- | --- |
| `cpu.d0` ... `cpu.d7` | Data registers |
| `cpu.a0` ... `cpu.a7` | Address registers (`a7` is the active stack pointer) |
| `cpu.pc` | Program counter: the address of the next instruction |
| `cpu.sr` | Status register |
| `cpu.usp`, `cpu.isp`, `cpu.msp` | User, interrupt and master stack pointer |
| `cpu.vbr` | Vector base register |

On a CPU which does not have a register, such as the master stack pointer or the
vector base register of a 68000, the field is 0.

## mem

The functions read memory the way the CPU sees it, but without side effects:
reading a register of a chip does not change it, as it would if the CPU read it.
Values are big endian and can be read at any address, odd addresses included.
The core uses the lower 24 bits of an address.

| Function | Description |
| --- | --- |
| `mem.peek_u8(addr)`, `mem.peek_u16(addr)`, `mem.peek_u32(addr)` | The byte, word or long word at `addr` |
| `mem.read_range(addr, len)` | `len` bytes from `addr` on, as a string (at most 16 MB) |

Custom chip registers which cannot be read on an Amiga, such as `BLTCON0`, read
as 0. What the chips hold in them is not memory and is not part of `mem`.

## keyboard

| Function | Description |
| --- | --- |
| `keyboard.press(key)` | Holds a key down |
| `keyboard.release(key)` | Lets a key go |
| `keyboard.release_all()` | Lets all keys go |
| `keyboard.pressed(key)` | Tells if a key is down |
| `keyboard.type(text)` | Types the text on a US keyboard and returns the number of frames this takes |

A key is given by its name or as a raw Amiga keycode (0 to 127). The names are:

- the character on the key of a US keyboard: `"a"` to `"z"`, `"0"` to `"9"`,
  `` "`" ``, `"-"`, `"="`, `"\\"`, `"["`, `"]"`, `";"`, `"'"`, `","`, `"."`, `"/"`
- `"space"`, `"backspace"`, `"tab"`, `"return"`, `"esc"`, `"del"`, `"help"`
- `"up"`, `"down"`, `"left"`, `"right"`, `"f1"` to `"f10"`
- `"lshift"`, `"rshift"`, `"capslock"`, `"ctrl"`, `"lalt"`, `"ralt"`, `"lamiga"`, `"ramiga"`
- the numeric keypad: `"kp0"` to `"kp9"`, `"kp."`, `"kp-"`, `"kp("`, `"kp)"`, `"kp/"`,
  `"kp*"`, `"kp+"`, `"kpenter"`

The two keys which only international keyboards have are given by their keycodes
(`0x2B` and `0x30`).

Key events are queued, and reach the keyboard in order before the next frame is
emulated. `keyboard.pressed()` waits until the keyboard has seen them.

`keyboard.type()` types the printable ASCII characters, `"\n"` with Return and
`"\t"` with Tab, using shift where a US keyboard needs it. It checks the whole
text first, and raises an error without typing anything if a character cannot
be typed. The keys are scheduled in emulated time (five frames per character)
and pressed while the emulator runs, so the script lets the frames pass:

```lua
emu.wait_frames(keyboard.type("echo hello\n"))
```

## video

| Function | Description |
| --- | --- |
| `video.screenshot(path [, x1, x2, y1, y2])` | Saves the latest picture as a PNG file: the entire texture, blanking areas included, or the cutout given in texels (x: super-hires pixels, y: lines; `x2` and `y2` are exclusive). |

The picture is the one of the frame before the current frame: the last frame
is handed to the video port shortly after it has ended (see `MissingAPI.md`).
In warp mode, the core draws only some of the frames unless frame skipping is
switched off with `-e "denise set FRAME_SKIPPING 0"`.

RetroShell's `screenshot save` is meant for the regression tests: it writes
raw pixel data and ends the emulator.
