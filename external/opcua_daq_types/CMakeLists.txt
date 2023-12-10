set_cmake_folder_context(TARGET_FOLDER_NAME)
project(opcua_daq_types VERSION 1.0.0 DESCRIPTION "DAQ types" LANGUAGES C)

option(OPC_UA_DAQ_BT_NODESET_SUPPORT        "Use the DAQ Base Types (BT) Nodeset."               ON)
option(OPC_UA_DAQ_BSP_NODESET_SUPPORT       "Use the DAQ Basic Signal Processing (BSP) Nodeset." ON)
option(OPC_UA_DAQ_DEVICE_NODESET_SUPPORT    "Use the DAQ Device Nodeset."                        ON)
option(OPC_UA_DAQ_ESP_NODESET_SUPPORT       "Use the DAQ ESP Nodeset."                           ON)
option(OPC_UA_DAQ_VENDOR_NODESET_SUPPORT    "Use additional OPC-UA Vendor Specifications"       OFF)


# Handle dependencies between nodesets:
if (OPC_UA_DAQ_VENDOR_NODESET_SUPPORT)
    set(OPC_UA_DAQ_ESP_NODESET_SUPPORT ON)
endif()

if (OPC_UA_DAQ_ESP_NODESET_SUPPORT)
    set(OPC_UA_DAQ_DEVICE_NODESET_SUPPORT ON)
endif()

if (OPC_UA_DAQ_DEVICE_NODESET_SUPPORT)
    set(OPC_UA_DAQ_BSP_NODESET_SUPPORT ON)
endif()

if (OPC_UA_DAQ_BSP_NODESET_SUPPORT)
    set(OPC_UA_DAQ_BT_NODESET_SUPPORT ON)
endif()

# Define empty list of nodeset dependencies
set(OPC_UA_NODESET_DEPENDENCIES)

#
# Fetch the Test and Measurement NodeSet
#
if (OPC_UA_DAQ_BT_NODESET_SUPPORT OR
    OPC_UA_DAQ_BSP_NODESET_SUPPORT  OR
    OPC_UA_DAQ_DEVICE_NODESET_SUPPORT OR
    OPC_UA_DAQ_ESP_NODESET_SUPPORT)

  set(daq_spec_REQUIREDVERSION "3.0.2")
  get_custom_fetch_content_params(daqspec FC_PARAMS)

  FetchContent_Declare(daqspec
    GIT_REPOSITORY https://github.com/openDAQ/opc-ua-companion-spec.git
    GIT_TAG        v${daq_spec_REQUIREDVERSION}
    ${FC_PARAMS}
  )
  FetchContent_GetProperties(daqspec)
  if(NOT daqspec_POPULATED)
      message(STATUS "Fetching daq specification ${daq_spec_REQUIREDVERSION}...")
      FetchContent_Populate(daqspec)
      set(COMPANION_SPECIFICATIONS_DIRPREFIX "${daqspec_SOURCE_DIR}/opendaq")
  endif()
endif()

# python interpreter is required for the code genaration process
find_package(Python3 REQUIRED)
set(PYTHON_EXECUTABLE ${Python3_EXECUTABLE})

