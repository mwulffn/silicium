set(LUA_SOURCES

  lapi.c
  lauxlib.c
  lbaselib.c
  lcode.c
  lcorolib.c
  lctype.c
  ldblib.c
  ldebug.c
  ldo.c
  ldump.c
  lfunc.c
  lgc.c
  linit.c
  liolib.c
  llex.c
  lmathlib.c
  lmem.c
  loadlib.c
  lobject.c
  lopcodes.c
  loslib.c
  lparser.c
  lstate.c
  lstring.c
  lstrlib.c
  ltable.c
  ltablib.c
  ltm.c
  lundump.c
  lutf8lib.c
  lvm.c
  lzio.c
)

# Lua is compiled as C++, so that a Lua error is thrown as a C++ exception
# (see LUAI_THROW in ldo.c) and unwinds the C++ functions of the app properly
# instead of jumping over them with longjmp
set_source_files_properties(${LUA_SOURCES} PROPERTIES LANGUAGE CXX)
add_library(lua ${LUA_SOURCES})

target_include_directories(lua PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
if(UNIX)
  target_compile_definitions(lua PRIVATE LUA_USE_POSIX)
endif()
