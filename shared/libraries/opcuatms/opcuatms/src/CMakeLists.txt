set(MODULE_NAME opcuatms)

set(SRC_PublicHeaders errors.h
                      opcuatms.h
                      core_types_utils.h
                      type_mappings.h
                      extension_object.h
                      converter_maps.h
)

set(SRC_PrivateHeaders exceptions.h
)

set(SRC_Cpp core_types_utils.cpp
            extension_object.cpp
)

# converters

set(CONVERTERS_SRC_DIR "converters")

set(SRC_Converters_Headers ${CONVERTERS_SRC_DIR}/struct_converter.h
                           ${CONVERTERS_SRC_DIR}/variant_converter.h
                           ${CONVERTERS_SRC_DIR}/list_conversion_utils.h
                           ${CONVERTERS_SRC_DIR}/selection_converter.h
)

set(SRC_Converters ${CONVERTERS_SRC_DIR}/range_converter.cpp
                   ${CONVERTERS_SRC_DIR}/unit_converter.cpp
                   ${CONVERTERS_SRC_DIR}/number_converter.cpp
                   ${CONVERTERS_SRC_DIR}/data_rule_converter.cpp
                   ${CONVERTERS_SRC_DIR}/dimension_rule_converter.cpp
                   ${CONVERTERS_SRC_DIR}/ratio_converter.cpp
                   ${CONVERTERS_SRC_DIR}/data_descriptor_converter.cpp
                   ${CONVERTERS_SRC_DIR}/scaling_converter.cpp
                   ${CONVERTERS_SRC_DIR}/dimension_converter.cpp
                   ${CONVERTERS_SRC_DIR}/function_block_type_converter.cpp
                   ${CONVERTERS_SRC_DIR}/core_types_converter.cpp
                   ${CONVERTERS_SRC_DIR}/base_object_converter.cpp
                   ${CONVERTERS_SRC_DIR}/dict_converter.cpp
                   ${CONVERTERS_SRC_DIR}/selection_converter.cpp
                   ${CONVERTERS_SRC_DIR}/argument_converter.cpp
                   ${CONVERTERS_SRC_DIR}/generic_struct_converter.cpp
)

set(SRC_PublicHeaders ${SRC_PublicHeaders} ${SRC_Converters_Headers})
set(SRC_Cpp ${SRC_Cpp} ${SRC_Converters})

source_group("converters" "${CONVERTERS_SRC_DIR}/*")

# /structs

prepend_include(${MODULE_NAME} SRC_PublicHeaders)
prepend_include(${MODULE_NAME} SRC_PrivateHeaders)

add_library(${MODULE_NAME} STATIC ${SRC_PublicHeaders}
                                  ${SRC_PrivateHeaders}
                                  ${SRC_Cpp}
)

add_library(${SDK_TARGET_NAMESPACE}::${MODULE_NAME} ALIAS ${MODULE_NAME})

target_include_directories(${MODULE_NAME} PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/>
                                                 $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include/>
                                                 $<INSTALL_INTERFACE:include/${MODULE_NAME}>
)

target_link_libraries(${MODULE_NAME} PUBLIC daq::opendaq
                                            daq::opcuashared
                                            daq::opcua_daq_types
)

set_target_properties(${MODULE_NAME} PROPERTIES PUBLIC_HEADER "${SRC_PublicHeaders}")
opendaq_set_output_lib_name(${MODULE_NAME} ${PROJECT_VERSION_MAJOR})