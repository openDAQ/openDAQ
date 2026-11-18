cmake_minimum_required(VERSION 3.24)
opendaq_set_cmake_folder_context(TARGET_FOLDER_NAME)

project(sftp_file_transfer VERSION 1.0.0 LANGUAGES CXX)

# Backend-agnostic part: the IFileTransfer interface, the SFTP authentication config and helpers shared by backends.
add_subdirectory(common)

# One directory, target, namespace and factory per SSH library backend; several can be built at the same time.
if (OPENDAQ_ENABLE_SFTP_FILE_TRANSFER_LIBSSH2)
    add_subdirectory(libssh2)
endif()

if (OPENDAQ_ENABLE_SFTP_FILE_TRANSFER_LIBSSH)
    add_subdirectory(libssh)
endif()
