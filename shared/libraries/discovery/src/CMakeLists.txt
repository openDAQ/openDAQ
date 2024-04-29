set(BASE_NAME discovery)
set(MODULE_NAME ${SDK_TARGET_NAME}_${BASE_NAME})

set(SRC_Headers common.h
                daq_discovery_client.h
                mdnsdiscovery_client.h
                daq_discovery_server.h
                mdnsdiscovery_server.h
)

set(SRC_Cpp daq_discovery_client.cpp
            daq_discovery_server.cpp 
)

prepend_include(daq_discovery SRC_Headers)
#prepend_include(${MODULE_NAME} SRC_Headers)

add_library(${MODULE_NAME} STATIC ${SRC_Headers} ${SRC_Cpp})
add_library(${SDK_TARGET_NAMESPACE}::${BASE_NAME} ALIAS ${MODULE_NAME})


target_include_directories(${MODULE_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)

target_link_libraries(${MODULE_NAME} PUBLIC mdns::mdns
                                            daq::coretypes
                                            daq::device
)
