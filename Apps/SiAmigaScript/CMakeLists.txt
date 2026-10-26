# SiAmigaScript - Scriptable Amiga backend
#
#   A command line application that runs the vAmiga core under the control of
#   Lua scripts. It has no user interface and depends on neither Qt nor SDL,
#   so it is built together with the cores.

add_executable(SiAmigaScript

  main.cpp
  LuaEngine.cpp
  ScriptHost.cpp
)

target_link_libraries(SiAmigaScript PRIVATE VACore lua)

if(WIN32)
  target_link_libraries(SiAmigaScript PRIVATE ws2_32)
endif()

add_subdirectory(ThirdParty/lua)
