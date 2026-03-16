opendaq_set_cmake_folder_context(TARGET_FOLDER_NAME)

function(opendaq_add_test_mock_module TARGET_NAME)
    set(SOURCES ${ARGN}) # Remaining args are source files
    add_library(${TARGET_NAME} SHARED ${SOURCES})
    target_link_libraries(${TARGET_NAME} PRIVATE ${SDK_TARGET_NAMESPACE}::${MAIN_TARGET})
    target_compile_definitions(${TARGET_NAME} PRIVATE BUILDING_SHARED_LIBRARY)
    if (MSVC)
        target_compile_options(${TARGET_NAME} PRIVATE /wd4100)
    endif()
endfunction()

add_library(empty_dll SHARED empty.cpp)
opendaq_add_test_mock_module(crashing_module crashes.cpp)
opendaq_add_test_mock_module(dependencies_failed dependencies_failed.cpp)
opendaq_add_test_mock_module(dependencies_succeeded dependencies_succeeded.cpp mock_module.cpp mock_module.h)
opendaq_add_test_mock_module(null_string_id_module null_string_id_module.cpp mock_module.cpp mock_module.h)
opendaq_add_test_mock_module(empty_string_id_module empty_string_id_module.cpp mock_module.cpp mock_module.h)
opendaq_add_test_mock_module(dependencies_obsolete_failed dependencies_obsolete_failed.cpp)
opendaq_add_test_mock_module(dependencies_obsolete_succeeded dependencies_obsolete_succeeded.cpp mock_module.cpp mock_module.h)
opendaq_add_test_mock_module(sdk_version_mismatch sdk_version_mismatch.cpp mock_module.cpp mock_module.h)
opendaq_add_test_mock_module(sdk_version_match sdk_version_match.cpp mock_module.cpp mock_module.h)

set_target_properties(
    empty_dll
    crashing_module
    dependencies_failed
    dependencies_succeeded
    null_string_id_module
    empty_string_id_module
    dependencies_obsolete_failed
    dependencies_obsolete_succeeded
    sdk_version_mismatch
    sdk_version_match
	
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
