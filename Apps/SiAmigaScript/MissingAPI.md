# What SiAmigaScript is missing in the core

SiAmigaScript uses the public API of the vAmiga core (`VAmiga.h`) only. Where
that API does not offer what the app needs, the app lives with a limitation or
works around it. This file lists each such place: what is missing, why the app
wants it, and what it does in the meantime. The code refers to this file at
every place concerned.

## Missing API

### A frame step which ends when the picture of the frame is ready

**Used in:** `ScriptHost::runFrame()`

`finishFrame()` sets `AgnusDebugger::eofTrap`, which is checked in
`Agnus::eolHandler()` when the beam leaves the last line of the frame (line 0,
position 0). The emulator pauses after the CPU instruction which is running at
that moment.

At that point, the last line of the frame has not been drawn, and the picture
has not been handed to the video port: Agnus draws the previous line in
`hsyncHandler()` at position `$12`, and the texture buffers are swapped in
`Denise::vsyncHandler()` right after it. So after `finishFrame()`, the stable
texture is the picture of the frame before the one which has just been
emulated. (Measured: after 300 calls from power-on, `agnus.getInfo().frame` is
300 and the texture number is 298.) In warp mode the picture is further behind
unless frame skipping is switched off (`denise set FRAME_SKIPPING 0`), but that
is a setting.

**Why the app wants it:** a script inspects the machine between two frames, and
it should see the picture which belongs to the registers and the memory it reads.
A script which compares the screen with an expected picture, or which waits
until something appears on the screen, would otherwise act one frame late.

**In the meantime:** frames are run with `finishFrame()`, and the picture is one
frame behind. Nothing reads the picture yet.

**What would solve it:** for example, checking `eofTrap` after the buffers have
been swapped in `vsyncHandler()` instead of in `eolHandler()`, or a second kind
of frame step which ends there.

## Other findings

### `~VAmiga()` throws if the emulator thread has never been launched

**Used in:** `ScriptHost` (the emulator is held in a `std::unique_ptr`)

The destructor calls `halt()`, and `VAMIGA_PUBLIC` throws when the thread has
not been launched. An exception from a destructor terminates the program, for
example when an object holding a `VAmiga` is left because of an error before
`launch()` was called. Like `VAHeadless`, the app creates the emulator only after
the command line has been checked.
