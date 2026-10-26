# Tests of SiAmigaScript

The tests are Lua scripts which run on the free AROS ROM, so they can run in CI.
A script ends with exit code 1 if a check fails.

```
build/Apps/SiAmigaScript/SiAmigaScript Apps/SiAmigaScript/Tests/testkit.lua
```

## testkit.lua

Boots the Amiga Test Kit and checks the keyboard and the joystick with its
keyboard and controller tests: it types "Hello" and checks the keys and raw
keycodes the Test Kit shows, then moves the joystick in port 2 and checks the
directions and the button.

Each check waits until a cutout of the picture has the expected fingerprint
(`video.fingerprint()`). If it does not appear, the picture is saved as
`testkit-<step>.png` in the current directory, and the error names the
fingerprint which was seen instead. A fingerprint is recorded by running the
script with a wrong value and checking the saved picture.

## AmigaTestKit.adf

Amiga Test Kit 1.21 by Keir Fraser, from
https://github.com/keirf/amiga-stuff/releases/tag/testkit-v1.21 (the ADF of
`AmigaTestKit-1.21.zip`, SHA-256
`abe7426c93619a7bb61ce10e3e66a4747fcaf22acd1d1876310033faa700ad28`).
It is in the public domain (The Unlicense).
