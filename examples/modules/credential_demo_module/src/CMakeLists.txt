set(LIB_NAME credential_demo_module)
set(MODULE_HEADERS_DIR ../include/${TARGET_FOLDER_NAME})

option(DAQMODULES_CREDENTIAL_DEMO_MODULE_ENABLE_PRIVATE_KEY_AUTH "Enable the OpenSSL-backed StandardPrivateKeyFile authentication method in the credential demo module" OFF)

set(SRC_Include common.h
                module_dll.h
                credential_demo_module_impl.h
                credential_demo_device_impl.h
                credential_demo_streaming_impl.h
                credential_demo_authenticator.h
)

set(SRC_Srcs module_dll.cpp
             credential_demo_module_impl.cpp
             credential_demo_device_impl.cpp
             credential_demo_streaming_impl.cpp
             credential_demo_authenticator.cpp
)

opendaq_prepend_include(${TARGET_FOLDER_NAME} SRC_Include)

source_group("module" FILES ${MODULE_HEADERS_DIR}/common.h
                            ${MODULE_HEADERS_DIR}/credential_demo_module_impl.h
                            ${MODULE_HEADERS_DIR}/credential_demo_device_impl.h
                            ${MODULE_HEADERS_DIR}/credential_demo_streaming_impl.h
                            ${MODULE_HEADERS_DIR}/credential_demo_authenticator.h
                            ${MODULE_HEADERS_DIR}/module_dll.h
                            module_dll.cpp
                            credential_demo_module_impl.cpp
                            credential_demo_device_impl.cpp
                            credential_demo_streaming_impl.cpp
                            credential_demo_authenticator.cpp
)

add_library(${LIB_NAME} SHARED ${SRC_Include}
                               ${SRC_Srcs}
)

add_library(${SDK_TARGET_NAMESPACE}::${LIB_NAME} ALIAS ${LIB_NAME})

opendaq_target_pch(${LIB_NAME} <opendaq/opendaq.h>)
opendaq_target_unity_libs(${LIB_NAME} BATCH_SIZE 4)

target_link_libraries(${LIB_NAME} PUBLIC daq::opendaq
)

if (DAQMODULES_CREDENTIAL_DEMO_MODULE_ENABLE_PRIVATE_KEY_AUTH)
    message(STATUS "    - Credential demo module private-key authentication enabled")
    find_package(OpenSSL REQUIRED)
    target_link_libraries(${LIB_NAME} PRIVATE OpenSSL::Crypto)
    target_compile_definitions(${LIB_NAME} PRIVATE DAQMODULES_CREDENTIAL_DEMO_MODULE_ENABLE_PRIVATE_KEY_AUTH)
else()
    message(STATUS "    - Credential demo module private-key authentication disabled (no OpenSSL dependency)")
endif()

target_include_directories(${LIB_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
                                              $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include>
                                              $<INSTALL_INTERFACE:include>
)

opendaq_set_module_properties(${LIB_NAME} ${PROJECT_VERSION_MAJOR})
opendaq_generate_version_header(${LIB_NAME})
