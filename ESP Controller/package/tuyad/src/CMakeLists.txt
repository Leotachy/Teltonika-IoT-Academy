# CMakeLists.txt for report_status_tuya

cmake_minimum_required(VERSION 3.19.2)

project(report_status LANGUAGES C)

# Configure optional libraries


# end of Configure optional libraries


# These are in cmake directory
message("Compiler = ${CMAKE_C_COMPILER_ID}")
include(cmake/${CMAKE_C_COMPILER_ID}.cmake)

add_subdirectory(src)
#add_subdirectory(thirdparty) #TEST


# Fake install target to get it to work with OpenWRT
install(CODE "message(STATUS \"This is a fake install for tuyad\")")
