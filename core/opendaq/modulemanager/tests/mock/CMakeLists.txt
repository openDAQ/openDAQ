opendaq_set_cmake_folder_context(TARGET_FOLDER_NAME)

add_library(empty_dll SHARED empty.cpp)

add_library(crashing_module SHARED crashes.cpp)
target_link_libraries(crashing_module PRIVATE ${SDK_TARGET_NAMESPACE}::${MAIN_TARGET})
target_compile_definitions(crashing_module PRIVATE BUILDING_SHARED_LIBRARY)

add_library(dependencies_failed SHARED dependencies_failed.cpp)
target_link_libraries(dependencies_failed PRIVATE ${SDK_TARGET_NAMESPACE}::${MAIN_TARGET})
target_compile_definitions(dependencies_failed PRIVATE BUILDING_SHARED_LIBRARY)

add_library(dependencies_succeeded SHARED dependencies_succeeded.cpp
                                          mock_module.cpp
                                          mock_module.h)
										  
target_link_libraries(dependencies_succeeded PRIVATE ${SDK_TARGET_NAMESPACE}::${MAIN_TARGET})
target_compile_definitions(dependencies_succeeded PRIVATE BUILDING_SHARED_LIBRARY)

if (MSVC)
    target_compile_options(dependencies_succeeded PRIVATE /wd4100)
endif()

add_library(null_string_id_module SHARED null_string_id_module.cpp
                                         mock_module.cpp
                                         mock_module.h)

target_link_libraries(null_string_id_module PRIVATE ${SDK_TARGET_NAMESPACE}::${MAIN_TARGET})
target_compile_definitions(null_string_id_module PRIVATE BUILDING_SHARED_LIBRARY)

if (MSVC)
    target_compile_options(null_string_id_module PRIVATE /wd4100)
endif()

add_library(empty_string_id_module SHARED empty_string_id_module.cpp
                                          mock_module.cpp
                                          mock_module.h)

target_link_libraries(empty_string_id_module PRIVATE ${SDK_TARGET_NAMESPACE}::${MAIN_TARGET})
target_compile_definitions(empty_string_id_module PRIVATE BUILDING_SHARED_LIBRARY)

if (MSVC)
    target_compile_options(empty_string_id_module PRIVATE /wd4100)
endif()

set_target_properties(
    empty_dll
    crashing_module
    dependencies_failed
    dependencies_succeeded
    null_string_id_module
    empty_string_id_module
	
    PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:${TEST_APP}>/mock
        RUNTIME_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:${TEST_APP}>/mock
        DEBUG_POSTFIX ""
        SUFFIX ${OPENDAQ_MODULE_SUFFIX}
)

add_custom_command(TARGET dependencies_succeeded POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:dependencies_succeeded>
        $<TARGET_FILE_DIR:dependencies_succeeded>/copy_$<TARGET_FILE_NAME:dependencies_succeeded>
)
