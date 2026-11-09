set(BASE_NAME sftp_file_transfer_libssh2)
set(MODULE_NAME ${SDK_TARGET_NAME}_${BASE_NAME})

set(SRC_PublicHeaders include/sftp_file_transfer/libssh2/sftp_file_transfer_libssh2_factory.h
)

set(SRC_PrivateHeaders include/sftp_file_transfer/libssh2/sftp_connection.h
                       include/sftp_file_transfer/libssh2/sftp_file_transfer_impl.h
)

set(SRC_Cpp src/sftp_connection.cpp
            src/sftp_file_transfer_impl.cpp
)

add_library(${MODULE_NAME} STATIC ${SRC_Cpp} ${SRC_PublicHeaders} ${SRC_PrivateHeaders})
add_library(${SDK_TARGET_NAMESPACE}::${BASE_NAME} ALIAS ${MODULE_NAME})

# Public: sftp_file_transfer/libssh2/sftp_file_transfer_libssh2_factory.h. The other headers next to it are internal
# to this library (they need libssh2 headers, which are not propagated).
target_include_directories(${MODULE_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# OpenSSL is libssh2's crypto backend anyway; used directly only for base64 of host key fingerprints.
find_package(OpenSSL REQUIRED COMPONENTS Crypto)

target_link_libraries(${MODULE_NAME}
    PUBLIC
        daq::sftp_file_transfer_common
    PRIVATE
        libssh2::libssh2
        OpenSSL::Crypto
        $<$<PLATFORM_ID:Windows>:ws2_32>
)
