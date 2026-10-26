Lua 5.4.8, from https://www.lua.org/ftp/lua-5.4.8.tar.gz: the files in `src/`
without `lua.c` and `luac.c` (the stand-alone interpreter and compiler).

Lua is distributed under the MIT license. The license text is at the end of
`lua.h`.

The library is compiled as C++ (see `CMakeLists.txt`), so the headers are
included without the `extern "C"` of `lua.hpp`.
