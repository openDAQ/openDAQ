SET(MODULE_NAME opcuaserver)

set(SOURCE_CPPS opcuaserver.cpp
                opcuaserverlock.cpp
                opcuasession.cpp
                opcuataskqueue.cpp
                opcuaaddnodeparams.cpp
                opcuatmstypes.cpp
                node_event_manager.cpp
                opcuaservernode.cpp
                opcuaservernodefactory.cpp
                event_attributes.cpp
                server_event_manager.cpp
)

set(SOURCE_HEADERS common.h
                   opcuaserver.h
                   opcuaserverlock.h
                   opcuasession.h
                   opcuataskqueue.h
                   opcuatmstypes.h
                   opcuaaddnodeparams.h
                   node_event_manager.h
                   opcuaservernode.h
                   opcuaservernodefactory.h
                   event_attributes.h
                   server_event_manager.h
)

set(INCLUDE_DIR ../include/${MODULE_NAME})

prepend_include(${INCLUDE_DIR} SOURCE_HEADERS)

set(ALL_SOURCES ${SOURCE_CPPS}
                ${SOURCE_HEADERS})

set(SOURCE_FILES ${ALL_SOURCES})

add_library(${MODULE_NAME} STATIC ${ALL_SOURCES})
add_library(${SDK_TARGET_NAMESPACE}::${MODULE_NAME} ALIAS ${MODULE_NAME})

target_include_directories(${MODULE_NAME} PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/>
                                                 $<INSTALL_INTERFACE:include/${MODULE_NAME}>
)

target_link_libraries(${MODULE_NAME} PUBLIC  daq::opcuashared
                                             daq::opendaq_utils
                                     PRIVATE daq::opcua_daq_types
)

set_target_properties(${MODULE_NAME} PROPERTIES PUBLIC_HEADER "${SOURCE_HEADERS}")

if(BUILD_64Bit)
    set_target_properties(${MODULE_NAME} PROPERTIES OUTPUT_NAME "${MODULE_NAME}64")
endif()

if(BUILD_64Bit OR BUILD_ARM)
    set_target_properties(${MODULE_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
else()
    set_target_properties(${MODULE_NAME} PROPERTIES POSITION_INDEPENDENT_CODE OFF)
endif()