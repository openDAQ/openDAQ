# Backend-agnostic part shared by every SFTP backend. Its headers keep the sftp_file_transfer/ include prefix.
set(BASE_NAME sftp_file_transfer_common)
set(MODULE_NAME ${SDK_TARGET_NAME}_${BASE_NAME})

set(SFTP_HEADERS_DIR include/sftp_file_transfer)

set(GENERATED_HEADERS_DIR ${CMAKE_CURRENT_BINARY_DIR}/${SFTP_HEADERS_DIR})

set(RTGEN_HEADERS_DIR ${SFTP_HEADERS_DIR})
set(RTGEN_OUTPUT_DIR ${GENERATED_HEADERS_DIR})
set(RTGEN_OUTPUT_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(RTGEN_RELATIVE_PARENT_DIR shared/libraries/sftp_file_transfer/common)

# Smart pointers only, no language bindings so far.
rtgen(SRC_FileTransfer file_transfer.h INTERNAL)

set(SRC_PublicHeaders sftp_authentication_config_factory.h
                      sftp_arguments.h
)

set(SRC_Cpp src/sftp_arguments.cpp
)

list(TRANSFORM SRC_PublicHeaders PREPEND "${SFTP_HEADERS_DIR}/")

list(APPEND SRC_PublicHeaders ${SRC_FileTransfer_PublicHeaders})

add_library(${MODULE_NAME} STATIC ${SRC_Cpp} ${SRC_PublicHeaders})
add_library(${SDK_TARGET_NAMESPACE}::${BASE_NAME} ALIAS ${MODULE_NAME})

target_include_directories(${MODULE_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}/include
)

target_link_libraries(${MODULE_NAME}
    PUBLIC
        daq::opendaq
)
