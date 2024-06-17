set_cmake_folder_context(TARGET_FOLDER_NAME)

add_library(empty_dll SHARED empty.cpp)

add_library(crashing_module SHARED crashes.cpp)
target_link_libraries(crashing_module PRIVATE ${SDK_TARGET_NAMESPACE}::${MAIN_TARGET})
target_compile_definitions(crashing_module PRIVATE BUILDING_SHARED_LIBRARY)

add_library(dependencies_failed SHARED dependencies_failed.cpp)
target_link_libraries(dependencies_failed PRIVATE ${SDK_TARGET_NAMESPACE}::${MAIN_TARGET})
target_compile_definitions(dependencies_failed PRIVATE BUILDING_SHARED_LIBRARY)

add_library(dependencies_succeeded SHARED dependencies_succeeded.cpp
                                          mock_module.cpp
                                          mock_module.h
)
target_link_libraries(dependencies_succeeded PRIVATE ${SDK_TARGET_NAMESPACE}::${MAIN_TARGET})
target_compile_definitions(dependencies_succeeded PRIVATE BUILDING_SHARED_LIBRARY)

if (MSVC)
    target_compile_options(dependencies_succeeded PRIVATE /wd4100)
endif()

set_target_properties(
    empty_dll
    crashing_module
    dependencies_failed
    dependencies_succeeded

    PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:${TEST_APP}>/mock
        RUNTIME_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:${TEST_APP}>/mock
        DEBUG_POSTFIX ""
        SUFFIX ${OPENDAQ_MODULE_SUFFIX}
)
