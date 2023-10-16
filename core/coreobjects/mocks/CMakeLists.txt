set(MOCK_LIB_NAME objects_mocks)

set(MOCK_HEADERS 
)

opendaq_prepend_path(include/coreobjects/mock MOCK_HEADERS)

set(MOCK_SOURCES dummy.cpp
)

source_group("mock" FILES ${MOCK_HEADERS}
                          ${MOCK_SOURCES}
)

set(GMOCK_HEADERS
)

opendaq_prepend_path(include/coreobjects/gmock GMOCK_HEADERS)
source_group("gmock" FILES ${GMOCK_HEADERS})

add_library(${MOCK_LIB_NAME} STATIC ${GMOCK_HEADERS} ${MOCK_HEADERS} ${MOCK_SOURCES})
add_library(${SDK_TARGET_NAMESPACE}::${MOCK_LIB_NAME} ALIAS ${MOCK_LIB_NAME})

target_link_libraries(${MOCK_LIB_NAME} PRIVATE daq::coreobjects
                                               daq::test_utils
                                       PUBLIC  daq::types_mocks
)

target_include_directories(${MOCK_LIB_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

