set(LIB_NAME simulator_device_module)
set(MODULE_HEADERS_DIR ../include/${TARGET_FOLDER_NAME})

set(SRC_Include common.h
                module_dll.h
                simulator_device_module_impl.h
                simulator_device_impl.h
                simulator_channel_impl.h
                signal_generator.h
)

set(SRC_Srcs module_dll.cpp
             simulator_device_module_impl.cpp
             simulator_device_impl.cpp
             simulator_channel_impl.cpp
             signal_generator.cpp
)

prepend_include(${TARGET_FOLDER_NAME} SRC_Include)

source_group("module" FILES ${MODULE_HEADERS_DIR}/simulator_device_module_common.h
                            ${MODULE_HEADERS_DIR}/simulator_device_module_impl.h
                            ${MODULE_HEADERS_DIR}/simulator_channel_impl.h
                            ${MODULE_HEADERS_DIR}/simulator_device_impl.h
                            ${MODULE_HEADERS_DIR}/signal_generator.h
                            ${MODULE_HEADERS_DIR}/utils.h
                            ${MODULE_HEADERS_DIR}/module_dll.h
                            module_dll.cpp
                            tuils.cpp
                            simulator_device_module_impl.cpp
                            simulator_device_impl.cpp
                            simulator_channel_impl.cpp
                            signal_generator.cpp
)


add_library(${LIB_NAME} SHARED ${SRC_Include}
                               ${SRC_Srcs}
)

add_library(${SDK_TARGET_NAMESPACE}::${LIB_NAME} ALIAS ${LIB_NAME})

if (MSVC)
    target_compile_options(${LIB_NAME} PRIVATE /bigobj)
endif()

target_link_libraries(${LIB_NAME} PUBLIC daq::opendaq
)

target_link_libraries(${LIB_NAME} PRIVATE $<BUILD_INTERFACE:Boost::asio>
)

target_include_directories(${LIB_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
                                              $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include>
                                              $<INSTALL_INTERFACE:include>
)

opendaq_set_module_properties(${LIB_NAME} ${PROJECT_VERSION_MAJOR})
create_version_header(${LIB_NAME})
