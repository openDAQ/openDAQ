
add_executable(test_copendaq_component  ${CMAKE_CURRENT_LIST_DIR}/../test_app.cpp
    test_copendaq_component.cpp
)

add_executable(test_copendaq_context  ${CMAKE_CURRENT_LIST_DIR}/../test_app.cpp
    test_copendaq_context.cpp
)

add_executable(test_copendaq_device  ${CMAKE_CURRENT_LIST_DIR}/../test_app.cpp
    test_copendaq_device.cpp
)

add_executable(test_copendaq_functionblock  ${CMAKE_CURRENT_LIST_DIR}/../test_app.cpp
    test_copendaq_functionblock.cpp
)

add_executable(test_copendaq_logger  ${CMAKE_CURRENT_LIST_DIR}/../test_app.cpp
    test_copendaq_logger.cpp
)

add_executable(test_copendaq_modulemanager  ${CMAKE_CURRENT_LIST_DIR}/../test_app.cpp
    test_copendaq_modulemanager.cpp
)

add_executable(test_copendaq_opendaq  ${CMAKE_CURRENT_LIST_DIR}/../test_app.cpp
    test_copendaq_opendaq.cpp
)

add_executable(test_copendaq_reader  ${CMAKE_CURRENT_LIST_DIR}/../test_app.cpp
    test_copendaq_reader.cpp
)

add_executable(test_copendaq_scheduler  ${CMAKE_CURRENT_LIST_DIR}/../test_app.cpp
    test_copendaq_scheduler.cpp
)

add_executable(test_copendaq_server  ${CMAKE_CURRENT_LIST_DIR}/../test_app.cpp
    test_copendaq_server.cpp
)

add_executable(test_copendaq_signal  ${CMAKE_CURRENT_LIST_DIR}/../test_app.cpp
    test_copendaq_signal.cpp
)

add_executable(test_copendaq_streaming  ${CMAKE_CURRENT_LIST_DIR}/../test_app.cpp
    test_copendaq_streaming.cpp
)

add_executable(test_copendaq_synchronization  ${CMAKE_CURRENT_LIST_DIR}/../test_app.cpp
    test_copendaq_synchronization.cpp
)

target_compile_definitions(test_copendaq_component PRIVATE ${OPENDAQ_TEST_COMPILE_DEFINES})
target_compile_definitions(test_copendaq_context PRIVATE ${OPENDAQ_TEST_COMPILE_DEFINES})
target_compile_definitions(test_copendaq_device PRIVATE ${OPENDAQ_TEST_COMPILE_DEFINES})
target_compile_definitions(test_copendaq_functionblock PRIVATE ${OPENDAQ_TEST_COMPILE_DEFINES})
target_compile_definitions(test_copendaq_logger PRIVATE ${OPENDAQ_TEST_COMPILE_DEFINES})
target_compile_definitions(test_copendaq_modulemanager PRIVATE ${OPENDAQ_TEST_COMPILE_DEFINES})
target_compile_definitions(test_copendaq_opendaq PRIVATE ${OPENDAQ_TEST_COMPILE_DEFINES})
target_compile_definitions(test_copendaq_reader PRIVATE ${OPENDAQ_TEST_COMPILE_DEFINES})
target_compile_definitions(test_copendaq_scheduler PRIVATE ${OPENDAQ_TEST_COMPILE_DEFINES})
target_compile_definitions(test_copendaq_server PRIVATE ${OPENDAQ_TEST_COMPILE_DEFINES})
target_compile_definitions(test_copendaq_signal PRIVATE ${OPENDAQ_TEST_COMPILE_DEFINES})
target_compile_definitions(test_copendaq_streaming PRIVATE ${OPENDAQ_TEST_COMPILE_DEFINES})
target_compile_definitions(test_copendaq_synchronization PRIVATE ${OPENDAQ_TEST_COMPILE_DEFINES})

