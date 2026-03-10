set(GMOCK_LIB_NAME objects_gmocks)

set(GMOCK_HEADERS 
)

opendaq_prepend_path(include/coreobjects/gmock GMOCK_HEADERS)

set(GMOCK_DUMMY_SOURCES dummy.cpp
)

opendaq_prepend_path(include/coreobjects/gmock GMOCK_HEADERS)
source_group("gmock" FILES ${GMOCK_HEADERS})

add_library(${GMOCK_LIB_NAME} STATIC ${GMOCK_HEADERS} ${GMOCK_DUMMY_SOURCES})
add_library(${SDK_TARGET_NAMESPACE}::${GMOCK_LIB_NAME} ALIAS ${GMOCK_LIB_NAME})

target_link_libraries(${GMOCK_LIB_NAME} PRIVATE daq::coreobjects
                                                gmock
                                        PUBLIC  daq::types_gmocks
)

target_include_directories(${GMOCK_LIB_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