set(DI_NAME_SPACE_INDEX 2)
set(NAME_SPACE_INDEX ${DI_NAME_SPACE_INDEX})
set(GENERATE_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/open62541")
set(NODEID_HEADERS)

#
# Generate DI nodeset
#
message(STATUS "Generating DI nodeset...")
ua_generate_nodeset_and_datatypes(
  NAME          "di"
  TARGET_PREFIX "${PROJECT_NAME}"
  IMPORT_BSD    "UA_TYPES#${open62541_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
  FILE_CSV      "${open62541_NODESET_DIR}/DI/OpcUaDiModel.csv"
  FILE_BSD      "${open62541_NODESET_DIR}/DI/Opc.Ua.Di.Types.bsd"
  OUTPUT_DIR    "${GENERATE_OUTPUT_DIR}"
  NAMESPACE_MAP "${NAME_SPACE_INDEX}:http://opcfoundation.org/UA/DI/"
  FILE_NS       "${open62541_NODESET_DIR}/DI/Opc.Ua.Di.NodeSet2.xml"
  INTERNAL
)
list(APPEND OPC_UA_NODESET_DEPENDENCIES ${PROJECT_NAME}-ns-di)
list(APPEND NODEID_HEADERS ${GENERATE_OUTPUT_DIR}/di_nodeids.h)
MATH(EXPR NAME_SPACE_INDEX "${NAME_SPACE_INDEX}+1")


#
# Generate DAQ Base Types (BT) nodeset if needed
#
if (OPC_UA_DAQ_BT_NODESET_SUPPORT)
  message(STATUS "Generating DAQ BT nodeset...")
  ua_generate_nodeset_and_datatypes(
    NAME          "daqbt"
    TARGET_PREFIX "${PROJECT_NAME}"
    IMPORT_BSD    "UA_TYPES#${open62541_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
    FILE_CSV      "${COMPANION_SPECIFICATIONS_DIRPREFIX}/Opc.Ua.Daq.Bt.NodeIds.csv"
    FILE_BSD      "${COMPANION_SPECIFICATIONS_DIRPREFIX}/Opc.Ua.Daq.Bt.Types.bsd"
    OUTPUT_DIR    "${GENERATE_OUTPUT_DIR}"
    NAMESPACE_MAP "${NAME_SPACE_INDEX}:https://docs.opendaq.io/specifications/opc-ua/daq/bt"
    FILE_NS       "${COMPANION_SPECIFICATIONS_DIRPREFIX}/Opc.Ua.Daq.Bt.NodeSet2.xml"
    DEPENDS       "di"
    INTERNAL
  )
  list(APPEND OPC_UA_NODESET_DEPENDENCIES ${PROJECT_NAME}-ns-daqbt)
  list(APPEND NODEID_HEADERS ${GENERATE_OUTPUT_DIR}/daqbt_nodeids.h)
  MATH(EXPR NAME_SPACE_INDEX "${NAME_SPACE_INDEX}+1")
endif()

#
# Generate DAQ Basic Signal Processing (BSP) nodeset if needed
#
if (OPC_UA_DAQ_BSP_NODESET_SUPPORT)
	message(STATUS "Generating DAQ BSP nodeset...")
  ua_generate_nodeset_and_datatypes(
    NAME          "daqbsp"
    TARGET_PREFIX "${PROJECT_NAME}"
    IMPORT_BSD    "UA_TYPES#${open62541_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
    IMPORT_BSD    "UA_TYPES_DAQBT#${COMPANION_SPECIFICATIONS_DIRPREFIX}/Opc.Ua.Daq.Bt.Types.bsd"
    FILE_CSV      "${COMPANION_SPECIFICATIONS_DIRPREFIX}/Opc.Ua.Daq.Bsp.NodeIds.csv"
    FILE_BSD      "${COMPANION_SPECIFICATIONS_DIRPREFIX}/Opc.Ua.Daq.Bsp.Types.bsd"
    OUTPUT_DIR    "${GENERATE_OUTPUT_DIR}"
    NAMESPACE_MAP "${NAME_SPACE_INDEX}:https://docs.opendaq.io/specifications/opc-ua/daq/bsp"
    FILE_NS       "${COMPANION_SPECIFICATIONS_DIRPREFIX}/Opc.Ua.Daq.Bsp.NodeSet2.xml"
    DEPENDS       "daqbt"
    INTERNAL
  )
  list(APPEND OPC_UA_NODESET_DEPENDENCIES ${PROJECT_NAME}-ns-daqbsp)
  list(APPEND NODEID_HEADERS ${GENERATE_OUTPUT_DIR}/daqbsp_nodeids.h)
  MATH(EXPR NAME_SPACE_INDEX "${NAME_SPACE_INDEX}+1")
endif()

#
# Generate DAQ Device nodeset if needed
#
if (OPC_UA_DAQ_DEVICE_NODESET_SUPPORT)
  message(STATUS "Generating DAQ Device nodeset...")
  ua_generate_nodeset_and_datatypes(
    NAME          "daqdevice"
    TARGET_PREFIX "${PROJECT_NAME}"
    IMPORT_BSD    "UA_TYPES#${open62541_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
    IMPORT_BSD    "UA_TYPES_DAQBT#${COMPANION_SPECIFICATIONS_DIRPREFIX}/Opc.Ua.Daq.Bt.Types.bsd"
    IMPORT_BSD    "UA_TYPES_DAQBSP#${COMPANION_SPECIFICATIONS_DIRPREFIX}/Opc.Ua.Daq.Bsp.Types.bsd"
    FILE_CSV      "${COMPANION_SPECIFICATIONS_DIRPREFIX}/Opc.Ua.Daq.Device.NodeIds.csv"
    FILE_BSD      "${COMPANION_SPECIFICATIONS_DIRPREFIX}/Opc.Ua.Daq.Device.Types.bsd"
    OUTPUT_DIR    "${GENERATE_OUTPUT_DIR}"
    NAMESPACE_MAP "${NAME_SPACE_INDEX}:https://docs.opendaq.io/specifications/opc-ua/daq/device"
    FILE_NS       "${COMPANION_SPECIFICATIONS_DIRPREFIX}/Opc.Ua.Daq.Device.NodeSet2.xml"
    DEPENDS       "daqbsp"
    INTERNAL
  )
  list(APPEND OPC_UA_NODESET_DEPENDENCIES ${PROJECT_NAME}-ns-daqdevice)
  list(APPEND NODEID_HEADERS ${GENERATE_OUTPUT_DIR}/daqdevice_nodeids.h)
  MATH(EXPR NAME_SPACE_INDEX "${NAME_SPACE_INDEX}+1")
endif()

#
# Generate DAQ ESP nodeset if needed
#
if (OPC_UA_DAQ_ESP_NODESET_SUPPORT)
  message(STATUS "Generating DAQ ESP nodeset...")
  ua_generate_nodeset_and_datatypes(
    NAME          "daqesp"
    TARGET_PREFIX "${PROJECT_NAME}"
    IMPORT_BSD    "UA_TYPES#${open62541_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
    IMPORT_BSD    "UA_TYPES_DAQBT#${COMPANION_SPECIFICATIONS_DIRPREFIX}/Opc.Ua.Daq.Bt.Types.bsd"
    IMPORT_BSD    "UA_TYPES_DAQBSP#${COMPANION_SPECIFICATIONS_DIRPREFIX}/Opc.Ua.Daq.Bsp.Types.bsd"
    IMPORT_BSD    "UA_TYPES_DAQDEVICE#${COMPANION_SPECIFICATIONS_DIRPREFIX}/Opc.Ua.Daq.Device.Types.bsd"
    FILE_CSV      "${COMPANION_SPECIFICATIONS_DIRPREFIX}/Opc.Ua.Daq.Esp.NodeIds.csv"
    FILE_BSD      "${COMPANION_SPECIFICATIONS_DIRPREFIX}/Opc.Ua.Daq.Esp.Types.bsd"
    OUTPUT_DIR    "${GENERATE_OUTPUT_DIR}"
    NAMESPACE_MAP "${NAME_SPACE_INDEX}:https://docs.opendaq.io/specifications/opc-ua/daq/esp"
    FILE_NS       "${COMPANION_SPECIFICATIONS_DIRPREFIX}/Opc.Ua.Daq.Esp.NodeSet2.xml"
    DEPENDS       "daqdevice"
    INTERNAL
  )
  list(APPEND OPC_UA_NODESET_DEPENDENCIES ${PROJECT_NAME}-ns-daqesp)
  list(APPEND NODEID_HEADERS ${GENERATE_OUTPUT_DIR}/daqesp_nodeids.h)
  MATH(EXPR NAME_SPACE_INDEX "${NAME_SPACE_INDEX}+1")
endif()

#
# Create custom target that will also execute the node compilations (because of the ALL keyword below)
#
list(LENGTH OPC_UA_NODESET_DEPENDENCIES OPC_UA_NODESET_DEPENDENCY_LENGTH)

set (OPC_UA_NODESET_SOURCES
    ${UA_NODESET_DI_SOURCES}
    ${UA_TYPES_DI_SOURCES}
    ${UA_NODESET_DAQBT_SOURCES}
    ${UA_TYPES_DAQBT_SOURCES}
    ${UA_NODESET_DAQBSP_SOURCES}
    ${UA_TYPES_DAQBSP_SOURCES}
    ${UA_NODESET_DAQDEVICE_SOURCES}
    ${UA_TYPES_DAQDEVICE_SOURCES}
    ${UA_NODESET_DAQESP_SOURCES}
    ${UA_TYPES_DAQESP_SOURCES}
)

set (OPC_UA_NODESET_HEADERS
    ${UA_NODESET_DI_HEADERS}
    ${UA_TYPES_DI_HEADERS}
    ${UA_NODESET_DAQBT_HEADERS}
    ${UA_TYPES_DAQBT_HEADERS}
    ${UA_NODESET_DAQBSP_HEADERS}
    ${UA_TYPES_DAQBSP_HEADERS}
    ${UA_NODESET_DAQDEVICE_HEADERS}
    ${UA_TYPES_DAQDEVICE_HEADERS}
    ${UA_NODESET_DAQESP_HEADERS}
    ${UA_TYPES_DAQESP_HEADERS}
    ${NODEID_HEADERS}
)

add_library(${PROJECT_NAME}
        STATIC
            ${OPC_UA_NODESET_SOURCES}
            ${OPC_UA_NODESET_HEADERS}
)

add_library(${SDK_TARGET_NAMESPACE}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:open62541>)

# Make sure that generation is run before but don't link libraries
add_dependencies(${PROJECT_NAME} ${OPC_UA_NODESET_DEPENDENCIES})

set(OPC_UA_NODESET_COMPILE_FLAGS)

set(DI_NAME_SPACE_INDEX 2)
set(NAME_SPACE_INDEX ${DI_NAME_SPACE_INDEX})

list(APPEND OPC_UA_NODESET_COMPILE_FLAGS NAMESPACE_DI=${NAME_SPACE_INDEX})
MATH(EXPR NAME_SPACE_INDEX "${NAME_SPACE_INDEX}+1")

if (OPC_UA_DAQ_BT_NODESET_SUPPORT)
    list(APPEND OPC_UA_NODESET_COMPILE_FLAGS NAMESPACE_DAQBT=${NAME_SPACE_INDEX})
    MATH(EXPR NAME_SPACE_INDEX "${NAME_SPACE_INDEX}+1")
endif()

if (OPC_UA_DAQ_BSP_NODESET_SUPPORT)
    list(APPEND OPC_UA_NODESET_COMPILE_FLAGS NAMESPACE_DAQBSP=${NAME_SPACE_INDEX})
    MATH(EXPR NAME_SPACE_INDEX "${NAME_SPACE_INDEX}+1")
endif()

if (OPC_UA_DAQ_DEVICE_NODESET_SUPPORT)
    list(APPEND OPC_UA_NODESET_COMPILE_FLAGS NAMESPACE_DAQDEVICE=${NAME_SPACE_INDEX})
    MATH(EXPR NAME_SPACE_INDEX "${NAME_SPACE_INDEX}+1")
endif()

if (OPC_UA_DAQ_ESP_NODESET_SUPPORT)
    list(APPEND OPC_UA_NODESET_COMPILE_FLAGS NAMESPACE_DAQESP=${NAME_SPACE_INDEX})
    MATH(EXPR NAME_SPACE_INDEX "${NAME_SPACE_INDEX}+1")
endif()

target_compile_definitions(${PROJECT_NAME} PUBLIC ${OPC_UA_NODESET_COMPILE_FLAGS})

# Ignore warnings defined in "external/bbopen62541"
target_compile_options(${PROJECT_NAME} PRIVATE ${OPEN62541_DISABLED_WARNINGS})

# Let libraries linking to us know where to find the generated headers
target_include_directories(${PROJECT_NAME} INTERFACE
    $<BUILD_INTERFACE:${GENERATE_OUTPUT_DIR}/..>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
