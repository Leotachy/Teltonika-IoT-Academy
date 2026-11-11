# CMakeLists.txt for luipgeo/src/hello_world

set(_target "hello_world")
add_executable(${_target})

target_sources(${_target} PRIVATE
        src/hello_world.c
)
