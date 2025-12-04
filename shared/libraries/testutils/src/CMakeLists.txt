set(MODULE_NAME test_utils)

set(SRC_Headers bb_memcheck_listener.h
                testutils.h
                memcheck_listener.h
                ut_logging.h
                test_comparators.h
                base_test_listener.h
)

prepend_include(testutils SRC_Headers)

add_library(${MODULE_NAME} INTERFACE)
add_library(${SDK_TARGET_NAMESPACE}::${MODULE_NAME} ALIAS ${MODULE_NAME})

target_include_directories(${MODULE_NAME}
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<INSTALL_INTERFACE:include/testutils>
)

target_compile_definitions(${MODULE_NAME} INTERFACE ${OPENDAQ_TEST_COMPILE_DEFINES})

target_compile_features(${MODULE_NAME} INTERFACE cxx_std_17)

target_link_libraries(${MODULE_NAME} INTERFACE daq::coretypes
                                               daq::coreobjects
                                               daq::opendaq
)

install(TARGETS ${MODULE_NAME}
        EXPORT ${SDK_NAME}
)

install(FILES ${SRC_Headers}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/testutils
)
