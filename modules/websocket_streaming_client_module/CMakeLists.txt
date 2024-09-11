cmake_minimum_required(VERSION 3.5)
set_cmake_folder_context(TARGET_FOLDER_NAME)
project(WebsocketStreamingClientModule VERSION 3.2.8 LANGUAGES C CXX)

add_subdirectory(src)

if (OPENDAQ_ENABLE_TESTS)
    add_subdirectory(tests)
endif()
