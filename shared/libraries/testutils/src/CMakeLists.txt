set(SRC_CommonHeaders daq_memcheck_listener.h
                      testutils.h
                      memcheck_listener.h
                      ut_logging.h
                      test_comparators.h
)
prepend_include(testutils SRC_CommonHeaders)

set(LIB_NAME test_utils)

set(SRC_Cpp base_test_listener.cpp)

set(SRC_InternalHeaders base_test_listener.h)
prepend_include(internal/testutils SRC_InternalHeaders)

add_library(${LIB_NAME} STATIC ${SRC_CommonHeaders}
                               ${SRC_InternalHeaders}
                               ${SRC_Cpp}
)

add_library(${SDK_TARGET_NAMESPACE}::${LIB_NAME} ALIAS ${LIB_NAME})

target_include_directories(${LIB_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include
                                              ${CMAKE_CURRENT_SOURCE_DIR}/../include/internal
)

target_compile_definitions(${LIB_NAME} PUBLIC ${OPENDAQ_TEST_COMPILE_DEFINES}
)

target_compile_features(${LIB_NAME} INTERFACE cxx_std_17)

target_link_libraries(${LIB_NAME} PUBLIC daq::coretypes
                                         daq::coreobjects
                                         daq::opendaq
                                         testlibs
)

set(INSTALLED_NAME ${SDK_TARGET_NAME}_${LIB_NAME})

set(SRC_InstalledHeaders base_test_listener.h
)
prepend_include(installed/testutils SRC_InstalledHeaders)

add_library(${INSTALLED_NAME} INTERFACE)
add_library(${SDK_TARGET_NAMESPACE}::${INSTALLED_NAME} ALIAS ${INSTALLED_NAME})

target_include_directories(${INSTALLED_NAME}
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include/installed>
        $<INSTALL_INTERFACE:include/testutils>
)

target_compile_definitions(${INSTALLED_NAME} INTERFACE ${OPENDAQ_TEST_COMPILE_DEFINES})

target_compile_features(${INSTALLED_NAME} INTERFACE cxx_std_17)

target_link_libraries(${INSTALLED_NAME} INTERFACE daq::coretypes
                                                  daq::coreobjects
                                                  daq::opendaq
)

install(TARGETS ${INSTALLED_NAME}
        EXPORT ${SDK_NAME}
)

install(FILES ${SRC_CommonHeaders} ${SRC_InstalledHeaders}
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/testutils
)
