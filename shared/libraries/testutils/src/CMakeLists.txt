set(MODULE_NAME test_utils)

set(SRC_Headers bb_memcheck_listener.h
                testutils.h
                memcheck_listener.h
                ut_logging.h
)

set(SRC_Cpp bb_memcheck_listener.cpp
            memcheck_listener.cpp
)

prepend_include(testutils SRC_Headers)

add_library(${MODULE_NAME} STATIC ${SRC_Headers}
                                  ${SRC_Cpp}
)

add_library(${SDK_TARGET_NAMESPACE}::${MODULE_NAME} ALIAS ${MODULE_NAME})

target_include_directories(${MODULE_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_compile_definitions(${MODULE_NAME} PUBLIC ${OPENDAQ_TEST_COMPILE_DEFINES}
)

target_compile_features(${MODULE_NAME} INTERFACE cxx_std_17)

target_link_libraries(${MODULE_NAME} PUBLIC daq::coretypes
                                            testlibs
)


