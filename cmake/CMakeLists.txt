set(CMAKE_FOLDER "cmakw")

list(APPEND CMAKE_MESSAGE_CONTEXT cmake)
set(CURR_MESSAGE_CONTEXT ${CMAKE_MESSAGE_CONTEXT})

message(STATUS "Import functions and macro from opendaq_cmake_utils repo")
include(FetchContent)
FetchContent_Declare(
    opendaq_cmake_utils
    GIT_REPOSITORY file:///home/nick/dewesoft/blueberry_sdk/opendaq_cmake_utils.git
    GIT_TAG main
    GIT_PROGRESS   ON
)
FetchContent_MakeAvailable(opendaq_cmake_utils)
