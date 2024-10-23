cmake_minimum_required(VERSION 3.20)
project(console_application VERSION ${OPENDAQ_EXAMPLES_VERSION})

find_package(openDAQ)
add_compile_definitions(MODULE_PATH="${OPENDAQ_MODULES_DIR}")

set(SRC_Headers console_application.h
                app_channel.h
				app_device.h
				app_function_block.h
				app_signal.h
				app_property_object.h
				app_input_port.h
)

set(SRC_Cpp console_application.cpp
            main.cpp
            app_channel.cpp
			app_device.cpp
			app_signal.cpp
			app_function_block.cpp
			app_property_object.cpp
			app_input_port.cpp
)

add_executable(console_application ${SRC_Headers}
                                   ${SRC_Cpp}
)

target_link_libraries(console_application PRIVATE daq::opendaq)
