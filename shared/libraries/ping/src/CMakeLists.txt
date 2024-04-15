set(MODULE_NAME discovery_ping)

set(SRC_Headers
    ip_v4.h
    ipv4_header.h
    log.h
    common.h
    format.h
    network_interface.h
    network_interface_exception.h

    icmp_ping.h
    icmp_header.h
)

prepend_include(${TARGET_FOLDER_NAME} SRC_Headers)

if (WIN32)
    set(SRC_Cpp_Win
        network_interfaces_win.cpp
    )
else()
    set(SRC_Cpp_Linux
        network_interfaces_linux.cpp
    )
endif()

set(SRC_Cpp
    ip_v4.cpp
    ipv4_header.cpp
    network_interface_exception.cpp
    ${SRC_Cpp_Win}
    ${SRC_Cpp_Linux}

    icmp_ping.cpp
    icmp_header.cpp
)

add_library(${MODULE_NAME} STATIC ${SRC_Cpp}
                                  ${SRC_Headers}
)

add_library(daq::${MODULE_NAME} ALIAS ${MODULE_NAME})
add_library(daq::${MODULE_NAME}_dev ALIAS ${MODULE_NAME})

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

target_link_libraries(${MODULE_NAME}
    PUBLIC
        Threads::Threads
        Boost::headers
        fmt::fmt
        #discovery_common
)

target_compile_definitions(${MODULE_NAME}
    PRIVATE
        BOOST_ALL_NO_LIB
        DISCOVERY_VERBOSE
)

if (WIN32)
    target_compile_definitions(${MODULE_NAME}
            PUBLIC  _WIN32_WINNT=${MIN_WINDOWS_VERSION}
                    WINVER=${MIN_WINDOWS_VERSION}
                    WIN32_LEAN_AND_MEAN
            )

    target_link_libraries(${MODULE_NAME}
        PUBLIC
            Ws2_32
            IPHLPAPI
    )
endif()

target_include_directories(${MODULE_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
                                                 $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include>

                                                 $<INSTALL_INTERFACE:include>
)

install(TARGETS ${MODULE_NAME}
        EXPORT ${SDK_NAME}
        RUNTIME
            DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY
            DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE
            DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ping
)
