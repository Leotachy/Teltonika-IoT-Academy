# CMakeLists.txt for luipgeo

cmake_minimum_required(VERSION 3.19.2)

project(luipgeo LANGUAGES C)

# These are in cmake directory
message("Compiler = ${CMAKE_C_COMPILER_ID}")
include(cmake/${CMAKE_C_COMPILER_ID}.cmake)

add_subdirectory(thirdparty)
add_subdirectory(src)





