set_cmake_folder_context(TARGET_FOLDER_NAME)
set(MODULE_NAME scheduler)

set(TEST_HEADERS test_scheduler.h
)

set(TEST_SOURCES test_awaitable.cpp
                 test_scheduler.cpp
                 test_scheduler_st.cpp
                 test_scheduler_mt.cpp
                 test_task.cpp
                 test_work.cpp
)

opendaq_prepare_test_runner(TEST_APP FOR ${MODULE_NAME}
                       SOURCES
                           test_app.cpp
                           ${TEST_SOURCES}
                           ${TEST_HEADERS}
)

##############################
# LIB test

set(TEST_SOURCES_INTERNAL test_scheduler_internals.cpp
                          test_task_internals.cpp
)

opendaq_prepare_internal_runner(TEST_APP_INTERNAL FOR ${MODULE_NAME}
                           SOURCES
                               test_app.cpp
                               ${TEST_SOURCES_INTERNAL}
)

if (MSVC)
    target_compile_options(${TEST_APP_INTERNAL} PRIVATE /wd4324)
endif()

target_link_libraries(${TEST_APP_INTERNAL} PRIVATE 
    $<BUILD_INTERFACE:Taskflow::Taskflow>
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
