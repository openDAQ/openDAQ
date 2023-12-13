set(BASE_NAME device)

set(SDK_HEADERS_DIR ../include/${MAIN_TARGET})
set(GENERATED_HEADERS_DIR ${CMAKE_CURRENT_BINARY_DIR}/${SDK_HEADERS_DIR})

set(RTGEN_HEADERS_DIR ${SDK_HEADERS_DIR})
set(RTGEN_OUTPUT_DIR ${GENERATED_HEADERS_DIR})
set(RTGEN_OUTPUT_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR})

rtgen(SRC_Device device.h)
rtgen(SRC_DeviceDomain device_domain.h)
rtgen(SRC_DeviceInfo device_info.h)
rtgen(SRC_DeviceInfoConfig device_info_config.h)
rtgen(SRC_IoFolderConfig io_folder_config.h)
rtgen(SRC_DeviceType device_type.h)

source_group("device" FILES ${SDK_HEADERS_DIR}/device.h
                            ${SDK_HEADERS_DIR}/device_impl.h
                            ${SDK_HEADERS_DIR}/device_domain.h
                            ${SDK_HEADERS_DIR}/device_info.h
                            ${SDK_HEADERS_DIR}/device_info_config.h
                            ${SDK_HEADERS_DIR}/device_info_impl.h
                            ${SDK_HEADERS_DIR}/device_info_factory.h
                            ${SDK_HEADERS_DIR}/device_private.h
                            device_info_impl.cpp
)

source_group("device_type" FILES ${SDK_HEADERS_DIR}/device_type.h
                                 ${SDK_HEADERS_DIR}/device_type_impl.h
                                 ${SDK_HEADERS_DIR}/device_type_factory.h
                                 device_type_impl.cpp
)

source_group("io_folder" FILES ${SDK_HEADERS_DIR}/io_folder_config.h
                               ${SDK_HEADERS_DIR}/io_folder_impl.h
                               ${SDK_HEADERS_DIR}/io_folder_factory.h
                               io_folder_impl.cpp
)

source_group("core_events" FILES ${SDK_HEADERS_DIR}/core_opendaq_event_args.h
                                 ${SDK_HEADERS_DIR}/core_opendaq_event_args_factory.h
                                 ${SDK_HEADERS_DIR}/core_opendaq_event_args_impl.h
                                 core_opendaq_event_args_impl.cpp
)

set(SRC_Cpp device_info_impl.cpp
            io_folder_impl.cpp
            device_type_impl.cpp
            core_opendaq_event_args_impl.cpp
)

set(SRC_PublicHeaders device_info_factory.h
                      device_errors.h
                      device_exceptions.h
                      device_impl.h
                      io_folder_factory.h
                      io_folder_impl.h
                      device_type_factory.h
                      device_private.h
                      core_opendaq_event_args_factory.h
                      core_opendaq_event_args.h
)

set(SRC_PrivateHeaders device_info_impl.h
                       device_type_impl.h
)

prepend_include(${MAIN_TARGET} SRC_PrivateHeaders)
prepend_include(${MAIN_TARGET} SRC_PublicHeaders)

list(APPEND SRC_Cpp ${SRC_Device_Cpp}
                    ${SRC_DeviceDomain_Cpp}
                    ${SRC_DeviceInfo_Cpp}
                    ${SRC_DeviceInfoConfig_Cpp}
                    ${SRC_IoFolderConfig_Cpp}
                    ${SRC_DeviceType_Cpp}
)

list(APPEND SRC_PublicHeaders ${SRC_Device_PublicHeaders}
                              ${SRC_DeviceDomain_PublicHeaders}
                              ${SRC_DeviceInfo_PublicHeaders}
                              ${SRC_DeviceInfoConfig_PublicHeaders}
                              ${SRC_IoFolderConfig_PublicHeaders}
                              ${SRC_DeviceType_PublicHeaders}
)

list(APPEND SRC_PrivateHeaders ${SRC_Device_PrivateHeaders}
                               ${SRC_DeviceDomain_PrivateHeaders}
                               ${SRC_DeviceInfo_PrivateHeaders}
                               ${SRC_DeviceInfoConfig_PrivateHeaders}
                               ${SRC_IoFolderConfig_PrivateHeaders}
                               ${SRC_DeviceType_PrivateHeaders}
)

opendaq_add_library(${BASE_NAME} STATIC
    ${SRC_Cpp}
    ${SRC_PrivateHeaders}
    ${SRC_PublicHeaders}
    ${ConfigHeaderSource}
    device.natvis
)

opendaq_target_link_libraries(${BASE_NAME}
    PUBLIC
        daq::coreobjects
        daq::function_block
        daq::utility
        daq::component
)

opendaq_target_include_directories(${BASE_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include/>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include/>

        $<INSTALL_INTERFACE:include>
)
