# CMakeLists.txt for luipgeo/thirdparty/openssl

# Build our own version of openssl for xcompilation
if (LUIPGEO_XCOMPILE_PREFIX)
   
    if (BUILD_SHARED_LIBS)
        set(OPENSSL_USE_STATIC_LIBS False PARENT_SCOPE)
    else()
        set(OPENSSL_USE_STATIC_LIBS True PARENT_SCOPE)
    endif()

    set(OpenSSL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/openssl-4.0.3/lib/cmake/OpenSSL" PARENT_SCOPE)

endif()



