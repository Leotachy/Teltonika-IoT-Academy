# CMakeLists.txt for src/tuyad

set(_target "tuyad")
add_executable(${_target})


target_sources(${_target} PRIVATE
        main.c

        tuya_interface.c
        tuya_interface.h

        tuya_action.c
        tuya_action.h

        json_utils.c
        json_utils.h

        ubus_interface.c
        ubus_interface.h
)

#if (BUILD_SHARED_LIBS)
#        target_link_libraries(${_target} PRIVATE libubox.so libubus.so)
#else()
#         target_link_libraries(${_target} PRIVATE libubox.a libubus.a)
#endif()

target_link_libraries(${_target} PRIVATE

			ubox
			ubus

                        link_core
                        utils_modules
)
