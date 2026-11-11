# CMakeLists.txt for my_esp_controller

cmake_minimum_required(VERSION 3.19.2)

project(my_esp_controller LANGUAGES C)

# These are in cmake directory
message("Compiler = ${CMAKE_C_COMPILER_ID}")
include(cmake/${CMAKE_C_COMPILER_ID}.cmake)

add_subdirectory(src)




# Fake install target to get it to work with OpenWRT
install(CODE "message(STATUS \"This is a fake install for my_esp_controller\")")
