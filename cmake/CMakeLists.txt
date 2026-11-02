set(CMAKE_FOLDER "cmake")

list(APPEND CMAKE_MESSAGE_CONTEXT cmake)
set(CURR_MESSAGE_CONTEXT ${CMAKE_MESSAGE_CONTEXT})

message(STATUS "Import functions and macro from opendaq-cmake-utils repo")
include(FetchContent)
FetchContent_Declare(
    opendaq-cmake-utils
    GIT_REPOSITORY  https://github.com/openDAQ/opendaq-cmake-utils.git
    GIT_TAG         21c878341fe3c7756b183554f64fe1b0a174e481
    GIT_PROGRESS    ON
)
FetchContent_MakeAvailable(opendaq-cmake-utils)
