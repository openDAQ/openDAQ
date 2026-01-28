set(GMOCK_LIB_NAME types_gmocks)

set(GMOCK_HEADERS mock_ptr.h
)

opendaq_prepend_path(include/coretypes/gmock GMOCK_HEADERS)
source_group("gmock" FILES ${GMOCK_HEADERS})

set(GMOCK_DUMMY_SOURCES dummy.cpp
)

source_group("gmock" FILES ${GMOCK_HEADERS}
                           ${GMOCK_SOURCES}
)

add_library(${GMOCK_LIB_NAME} STATIC ${GMOCK_HEADERS} ${GMOCK_DUMMY_SOURCES})
add_library(${SDK_TARGET_NAMESPACE}::${GMOCK_LIB_NAME} ALIAS ${GMOCK_LIB_NAME})

target_link_libraries(${GMOCK_LIB_NAME} PRIVATE daq::coretypes
                                                gmock
)

target_include_directories(${GMOCK_LIB_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

