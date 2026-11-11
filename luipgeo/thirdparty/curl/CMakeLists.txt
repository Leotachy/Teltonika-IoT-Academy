# CMakeLists.txt for luipgeo/thirdparty/curl

include(FetchContent)

if (LUIPGEO_XCOMPILE_PREFIX)
    #set(find_package_fix_patch git apply ${CMAKE_CURRENT_SOURCE_DIR}/patches/015-openssl_find_package_fix.patch)
    set(find_package_fix_patch patch -p0 -N -i "${CMAKE_CURRENT_SOURCE_DIR}/patches/015-openssl_find_package_fix.patch")
endif()
#COMMAND patch -p0 -N -i "${CMAKE_CURRENT_SOURCE_DIR}/patches/015-openssl_find_package_fix.patch
FetchContent_Declare(
    curl
    GIT_REPOSITORY https://github.com/curl/curl.git
    GIT_TAG curl-8_22_0
    PATCH_COMMAND ${find_package_fix_patch}
    UPDATE_DISCONNECTED 1
)

FetchContent_MakeAvailable(curl)
