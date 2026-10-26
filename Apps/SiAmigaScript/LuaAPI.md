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
