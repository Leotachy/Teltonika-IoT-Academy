# CMakeLists.txt for luipgeo/src/luipgeo

set(_target "luipgeo")
add_executable(${_target})

target_sources(${_target} PRIVATE
        src/luipgeo.c
)

target_link_libraries(${_target} PRIVATE CURL::libcurl_static)

# Seems to be a bug with cJSON and FetchContent?!?!??????
# https://github.com/DaveGamble/cJSON/issues/816
# The three lines below would be replaced by a single one `target_link_libraries(${_target} PUBLIC cjson)`
# if the cJSON project would include the header files as part of the target cjson?
configure_file(${cJSON_SOURCE_DIR}/cJSON.h ${CMAKE_BINARY_DIR}/include/cJSON/cJSON.h)
target_include_directories(${_target} PUBLIC ${CMAKE_BINARY_DIR}/include)
target_link_libraries(${_target} PUBLIC cjson)