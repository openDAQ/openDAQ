set(PYTHON_TESTS_COMMAND_ARGS -m unittest discover ${CMAKE_CURRENT_SOURCE_DIR})
set(PYTHON_TESTS test_py_opendaq)

set(PYHTON_TEST_SRC
    opendaq_test.py
    test_architecture.py
    test_opendaq_device.py
    test_opendaq_instance.py
    test_core_types.py
    test_core_type_integer.py
    test_core_type_string.py
    test_doc_quick_start.py
    test_how_to.py
    test_how_to_add_function_block.py
    test_reader_datetime.py
    test_property_system.py
)

option(OPENDAQ_VENV_PYTHON_EXECUTABLE_TESTS "Use python executable from virtual environment for tests" OFF)
set(PYTHON_EXECUTABLE ${Python_EXECUTABLE})

if(OPENDAQ_VENV_PYTHON_EXECUTABLE_TESTS)
    set(PYTHON_EXECUTABLE python)
endif()

message(STATUS "Python executable for tests: ${PYTHON_EXECUTABLE}")

add_custom_target(
    ${PYTHON_TESTS}
    COMMAND ${PYTHON_EXECUTABLE} ${PYTHON_TESTS_COMMAND_ARGS}
    USES_TERMINAL
    COMMENT "Running python bindings tests"
    SOURCES ${PYHTON_TEST_SRC}
    WORKING_DIRECTORY $<TARGET_FILE_DIR:py_opendaq_daq>
)

list(JOIN PYTHON_TESTS_COMMAND_ARGS " " PYTHON_TESTS_COMMAND_ARGS_STRING)

set_target_properties(
    ${PYTHON_TESTS}
    PROPERTIES
        VS_DEBUGGER_COMMAND ${PYTHON_EXECUTABLE}
        VS_DEBUGGER_COMMAND_ARGUMENTS ${PYTHON_TESTS_COMMAND_ARGS_STRING}
        VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:py_opendaq_daq>
)

add_test(
    NAME ${PYTHON_TESTS}
    COMMAND ${PYTHON_EXECUTABLE} -m unittest discover ${CMAKE_CURRENT_SOURCE_DIR}
    WORKING_DIRECTORY $<TARGET_FILE_DIR:py_opendaq_daq>
)

set_tests_properties(test_py_opendaq PROPERTIES LABELS "python_bindings")
