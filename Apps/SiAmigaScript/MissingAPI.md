# What SiAmigaScript is missing in the core

SiAmigaScript uses the public API of the vAmiga core (`VAmiga.h`). Where that
API does not offer what the app needs, the app lives with a limitation or works
around it. This file lists each such place: what is missing, why the app wants
it, and what it does in the meantime. The code refers to this file at every
place concerned, and every use of the internals of the core is in
`Internals.cpp`.

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
frame behind. `video.screenshot()` and `video.fingerprint()` read the picture
of the previous frame, which LuaAPI.md tells.

**What would solve it:** for example, checking `eofTrap` after the buffers have
been swapped in `vsyncHandler()` instead of in `eolHandler()`, or a second kind
of frame step which ends there.

### Reading memory with the side effects of a CPU access

**Used in:** `mem.peek_u8()`, `mem.peek_u16()`, `mem.peek_u32()`, `mem.read_range()`

`MemoryDebuggerAPI` reads memory without side effects (`spypeek8()`,
`spypeek16()`). There is no public function which reads memory the way the CPU
does, with the effects of the access: reading a CIA's interrupt control
register clears it, for example.

**Why the app might want it:** reading without side effects is the right thing
for inspecting the machine, and it is all the app offers. A script which wants
to act like the program, such as acknowledging an interrupt by reading the
register, cannot do that.

**In the meantime:** the `mem` functions read without side effects. Nothing
needs reads with side effects yet.

### Executing a RetroShell command and getting its output

**Used in:** `internals::execShellCommand()` (for `emu.shell()`)

`RetroShellAPI` can execute commands (`execScriptAsync()`), and reports with a
message when they have been executed or one of them has failed. It does not
give the output of a command, and not the error of a failed one. All output
goes to the console (`text()`, `setStream()`), mixed with the echoed command,
the prompt and line numbers.

Internally, `RetroShell::asyncExec()` takes an `InputLine` with a completion
handler, which receives exactly the output of the command, or its exception.
The JSON RPC server is built on it.

**Why the app wants it:** `emu.shell()` gives scripts everything RetroShell
can do, in particular its inspectors (`r cpu`, `? copper list`, `os tasks`,
...), without the app having to offer each of them again. A script needs the
output of the command, and must learn when it fails.

**In the meantime:** `execShellCommand()` calls `RetroShell::asyncExec()`
directly, with a completion handler, as the RPC server does. As `asyncExec()`
does not lock the shell (see below), the emulator thread is suspended with the
public `suspend()` while the command is queued.

**What would solve it:** a public function which executes a command and
returns its output or throws its error, such as `string RetroShellAPI::exec(const
string &command)`, or an `execAsync()` with a completion handler.

## Other findings

### `RetroShell::asyncExec()` does not lock the command queue

`asyncExec()` adds the command to `commands` without taking the shell's lock,
while `exec()` processes the same vector on the emulator thread under the lock.
`asyncExecScript()` does take the lock. The RPC and RSH servers call
`asyncExec()` when a request arrives, which looks like it may happen while the
emulator thread processes the queue. Not verified; worth a look.

### `~VAmiga()` throws if the emulator thread has never been launched

**Used in:** `ScriptHost` (the emulator is held in a `std::unique_ptr`)

The destructor calls `halt()`, and `VAMIGA_PUBLIC` throws when the thread has
not been launched. An exception from a destructor terminates the program, for
example when an object holding a `VAmiga` is left because of an error before
`launch()` was called. Like `VAHeadless`, the app creates the emulator only after
the command line has been checked.
