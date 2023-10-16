set(APP_AUDIO audio_application)

set(SRC_Headers utils.h
)

set(SRC_Cpp main.cpp
            utils.cpp
)

prepend_include(${TARGET_FOLDER_NAME} SRC_Headers)
add_executable(${APP_AUDIO} ${SRC_Headers}
                              ${SRC_Cpp}
)

target_include_directories(${APP_AUDIO} PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/>
                                               $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include/>
)

target_link_libraries(${APP_AUDIO} PRIVATE daq::opendaq
)

add_dependencies(${APP_AUDIO} daq::ref_fb_module
                              daq::audio_device_module
)


