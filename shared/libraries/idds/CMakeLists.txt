cmake_minimum_required(VERSION 3.5)
set_cmake_folder_context(TARGET_FOLDER_NAME ${SDK_TARGET_NAMESPACE}_idds)

project(OpenDaqIDDS
    VERSION   1.0.0
    LANGUAGES CXX
)

add_subdirectory(src)


