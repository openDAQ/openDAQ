set(LIB_NAME discovery_common)
set(INCLUDE_PREFIX ${LIB_NAME})

set(SRC_HEADERS daq_discovery_common.h)

set(SRC_CPPS daq_discovery_common.cpp)

prepend_include(${INCLUDE_PREFIX} SRC_HEADERS)

add_library(${LIB_NAME} STATIC ${SRC_HEADERS} ${SRC_CPPS})
add_library(${SDK_TARGET_NAMESPACE}::${LIB_NAME} ALIAS ${LIB_NAME})

target_include_directories(${LIB_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${INCLUDE_PREFIX}>
)

target_link_libraries(${LIB_NAME} PUBLIC mdns::mdns
                                         daq::coreobjects
)

install(TARGETS ${LIB_NAME}
        EXPORT ${SDK_NAME}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(FILES ${SRC_HEADERS}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${INCLUDE_PREFIX}
)
