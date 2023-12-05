SET(MODULE_NAME opcuashared)

set(RES_DIR_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../external/res")
set(CMAKE_GLOBALS_HEADER ${CMAKE_CURRENT_BINARY_DIR}/../include/${MODULE_NAME}/generated/cmake_globals.h)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake_globals.h.template
    ${CMAKE_GLOBALS_HEADER} @ONLY
)

set(SOURCE_CPPS opcuavariant.cpp
                opcuacommon.cpp
                opcuasecuritycommon.cpp
                opcuaendpoint.cpp
                opcuanodecollection.cpp
                opcuadatavalue.cpp
                opcuacallmethodresult.cpp
                opcuaversion.cpp
                opcuasecurity_config.cpp
                opcuadatatypearraylist.cpp
                opcuanodeid.cpp
                cmake_globals.h.template
                OpcUaTypes.natvis
                bcrypt.cpp
                bcrypt/crypt_blowfish.c
)

set(SOURCE_NODE_CPPS node/opcuanode.cpp
                     node/opcuanodeobject.cpp
                     node/opcuanodemethod.cpp
                     node/opcuanodevariable.cpp
                     node/opcuatype.cpp
                     node/opcuaobjecttype.cpp
                     node/opcuavariabletype.cpp
                     node/opcuadatatype.cpp
)

set(SOURCE_HEADERS opcuavariant.h
                   opcua.h
                   opcualog.h
                   opcuacollection.h
                   opcuacommon.h
                   opcuasecuritycommon.h
                   opcuaendpoint.h
                   opcuanodecollection.h
                   opcuaexception.h
                   opcuadatavalue.h
                   opcuacallmethodresult.h
                   opcuaversion.h
                   opcuaobject.h
                   opcuasecurity_config.h
                   opcuadatatypearraylist.h
                   opcuavector.h
                   opcuanodeid.h
                   opcua_attribute.h
                   bcrypt.h
                   bcrypt/crypt_blowfish.h
)

set(SOURCE_NODE_HEADERS node/opcuanode.h
                        node/opcuanodeobject.h
                        node/opcuanodemethod.h
                        node/opcuanodevariable.h
                        node/opcuatype.h
                        node/opcuaobjecttype.h
                        node/opcuavariabletype.h
                        node/opcuadatatype.h
)

set(INCLUDE_DIR ../include/${MODULE_NAME})

prepend_include(${INCLUDE_DIR} SOURCE_HEADERS)
prepend_include(${INCLUDE_DIR} SOURCE_NODE_HEADERS)

set(ALL_SOURCES ${SOURCE_CPPS}
                ${SOURCE_NODE_CPPS}
                ${SOURCE_HEADERS}
                ${SOURCE_NODE_HEADERS}
                ${CMAKE_GLOBALS_HEADER}
)

if (MSVC)
    set_source_files_properties(bcrypt/crypt_blowfish.h
                                bcrypt/crypt_blowfish.c
                                PROPERTIES LANGUAGE C COMPILE_FLAGS /wd4244)
endif()

set(SOURCE_FILES ${ALL_SOURCES})

add_library(${MODULE_NAME} STATIC ${ALL_SOURCES})
add_library(${SDK_TARGET_NAMESPACE}::${MODULE_NAME} ALIAS ${MODULE_NAME})

if(BUILD_64Bit OR BUILD_ARM)
    set_target_properties(${MODULE_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
else()
    set_target_properties(${MODULE_NAME} PROPERTIES POSITION_INDEPENDENT_CODE OFF)
endif()

target_include_directories(${MODULE_NAME} PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                                                 $<INSTALL_INTERFACE:include/${MODULE_NAME}>
                                                 $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
                                          PRIVATE ${PROJECT_SOURCE_DIR}/include/${MODULE_NAME}/generated
)

target_link_libraries(${MODULE_NAME} PUBLIC  open62541
                                             rapidjson
                                     PRIVATE daq::opendaq_utils
)

if (MSVC)
    target_compile_definitions(${MODULE_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
    set_source_files_properties(opcuacommon.cpp PROPERTIES COMPILE_FLAGS /wd4100)
endif()

set_target_properties(${MODULE_NAME} PROPERTIES PUBLIC_HEADER "${SOURCE_HEADERS}")
set_target_properties(${MODULE_NAME} PROPERTIES PUBLIC_NODE_HEADER "${SOURCE_NODE_HEADERS}")

if(BUILD_64Bit)
    set_target_properties(${MODULE_NAME} PROPERTIES OUTPUT_NAME "${MODULE_NAME}64")
endif()

if (MSVC)
    source_group("Header Files\\Node" "node/.+[.]h")
    source_group("Source Files\\Node" "node/.+[.]cpp")
endif()
