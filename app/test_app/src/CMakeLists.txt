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

if (WIN32)
    set(BCRYPT_LIB bcrypt.dll)
endif()

target_link_libraries(${MODULE_NAME}
    PUBLIC
        daq::opcuatms_server
        daq::opcuatms_client
        daq::signal
        daq::opcuatms
        daq::opendaq_mocks
        ${BCRYPT_LIB}
)
