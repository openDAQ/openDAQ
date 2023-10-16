set(LIB_NAME audio_device_module)
set(MODULE_HEADERS_DIR ../include/${TARGET_FOLDER_NAME})

set(SRC_Include common.h
                module_dll.h
                audio_device_module_impl.h
                audio_device_impl.h
                audio_channel_impl.h
                wav_writer_fb_impl.h
                miniaudio_utils.h
)

set(SRC_Srcs module_dll.cpp
             audio_device_module_impl.cpp
             audio_device_impl.cpp
             audio_channel_impl.cpp
             wav_writer_fb_impl.cpp
             miniaudio_utils.cpp
)

prepend_include(${TARGET_FOLDER_NAME} SRC_Include)

source_group("module" FILES ${MODULE_HEADERS_DIR}/audio_device_module_common.h
                            ${MODULE_HEADERS_DIR}/audio_device_module_impl.h
                            ${MODULE_HEADERS_DIR}/audio_channel_impl.h
                            ${MODULE_HEADERS_DIR}/audio_device_impl.h
                            ${MODULE_HEADERS_DIR}/wav_writer_fb_impl.h
                            ${MODULE_HEADERS_DIR}/miniaudio_utils.h
                            ${MODULE_HEADERS_DIR}/module_dll.h
                            module_dll.cpp
                            audio_device_module_impl.cpp
                            audio_device_impl.cpp
                            audio_channel_impl.cpp
                            wav_writer_fb_impl.cpp
                            miniaudio_utils.cpp
)


add_library(${LIB_NAME} SHARED ${SRC_Include}
                               ${SRC_Srcs}
)

add_library(${SDK_TARGET_NAMESPACE}::${LIB_NAME} ALIAS ${LIB_NAME})

if("${Boost_LIBRARIES}")
    set(BOOST_LIBS ${Boost_LIBRARIES})
endif()

target_link_libraries(${LIB_NAME}
        PUBLIC
            daq::opendaq
        PRIVATE
            miniaudio::miniaudio
            ${BOOST_LIBS}
)

# Should link only what you need $<BUILD_INTERFACE:Boost::locale>
target_link_libraries(${LIB_NAME} PRIVATE $<BUILD_INTERFACE:Boost::locale>)

if (CMAKE_COMPILER_IS_GNUCXX AND MINGW)
    target_compile_options(${LIB_NAME} PRIVATE -Wno-error)
endif()

target_include_directories(${LIB_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
                                              $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include>
                                              $<INSTALL_INTERFACE:include>
                                              ${Boost_INCLUDE_DIRS}
)

opendaq_set_module_properties(${LIB_NAME} ${PROJECT_VERSION_MAJOR})
create_version_header(${LIB_NAME})
