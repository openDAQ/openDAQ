set_cmake_folder_context(TARGET_FOLDER_NAME)
set(MODULE_NAME utility)

##############################
# DLL Test

set(TEST_HEADERS
)

set(TEST_SOURCES
)

opendaq_prepare_test_runner(TEST_APP FOR ${MODULE_NAME}
                       SOURCES
                           test_app.cpp
                           ${TEST_SOURCES}
                           ${TEST_HEADERS}
)

##############################
# LIB test

set(TEST_HEADERS_INTERNAL
)

set(TEST_SOURCES_INTERNAL test_ids_parser.cpp
)

opendaq_prepare_internal_runner(TEST_APP_INTERNAL FOR ${MODULE_NAME}
                       SOURCES
                           test_app.cpp
                           ${TEST_SOURCES_INTERNAL}
                           ${TEST_HEADERS_INTERNAL}
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
