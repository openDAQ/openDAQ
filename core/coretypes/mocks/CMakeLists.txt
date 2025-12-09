set(MOCK_LIB_NAME types_mocks)

set(MOCK_HEADERS 
)

opendaq_prepend_path(mocks MOCK_HEADERS)

set(MOCK_SOURCES dummy.cpp
)

source_group("mock" FILES ${MOCK_HEADERS}
                           ${MOCK_SOURCES}
)

set(GMOCK_HEADERS mock_ptr.h
)

opendaq_prepend_path(include/coretypes/gmock GMOCK_HEADERS)
source_group("gmock" FILES ${GMOCK_HEADERS})

add_library(${MOCK_LIB_NAME} STATIC ${MOCK_HEADERS} ${MOCK_SOURCES} ${GMOCK_HEADERS})
add_library(${SDK_TARGET_NAMESPACE}::${MOCK_LIB_NAME} ALIAS ${MOCK_LIB_NAME})

target_link_libraries(${MOCK_LIB_NAME} PRIVATE daq::coretypes
                                               daq::test_utils
)

target_include_directories(${MOCK_LIB_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

