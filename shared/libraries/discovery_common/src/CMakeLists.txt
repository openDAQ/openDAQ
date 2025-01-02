set(BASE_NAME discovery_common)
set(MODULE_NAME ${SDK_TARGET_NAME}_${BASE_NAME})

set(SRC_HEADERS daq_discovery_common.h)

set(SRC_CPPS daq_discovery_common.cpp)

prepend_include(discovery_common SRC_HEADERS)

add_library(${MODULE_NAME} STATIC ${SRC_HEADERS} ${SRC_CPPS})
add_library(${SDK_TARGET_NAMESPACE}::${BASE_NAME} ALIAS ${MODULE_NAME})


target_include_directories(${MODULE_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)

target_link_libraries(${MODULE_NAME} PUBLIC mdns::mdns
                                            daq::coreobjects
)
