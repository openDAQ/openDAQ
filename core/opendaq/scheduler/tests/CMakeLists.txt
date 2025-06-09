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
                 test_scheduler_internals.cpp
                 test_task_internals.cpp
)

opendaq_prepare_test_runner(TEST_APP FOR ${MODULE_NAME}
                       SOURCES
                           test_app.cpp
                           ${TEST_SOURCES}
                           ${TEST_HEADERS}
)

target_link_libraries(${TEST_APP} PRIVATE 
    $<BUILD_INTERFACE:Taskflow::Taskflow>
)

if (MSVC)
    target_compile_options(${TEST_APP} PRIVATE /wd4324)
endif()

##############################
# CTest setup

add_test(NAME ${TEST_APP}
         COMMAND $<TARGET_FILE_NAME:${TEST_APP}>
         WORKING_DIRECTORY $<TARGET_FILE_DIR:${TEST_APP}>
)

if (OPENDAQ_ENABLE_COVERAGE)
    setup_target_for_coverage(${TEST_APP}coverage ${TEST_APP} ${TEST_APP}coverage)
endif()
