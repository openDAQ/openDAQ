set(LIB_NAME discovery)
set(INCLUDE_PREFIX daq_${LIB_NAME})

set(SRC_HEADERS common.h
                daq_discovery_client.h
                mdnsdiscovery_client.h
)

set(SRC_CPPS daq_discovery_client.cpp
)

opendaq_prepend_include(${INCLUDE_PREFIX} SRC_HEADERS)

add_library(${LIB_NAME} STATIC ${SRC_HEADERS} ${SRC_CPPS})
add_library(${SDK_TARGET_NAMESPACE}::${LIB_NAME} ALIAS ${LIB_NAME})

target_include_directories(${LIB_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${INCLUDE_PREFIX}>
        ${Boost_INCLUDE_DIRS}
)

if (WIN32)
    set(BCRYPT_LIB bcrypt.dll)
endif()

target_link_libraries(${LIB_NAME} PUBLIC mdns::mdns
                                         daq::coreobjects
                                         ${BCRYPT_LIB}
                                         $<BUILD_INTERFACE:Boost::uuid>
                                         daq::discovery_common
)

install(TARGETS ${LIB_NAME}
        EXPORT ${SDK_NAME}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(FILES ${SRC_HEADERS}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${INCLUDE_PREFIX}
)
