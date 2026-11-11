# CMakeLists.txt for my_esp_controller/src/my_esp_controller

set(_target "my_esp_controller")
add_executable(${_target})

target_sources(${_target} PRIVATE
        main.c
        esp_device_interface.c
        esp_device_interface.h
        util.h
        ubus_interface.c
        ubus_interface.h
        
)


target_link_libraries(${_target} PRIVATE
			serialport
                        ubus
                        ubox
                        link_core
                        utils_modules)
