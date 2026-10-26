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