set_target_properties(test_copendaq_component PROPERTIES DEBUG_POSTFIX _debug)
set_target_properties(test_copendaq_context PROPERTIES DEBUG_POSTFIX _debug)
set_target_properties(test_copendaq_device PROPERTIES DEBUG_POSTFIX _debug)
set_target_properties(test_copendaq_functionblock PROPERTIES DEBUG_POSTFIX _debug)
set_target_properties(test_copendaq_logger PROPERTIES DEBUG_POSTFIX _debug)
set_target_properties(test_copendaq_modulemanager PROPERTIES DEBUG_POSTFIX _debug)
set_target_properties(test_copendaq_opendaq PROPERTIES DEBUG_POSTFIX _debug)
set_target_properties(test_copendaq_reader PROPERTIES DEBUG_POSTFIX _debug)
set_target_properties(test_copendaq_scheduler PROPERTIES DEBUG_POSTFIX _debug)
set_target_properties(test_copendaq_server PROPERTIES DEBUG_POSTFIX _debug)
set_target_properties(test_copendaq_signal PROPERTIES DEBUG_POSTFIX _debug)
set_target_properties(test_copendaq_streaming PROPERTIES DEBUG_POSTFIX _debug)
set_target_properties(test_copendaq_synchronization PROPERTIES DEBUG_POSTFIX _debug)

target_link_libraries(test_copendaq_component PRIVATE daq::copendaq daq::test_utils)
target_link_libraries(test_copendaq_context PRIVATE daq::copendaq daq::test_utils)
target_link_libraries(test_copendaq_device PRIVATE daq::copendaq daq::test_utils)
target_link_libraries(test_copendaq_functionblock PRIVATE daq::copendaq daq::test_utils)
target_link_libraries(test_copendaq_logger PRIVATE daq::copendaq daq::test_utils)
target_link_libraries(test_copendaq_modulemanager PRIVATE daq::copendaq daq::test_utils)
target_link_libraries(test_copendaq_opendaq PRIVATE daq::copendaq daq::test_utils)
target_link_libraries(test_copendaq_reader PRIVATE daq::copendaq daq::test_utils)
target_link_libraries(test_copendaq_scheduler PRIVATE daq::copendaq daq::test_utils)
target_link_libraries(test_copendaq_server PRIVATE daq::copendaq daq::test_utils)
target_link_libraries(test_copendaq_signal PRIVATE daq::copendaq daq::test_utils)
target_link_libraries(test_copendaq_streaming PRIVATE daq::copendaq daq::test_utils)
target_link_libraries(test_copendaq_synchronization PRIVATE daq::copendaq daq::test_utils)

add_test(NAME test_copendaq_component COMMAND $<TARGET_FILE_NAME:test_copendaq_component> WORKING_DIRECTORY bin)
add_test(NAME test_copendaq_context COMMAND $<TARGET_FILE_NAME:test_copendaq_context> WORKING_DIRECTORY bin)
add_test(NAME test_copendaq_device COMMAND $<TARGET_FILE_NAME:test_copendaq_device> WORKING_DIRECTORY bin)
add_test(NAME test_copendaq_functionblock COMMAND $<TARGET_FILE_NAME:test_copendaq_functionblock> WORKING_DIRECTORY bin)
add_test(NAME test_copendaq_logger COMMAND $<TARGET_FILE_NAME:test_copendaq_logger> WORKING_DIRECTORY bin)
add_test(NAME test_copendaq_modulemanager COMMAND $<TARGET_FILE_NAME:test_copendaq_modulemanager> WORKING_DIRECTORY bin)
add_test(NAME test_copendaq_opendaq COMMAND $<TARGET_FILE_NAME:test_copendaq_opendaq> WORKING_DIRECTORY bin)
add_test(NAME test_copendaq_reader COMMAND $<TARGET_FILE_NAME:test_copendaq_reader> WORKING_DIRECTORY bin)
add_test(NAME test_copendaq_scheduler COMMAND $<TARGET_FILE_NAME:test_copendaq_scheduler> WORKING_DIRECTORY bin)
add_test(NAME test_copendaq_server COMMAND $<TARGET_FILE_NAME:test_copendaq_server> WORKING_DIRECTORY bin)
add_test(NAME test_copendaq_signal COMMAND $<TARGET_FILE_NAME:test_copendaq_signal> WORKING_DIRECTORY bin)
add_test(NAME test_copendaq_streaming COMMAND $<TARGET_FILE_NAME:test_copendaq_streaming> WORKING_DIRECTORY bin)
add_test(NAME test_copendaq_synchronization COMMAND $<TARGET_FILE_NAME:test_copendaq_synchronization> WORKING_DIRECTORY bin)
