project(test_tmp CXX)
set(MODULE_NAME test_app)

set(SRC_Cpp main.cpp
)

add_executable(${MODULE_NAME} ${SRC_Cpp}
)

target_include_directories(${MODULE_NAME}
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include/>
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(${MODULE_NAME} daq::opendaq)

add_dependencies(${MODULE_NAME}
        daq::opcua_client_module
        daq::opcua_server_module
)
