set(MODULE_NAME discovery_server)

set(SRC_Headers
    common.h
    mdnsdiscovery_server.h
)

prepend_include(${TARGET_FOLDER_NAME} SRC_Headers)

set(SRC_Cpp
    mdnsdiscovery_server.cpp
)

add_library(${MODULE_NAME} STATIC ${SRC_Cpp}
                                  ${SRC_Headers}
)

add_library(daq::${MODULE_NAME} ALIAS ${MODULE_NAME})
add_library(daq::${MODULE_NAME}_dev ALIAS ${MODULE_NAME})

find_package(Threads REQUIRED)


target_include_directories(${MODULE_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include>

        $<INSTALL_INTERFACE:include>
)

target_link_libraries(${MODULE_NAME} 
    PUBLIC 
        mdns::mdns
        daq::discovery_common
        daq::coreobjects
)


#opendaq_set_output_lib_name(${MODULE_NAME} ${PROJECT_VERSION_MAJOR})
