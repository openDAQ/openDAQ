set_cmake_folder_context(TARGET_FOLDER_NAME)
set(MODULE_NAME module_manager)

set(TEST_MOCKS mock/mock_module.h
               mock/mock_module.cpp
)

##############################
# DLL Test


set(TEST_SOURCES test_check_dependencies.cpp
                 module_manager_test.cpp
)

opendaq_prepare_test_runner(TEST_APP FOR ${MODULE_NAME}
                       SOURCES
                           test_app.cpp
						   test_module_manager_internals.cpp
                           ${TEST_MOCKS}
                           ${TEST_SOURCES}
)

target_compile_definitions(${TEST_APP} PRIVATE MODULE_TEST_DIR="mock"
                                               BOOST_DLL_USE_STD_FS
                                               BOOST_ALL_NO_LIB
                                               EMPTY_MODULE_FILE_NAME="$<TARGET_FILE_NAME:empty_dll>"
                                               CRASHING_MODULE_FILE_NAME="$<TARGET_FILE_NAME:crashing_module>"
                                               DEPENDENCIES_FAILED_MODULE_NAME="$<TARGET_FILE_NAME:dependencies_failed>"
                                               DEPENDENCIES_SUCCEEDED_MODULE_NAME="$<TARGET_FILE_NAME:dependencies_succeeded>"
)

target_link_libraries(${TEST_APP} 
    PRIVATE 
        $<BUILD_INTERFACE:Boost::dll>
)

set_target_properties(${TEST_APP}
    PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:${TEST_APP}>
)


add_subdirectory(mock)

# Do not link to these targets only make sure they are built
add_dependencies(${TEST_APP} empty_dll
                             crashing_module
                             dependencies_failed
                             dependencies_succeeded
)

set_target_properties(${TEST_APP}
    PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:${TEST_APP}>
)

##############################
# CTest setup



add_test(NAME ${TEST_APP}
         COMMAND $<TARGET_FILE_NAME:${TEST_APP}>
         WORKING_DIRECTORY $<TARGET_FILE_DIR:${TEST_APP}>
)

if (OPENDAQ_ENABLE_COVERAGE)
    setup_target_for_coverage(${TEST_APP}coverage ${TEST_APP} ${TEST_APP}coverage)
endif()
