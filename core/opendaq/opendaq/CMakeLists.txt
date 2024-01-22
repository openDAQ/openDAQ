cmake_minimum_required(VERSION 3.5)
get_current_folder_name(TARGET_FOLDER_NAME)
list(APPEND CMAKE_MESSAGE_CONTEXT ${TARGET_FOLDER_NAME})

project(${SDK_NAME} VERSION ${OPENDAQ_CORE_VERSION} LANGUAGES CXX)

add_subdirectory(src)

if (OPENDAQ_ENABLE_TESTS)
    add_subdirectory(mocks)
    add_subdirectory(tests)
endif()
