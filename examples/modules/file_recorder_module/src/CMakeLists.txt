set(LIB_NAME file_recorder_module)
set(MODULE_HEADERS_DIR ../include/${TARGET_FOLDER_NAME})

set(SRC_Include module_dll.h
                common.h
                file_format.h
                file_recorder_module_impl.h
                file_recorder_fb_impl.h
                file_player_fb_impl.h
                signal_file_writer.h
                signal_file_reader.h
)

set(SRC_Srcs module_dll.cpp
             file_format.cpp
             file_recorder_module_impl.cpp
             file_recorder_fb_impl.cpp
             file_player_fb_impl.cpp
             signal_file_writer.cpp
             signal_file_reader.cpp
)

opendaq_prepend_include(${TARGET_FOLDER_NAME} SRC_Include)

source_group("module" FILES ${MODULE_HEADERS_DIR}/file_recorder_module_impl.h
                            ${MODULE_HEADERS_DIR}/module_dll.h
                            ${SRC_Srcs}
)

add_library(${LIB_NAME} SHARED ${SRC_Include}
                               ${SRC_Srcs}
)
add_library(${SDK_TARGET_NAMESPACE}::${LIB_NAME} ALIAS ${LIB_NAME})

if (MSVC)
    target_compile_options(${LIB_NAME} PRIVATE /bigobj)
endif()

target_link_libraries(${LIB_NAME} PRIVATE $<BUILD_INTERFACE:std::filesystem>
                                  PUBLIC daq::opendaq
)

target_include_directories(${LIB_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
                                              $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include>
                                              $<INSTALL_INTERFACE:include>
)

opendaq_set_module_properties(${LIB_NAME} ${PROJECT_VERSION_MAJOR})
opendaq_generate_version_header(${LIB_NAME})
