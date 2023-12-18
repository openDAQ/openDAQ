set(daq_hbk_REQUIREDVERSION "3.0.12")

get_custom_fetch_content_params(daqhbkspec FC_PARAMS)
FetchContent_Declare(daqhbkspec
  GIT_REPOSITORY "https://github.com/hbkworld/opc-ua-specs.git"
  GIT_TAG        v${daq_hbk_REQUIREDVERSION}
  ${FC_PARAMS}
)
FetchContent_GetProperties(daqhbkspec)
if(NOT daqhbkspec_POPULATED)
    message(STATUS "Fetching hbk daq specification ${daq_hbk_REQUIREDVERSION}...")
    FetchContent_Populate(daqhbkspec)
    list(APPEND VENDOR_COMPANION_SPECIFICATION_DIRPREFIX "${daqhbkspec_SOURCE_DIR}/opendaq/hbk")
endif()

message(STATUS "Generating DAQ Hbk nodeset...")

ua_generate_nodeset_and_datatypes(
  NAME          "daqhbk"
  TARGET_PREFIX "${PROJECT_NAME}"
  IMPORT_BSD    "UA_TYPES#${open62541_NODESET_DIR}/Schema/Opc.Ua.Types.bsd"
  IMPORT_BSD    "UA_TYPES_DAQBT#${COMPANION_SPECIFICATIONS_DIRPREFIX}/Opc.Ua.Daq.Bt.Types.bsd"
  IMPORT_BSD    "UA_TYPES_DAQBSP#${COMPANION_SPECIFICATIONS_DIRPREFIX}/Opc.Ua.Daq.Bsp.Types.bsd"
  IMPORT_BSD    "UA_TYPES_DAQDEVICE#${COMPANION_SPECIFICATIONS_DIRPREFIX}/Opc.Ua.Daq.Device.Types.bsd"
  IMPORT_BSD    "UA_TYPES_DAQESP#${COMPANION_SPECIFICATIONS_DIRPREFIX}/Opc.Ua.Daq.Esp.Types.bsd"
  FILE_CSV      "${VENDOR_COMPANION_SPECIFICATION_DIRPREFIX}/Opc.Ua.Daq.Vendor.Hbk.NodeIds.csv"
  FILE_BSD      "${VENDOR_COMPANION_SPECIFICATION_DIRPREFIX}/Opc.Ua.Daq.Vendor.Hbk.Types.bsd"
  OUTPUT_DIR    "${GENERATE_OUTPUT_DIR}"
  NAMESPACE_MAP "${NAME_SPACE_INDEX}:https://docs.opendaq.io/specifications/opc-ua/daq/vendor/hbk"
  FILE_NS       "${VENDOR_COMPANION_SPECIFICATION_DIRPREFIX}/Opc.Ua.Daq.Vendor.Hbk.NodeSet2.xml"
  DEPENDS       "daqesp"
  INTERNAL
)

list(APPEND OPC_UA_NODESET_DEPENDENCIES ${PROJECT_NAME}-ns-daqhbk)
list(APPEND OPC_UA_NODESET_SOURCES ${UA_NODESET_DAQHBK_SOURCES} ${UA_TYPES_DAQHBK_SOURCES})
list(APPEND OPC_UA_NODESET_HEADERS ${UA_NODESET_DAQHBK_HEADERS} ${UA_TYPES_DAQHBK_HEADERS} ${GENERATE_OUTPUT_DIR}/daqhbk_nodeids.h)
list(APPEND OPC_UA_NODESET_COMPILE_FLAGS NAMESPACE_DAQHBK=${NAME_SPACE_INDEX})
list(APPEND OPCUA_NODESET_NAMES "daqhbk")
list(APPEND OPCUA_NODESET_URIS "https://docs.opendaq.io/specifications/opc-ua/daq/vendor/hbk")
MATH(EXPR NAME_SPACE_INDEX "${NAME_SPACE_INDEX}+1")