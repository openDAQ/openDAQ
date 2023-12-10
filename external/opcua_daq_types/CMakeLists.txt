set_cmake_folder_context(TARGET_FOLDER_NAME)
project(opcua_daq_types VERSION 1.0.0 DESCRIPTION "DAQ types" LANGUAGES C)

# Define empty list of nodeset dependencies
list(APPEND OPC_UA_NODESET_DEPENDENCIES)

# Add supported Vendors by openDAQ. Please add the vendor name which is used in this file name pattern:
# Opc.Ua.Daq.Vendor.xxx.NodeSet2.xml
list(APPEND OPC_UA_VENDOR_LIST "Hbk")
# Add repository to be fetched and the used tag. Important index needs to be match with vendor list!
list(APPEND OPC_UA_VENDOR_REPOSITORY_LIST "https://github.com/hbkworld/opc-ua-specs.git v3.0.12")

# Define empty list for spec directories 
list(APPEND VENDOR_COMPANION_SPECIFICATIONS_DIRPREFIX)

#
# Fetch the openDAQ OPC-UA Companion Specification NodeSet
#
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

#
# Fetch vendor specifications. Since the repository link could be every time different,
#
set(LOOP_RUN_INDEX 0)
foreach(REP ${OPC_UA_VENDOR_REPOSITORY_LIST})
  string(REPLACE " " ";" REP ${REP})
  list(GET REP 0 REP_URL)
  list(GET REP 1 REP_TAG)
  list(GET OPC_UA_VENDOR_LIST 0 vendor)
  string(TOLOWER ${vendor} vendor)

  get_custom_fetch_content_params(daq${vendor}spec FC_PARAMS)

  FetchContent_Declare(daq${vendor}spec
    GIT_REPOSITORY ${REP_URL}
    GIT_TAG        ${REP_TAG}
    ${FC_PARAMS}
  )
  FetchContent_GetProperties(daq${vendor}spec)
  if(NOT daq${vendor}spec_POPULATED)
      message(STATUS "Fetching ${vendor} daq specification ${REP_TAG}...")
      FetchContent_Populate(daq${vendor}spec)
      list(APPEND VENDOR_COMPANION_SPECIFICATIONS_DIRPREFIX "${daq${vendor}spec_SOURCE_DIR}/opendaq/hbk")
  endif()
endforeach()

# python interpreter is required for the code genaration process
find_package(Python3 REQUIRED)
set(PYTHON_EXECUTABLE ${Python3_EXECUTABLE})

