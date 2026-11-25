# lt_server_stress: loops the tests of the Modern LT modules and the integration suites that use the LT server until a
# deadline (stress.py)
if (NOT Python3_EXECUTABLE AND Python_EXECUTABLE)
    set(Python3_EXECUTABLE ${Python_EXECUTABLE})
endif()
find_package(Python3 REQUIRED COMPONENTS Interpreter)

# Counted from configure, so the build is inside the budget and only the setup before it is not
set(OPENDAQ_LT_SERVER_STRESS_MINUTES 300 CACHE STRING "Minutes from configure after which the stress test starts no run")
string(TIMESTAMP configureTime "%s" UTC)
math(EXPR deadline "${configureTime} + ${OPENDAQ_LT_SERVER_STRESS_MINUTES} * 60")

set(suites "")
if (TARGET test_ws_stream_srv_module)
    list(APPEND suites "$<TARGET_FILE:test_ws_stream_srv_module>=*")
endif()
if (TARGET test_lt_streaming_tls_integration)
    list(APPEND suites "$<TARGET_FILE:test_lt_streaming_tls_integration>=*")
endif()
if (TARGET test_ws_stream_cl_module)
    list(APPEND suites "$<TARGET_FILE:test_ws_stream_cl_module>=*")
endif()
if (TARGET test_device_modules)
    list(APPEND suites "$<TARGET_FILE:test_device_modules>=*Websocket*")
endif()
if (TARGET test_device_modules_streaming)
    list(APPEND suites "$<TARGET_FILE:test_device_modules_streaming>=*_LT/*:*ModernLT*")
endif()

add_test(NAME lt_server_stress
         COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/stress.py --deadline ${deadline} ${suites}
)
math(EXPR timeout "${OPENDAQ_LT_SERVER_STRESS_MINUTES} * 60")
set_tests_properties(lt_server_stress PROPERTIES TIMEOUT ${timeout})
