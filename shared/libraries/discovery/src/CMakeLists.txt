set(BASE_NAME discovery)
set(MODULE_NAME ${SDK_TARGET_NAME}_${BASE_NAME})

set(SRC_HEADERS common.h
                daq_discovery_client.h
                mdnsdiscovery_client.h
)

set(SRC_CPPS daq_discovery_client.cpp
)

prepend_include(daq_discovery SRC_HEADERS)
#prepend_include(${MODULE_NAME} SRC_HEADERS)

add_library(${MODULE_NAME} STATIC ${SRC_HEADERS} ${SRC_CPPS})
add_library(${SDK_TARGET_NAMESPACE}::${BASE_NAME} ALIAS ${MODULE_NAME})


target_include_directories(${MODULE_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)

target_link_libraries(${MODULE_NAME} PUBLIC mdns::mdns
                                            daq::opendaq
)