set(DI_NAME_SPACE_INDEX 2)
set(NAME_SPACE_INDEX ${DI_NAME_SPACE_INDEX})
set(GENERATE_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/open62541")

# Define some empty list necessary for code generation
list(APPEND OPC_UA_NODESET_COMPILE_FLAGS)
list(APPEND OPC_UA_NODESET_SOURCES)
list(APPEND OPC_UA_NODESET_HEADERS)

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
list(APPEND OPC_UA_NODESET_SOURCES ${UA_NODESET_DI_SOURCES} ${UA_TYPES_DI_SOURCES})
list(APPEND OPC_UA_NODESET_HEADERS ${UA_NODESET_DI_HEADERS} ${UA_TYPES_DI_HEADERS} ${GENERATE_OUTPUT_DIR}/di_nodeids.h)
list(APPEND OPC_UA_NODESET_COMPILE_FLAGS NAMESPACE_DI=${NAME_SPACE_INDEX})
MATH(EXPR NAME_SPACE_INDEX "${NAME_SPACE_INDEX}+1")


#
# Generate DAQ Base Types (BT) nodeset if needed
#
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
list(APPEND OPC_UA_NODESET_SOURCES ${UA_NODESET_DAQBT_SOURCES} ${UA_TYPES_DAQBT_SOURCES})
list(APPEND OPC_UA_NODESET_HEADERS ${UA_NODESET_DAQBT_HEADERS} ${UA_TYPES_DAQBT_HEADERS} ${GENERATE_OUTPUT_DIR}/daqbt_nodeids.h)
list(APPEND OPC_UA_NODESET_COMPILE_FLAGS NAMESPACE_DAQBT=${NAME_SPACE_INDEX})
MATH(EXPR NAME_SPACE_INDEX "${NAME_SPACE_INDEX}+1")

#
# Generate DAQ Basic Signal Processing (BSP) nodeset if needed
#
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
list(APPEND OPC_UA_NODESET_SOURCES ${UA_NODESET_DAQBSP_SOURCES} ${UA_TYPES_DAQBSP_SOURCES})
list(APPEND OPC_UA_NODESET_HEADERS ${UA_NODESET_DAQBSP_HEADERS} ${UA_TYPES_DAQBSP_HEADERS} ${GENERATE_OUTPUT_DIR}/daqbsp_nodeids.h)
list(APPEND OPC_UA_NODESET_COMPILE_FLAGS NAMESPACE_DAQBSP=${NAME_SPACE_INDEX})
MATH(EXPR NAME_SPACE_INDEX "${NAME_SPACE_INDEX}+1")

#
# Generate DAQ Device nodeset if needed
#
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
list(APPEND OPC_UA_NODESET_SOURCES ${UA_NODESET_DAQDEVICE_SOURCES} ${UA_TYPES_DAQDEVICE_SOURCES})
list(APPEND OPC_UA_NODESET_HEADERS ${UA_NODESET_DAQDEVICE_HEADERS} ${UA_TYPES_DAQDEVICE_HEADERS} ${GENERATE_OUTPUT_DIR}/daqdevice_nodeids.h)
list(APPEND OPC_UA_NODESET_COMPILE_FLAGS NAMESPACE_DAQDEVICE=${NAME_SPACE_INDEX})
MATH(EXPR NAME_SPACE_INDEX "${NAME_SPACE_INDEX}+1")

#
# Generate DAQ ESP nodeset if needed
#
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
list(APPEND OPC_UA_NODESET_SOURCES ${UA_NODESET_DAQESP_SOURCES} ${UA_TYPES_DAQESP_SOURCES})
list(APPEND OPC_UA_NODESET_HEADERS ${UA_NODESET_DAQESP_HEADERS} ${UA_TYPES_DAQESP_HEADERS} ${GENERATE_OUTPUT_DIR}/daqesp_nodeids.h)
list(APPEND OPC_UA_NODESET_COMPILE_FLAGS NAMESPACE_DAQESP=${NAME_SPACE_INDEX})
MATH(EXPR NAME_SPACE_INDEX "${NAME_SPACE_INDEX}+1")


set(LOOP_RUN_INDEX 0)
foreach(VENDOR_SPEC ${OPC_UA_VENDOR_LIST})
  message(STATUS "Generating DAQ ${VENDOR_SPEC} nodeset...")
  string(TOLOWER ${VENDOR_SPEC} specName)
  string(TOUPPER ${VENDOR_SPEC} specNameUpper)
  list(GET VENDOR_COMPANION_SPECIFICATIONS_DIRPREFIX 0 VENDOR_COMPANION_SPECIFICATION_DIRPREFIX)

  ua_generate_nodeset_and_datatypes(
    NAME          "daq${specName}"
    TARGET_PREFIX "${PROJECT_NAME}"
    IMPORT_BSD    "UA_TYPES#${open62541_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
    IMPORT_BSD    "UA_TYPES_DAQBT#${COMPANION_SPECIFICATIONS_DIRPREFIX}/Opc.Ua.Daq.Bt.Types.bsd"
    IMPORT_BSD    "UA_TYPES_DAQBSP#${COMPANION_SPECIFICATIONS_DIRPREFIX}/Opc.Ua.Daq.Bsp.Types.bsd"
    IMPORT_BSD    "UA_TYPES_DAQDEVICE#${COMPANION_SPECIFICATIONS_DIRPREFIX}/Opc.Ua.Daq.Device.Types.bsd"
    IMPORT_BSD    "UA_TYPES_DAQESP#${COMPANION_SPECIFICATIONS_DIRPREFIX}/Opc.Ua.Daq.Esp.Types.bsd"
    FILE_CSV      "${VENDOR_COMPANION_SPECIFICATION_DIRPREFIX}/Opc.Ua.Daq.Vendor.${VENDOR_SPEC}.NodeIds.csv"
    FILE_BSD      "${VENDOR_COMPANION_SPECIFICATION_DIRPREFIX}/Opc.Ua.Daq.Vendor.${VENDOR_SPEC}.Types.bsd"
    OUTPUT_DIR    "${GENERATE_OUTPUT_DIR}"
    NAMESPACE_MAP "${NAME_SPACE_INDEX}:https://docs.opendaq.io/specifications/opc-ua/daq/vendor/${specName}"
    FILE_NS       "${VENDOR_COMPANION_SPECIFICATION_DIRPREFIX}/Opc.Ua.Daq.Vendor.${VENDOR_SPEC}.NodeSet2.xml"
    DEPENDS       "daqesp"
    INTERNAL
  )
  list(APPEND OPC_UA_NODESET_DEPENDENCIES ${PROJECT_NAME}-ns-daq${specName})
  list(APPEND OPC_UA_NODESET_SOURCES ${UA_NODESET_DAQ${specNameUpper}_SOURCES} ${UA_TYPES_DAQ${specNameUpper}_SOURCES})
  list(APPEND OPC_UA_NODESET_HEADERS ${UA_NODESET_DAQ${specNameUpper}_HEADERS} ${UA_TYPES_DAQ${specNameUpper}_HEADERS} ${GENERATE_OUTPUT_DIR}/daq${specName}_nodeids.h)
  list(APPEND OPC_UA_NODESET_COMPILE_FLAGS NAMESPACE_DAQ${specNameUpper}=${NAME_SPACE_INDEX})
  MATH(EXPR NAME_SPACE_INDEX "${NAME_SPACE_INDEX}+1")
  MATH(EXPR LOOP_RUN_INDEX "${LOOP_RUN_INDEX}+1")
endforeach()


#
# Create custom target that will also execute the node compilations (because of the ALL keyword below)
#
list(LENGTH OPC_UA_NODESET_DEPENDENCIES OPC_UA_NODESET_DEPENDENCY_LENGTH)

add_library(${PROJECT_NAME}
        STATIC
            ${OPC_UA_NODESET_SOURCES}
            ${OPC_UA_NODESET_HEADERS}
)

add_library(${SDK_TARGET_NAMESPACE}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:open62541>)

# Make sure that generation is run before but don't link libraries
add_dependencies(${PROJECT_NAME} ${OPC_UA_NODESET_DEPENDENCIES})

target_compile_definitions(${PROJECT_NAME} PUBLIC ${OPC_UA_NODESET_COMPILE_FLAGS})

# Ignore warnings defined in "external/bbopen62541"
target_compile_options(${PROJECT_NAME} PRIVATE ${OPEN62541_DISABLED_WARNINGS})

# Let libraries linking to us know where to find the generated headers
target_include_directories(${PROJECT_NAME} INTERFACE
    $<BUILD_INTERFACE:${GENERATE_OUTPUT_DIR}/..>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
