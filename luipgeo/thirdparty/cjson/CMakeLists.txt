# CMakeLists.txt for luipgeo/thirdparty/cjson

include(FetchContent)

FetchContent_Declare(
    cJSON
    GIT_REPOSITORY https://github.com/davegamble/cjson.git
    GIT_TAG v1.7.19
)

FetchContent_MakeAvailable(cJSON)
