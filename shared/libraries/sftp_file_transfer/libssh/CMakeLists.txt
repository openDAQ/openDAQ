set(BASE_NAME sftp_file_transfer_libssh)
set(MODULE_NAME ${SDK_TARGET_NAME}_${BASE_NAME})

set(SRC_PublicHeaders include/sftp_file_transfer/libssh/sftp_file_transfer_libssh_factory.h
)

set(SRC_PrivateHeaders include/sftp_file_transfer/libssh/sftp_connection.h
                       include/sftp_file_transfer/libssh/sftp_file_transfer_impl.h
)

set(SRC_Cpp src/sftp_connection.cpp
            src/sftp_file_transfer_impl.cpp
)

add_library(${MODULE_NAME} STATIC ${SRC_Cpp} ${SRC_PublicHeaders} ${SRC_PrivateHeaders})
add_library(${SDK_TARGET_NAMESPACE}::${BASE_NAME} ALIAS ${MODULE_NAME})

# Public: sftp_file_transfer/libssh/sftp_file_transfer_libssh_factory.h. The other headers next to it are internal
# to this library (they need libssh headers, which are not propagated).
target_include_directories(${MODULE_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# libssh is a shared library (LGPL-2.1, see external/libssh); it opens its own sockets, so no socket library here.
target_link_libraries(${MODULE_NAME}
    PUBLIC
        daq::sftp_file_transfer_common
    PRIVATE
        ssh
)
