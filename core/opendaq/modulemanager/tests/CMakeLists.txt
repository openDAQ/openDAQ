set_cmake_folder_context(TARGET_FOLDER_NAME)
set(MODULE_NAME module_manager)

##############################
# DLL Test

set(TEST_MOCKS mock/mock_module.h
               mock/mock_module.cpp
)

set(TEST_SOURCES test_check_dependencies.cpp
                 module_manager_test.cpp
)


opendaq_prepare_test_runner(TEST_APP FOR ${MODULE_NAME}
                       SOURCES
                           test_app.cpp
                           ${TEST_MOCKS}
                           ${TEST_SOURCES}
)

target_compile_definitions(${TEST_APP} PRIVATE MODULE_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}/test"
)

##############################
# LIB test

add_subdirectory(mock)

set(TEST_SOURCES_INTERNAL test_module_manager_internals.cpp
)

opendaq_prepare_internal_runner(TEST_APP_INTERNAL FOR ${MODULE_NAME}
                           SOURCES
                               test_app.cpp
                               ${TEST_SOURCES_INTERNAL}
)

target_compile_definitions(${TEST_APP_INTERNAL} PRIVATE MODULE_TEST_DIR="$<TARGET_FILE_DIR:empty_dll>"
                                                        BOOST_DLL_USE_STD_FS
                                                        EMPTY_MODULE_FILE_NAME="$<TARGET_FILE_NAME:empty_dll>"
                                                        CRASHING_MODULE_FILE_NAME="$<TARGET_FILE_NAME:crashing_module>"
                                                        DEPENDENCIES_FAILED_MODULE_NAME="$<TARGET_FILE_NAME:dependencies_failed>"
                                                        DEPENDENCIES_SUCCEEDED_MODULE_NAME="$<TARGET_FILE_NAME:dependencies_succeeded>"
)

target_link_libraries(${TEST_APP_INTERNAL} PRIVATE daq::device
                                                   daq::function_block
                                                   ${Boost_LIBRARIES}
)

target_link_libraries(${TEST_APP} PRIVATE daq::coreobjects
)

# we shouldn't link everything, only the necessary bits Boost::dll
target_link_libraries(${TEST_APP_INTERNAL} PRIVATE $<BUILD_INTERFACE:Boost::dll>)

# Do not link to these targets only make sure they are built
add_dependencies(${TEST_APP_INTERNAL} empty_dll
                                      crashing_module
                                      dependencies_failed
                                      dependencies_succeeded
)

##############################
# CTest setup

add_test(NAME ${TEST_APP}
         COMMAND $<TARGET_FILE_NAME:${TEST_APP}>
         WORKING_DIRECTORY bin
)

add_test(NAME ${TEST_APP_INTERNAL}
        COMMAND $<TARGET_FILE_NAME:${TEST_APP_INTERNAL}>
        WORKING_DIRECTORY bin
)

if (OPENDAQ_ENABLE_COVERAGE)
    setup_target_for_coverage(${TEST_APP}coverage ${TEST_APP} ${TEST_APP}coverage)
endif()
