set(BASE_NAME coreobjects)
set(LIB_NAME ${SDK_TARGET_NAMESPACE}${BASE_NAME})


set(SDK_HEADERS_DIR ../include/${BASE_NAME})
set(GENERATED_HEADERS_DIR ${CMAKE_CURRENT_BINARY_DIR}/${SDK_HEADERS_DIR})

set(ConfigHeaderSource ${SDK_HEADERS_DIR}/coreobjects_config.h.in)
set(ConfigHeader ${GENERATED_HEADERS_DIR}/coreobjects_config.h)

configure_file(${ConfigHeaderSource} ${ConfigHeader})

set_source_files_properties(${ConfigHeader} PROPERTIES GENERATED TRUE)
source_group("Generated\\Header Files" FILES ${ConfigHeader})

set(RTGEN_HEADERS_DIR ${SDK_HEADERS_DIR})
set(RTGEN_OUTPUT_DIR ${GENERATED_HEADERS_DIR})
#set(RTGEN_VERBOSE TRUE)

rtgen(SRC_EvalValue eval_value.h)
rtgen(SRC_Ownable ownable.h)
rtgen(SRC_Property property.h)
rtgen(SRC_PropertyInternal property_internal.h)
rtgen(SRC_PropertyBuilder property_builder.h)
rtgen(SRC_PropertyValueEventArgs property_value_event_args.h)
rtgen(SRC_EndUpdateEventArgs end_update_event_args.h)
rtgen(SRC_PropertyObject property_object.h)
rtgen(SRC_PropertyObjectInternal property_object_internal.h)
rtgen(SRC_PropertyObjectClass property_object_class.h)
rtgen(SRC_PropertyObjectClassBuilder property_object_class_builder.h)
rtgen(SRC_PropertyObjectProtected property_object_protected.h)
rtgen(SRC_CallableInfo callable_info.h)
rtgen(SRC_ArgumentInfo argument_info.h)
rtgen(SRC_Coercer coercer.h)
rtgen(SRC_Validator validator.h)
rtgen(SRC_Unit unit.h)
rtgen(SRC_UnitBuilder unit_builder.h)
rtgen(SRC_ComponentType component_type.h)
rtgen(SRC_CoreEventArgs core_event_args.h)

get_target_property(CORE_CONTAINERS_SRCS core_containers INTERFACE_SOURCES)

source_group("core_containers" FILES ${CORE_CONTAINERS_SRCS})

source_group("eval_value" FILES ${SDK_HEADERS_DIR}/eval_nodes.h
                                ${SDK_HEADERS_DIR}/eval_value.h
                                ${SDK_HEADERS_DIR}/eval_value_factory.h
                                ${SDK_HEADERS_DIR}/eval_value_helpers.h
                                ${SDK_HEADERS_DIR}/eval_value_impl.h
                                ${SDK_HEADERS_DIR}/eval_value_parser.h
                                ${SDK_HEADERS_DIR}/eval_value_lexer.h
                                eval_value_impl.cpp
                                eval_value_lexer.cpp
                                eval_value_parser.cpp
                                eval_nodes.cpp
)

source_group("validation" FILES ${SDK_HEADERS_DIR}/validator.h
                                ${SDK_HEADERS_DIR}/validator_factory.h
                                ${SDK_HEADERS_DIR}/validator_impl.h
                                ${SDK_HEADERS_DIR}/coercer.h
                                ${SDK_HEADERS_DIR}/coercer_factory.h
                                ${SDK_HEADERS_DIR}/coercer_impl.h
                                validator_impl.cpp
                                coercer_impl.cpp
)

source_group("property" FILES ${SDK_HEADERS_DIR}/property.h
                              ${SDK_HEADERS_DIR}/property_internal.h
                              ${SDK_HEADERS_DIR}/property_builder.h
                              ${SDK_HEADERS_DIR}/property_ptr.fwd_declare.h
                              ${SDK_HEADERS_DIR}/property_factory.h
                              ${SDK_HEADERS_DIR}/property_impl.h
                              ${SDK_HEADERS_DIR}/property_builder_impl.h
                              ${SDK_HEADERS_DIR}/argument_info.h
                              ${SDK_HEADERS_DIR}/argument_info_factory.h
                              ${SDK_HEADERS_DIR}/argument_info_impl.h
                              ${SDK_HEADERS_DIR}/callable_info_factory.h
                              ${SDK_HEADERS_DIR}/callable_info_impl.h
                              ${SDK_HEADERS_DIR}/callable_info.h
                              property_impl.cpp
                              property_builder_impl.cpp
                              argument_info_impl.cpp
                              callable_info_impl.cpp
)

source_group("property_object" FILES ${SDK_HEADERS_DIR}/property_object.h
                                     ${SDK_HEADERS_DIR}/property_object_factory.h
                                     ${SDK_HEADERS_DIR}/property_object_impl.h
                                     ${SDK_HEADERS_DIR}/object_keys.h
                                     ${SDK_HEADERS_DIR}/property_object_ptr.custom.h
                                     ${SDK_HEADERS_DIR}/property_object_protected.h
                                     ${SDK_HEADERS_DIR}/property_object_internal.h
                                     property_object_impl.cpp
)

source_group("property_object_class" FILES ${SDK_HEADERS_DIR}/property_object_class.h
                                           ${SDK_HEADERS_DIR}/property_object_class_builder.h
                                           ${SDK_HEADERS_DIR}/property_object_class_factory.h
                                           ${SDK_HEADERS_DIR}/property_object_class_impl.h
                                           ${SDK_HEADERS_DIR}/property_object_class_builder_impl.h
                                           property_object_class_impl.cpp
                                           property_object_class_builder_impl.cpp
)

source_group("property_value" FILES ${SDK_HEADERS_DIR}/property_value.h
                                    ${SDK_HEADERS_DIR}/property_value_event_args.h
                                    ${SDK_HEADERS_DIR}/property_value_event_args_factory.h
                                    ${SDK_HEADERS_DIR}/property_value_event_args_impl.h
                                    property_value_event_args_impl.cpp
)

source_group("end_update_event_args" FILES ${SDK_HEADERS_DIR}/end_update_event_args.h
                                           ${SDK_HEADERS_DIR}/end_update_event_args_factory.h
                                           ${SDK_HEADERS_DIR}/end_update_event_args_impl.h
                                           end_update_event_args_impl.cpp
)

source_group("owning_containers" FILES ${SDK_HEADERS_DIR}/owning_dict_factory.h
                                       ${SDK_HEADERS_DIR}/owning_dict_impl.h
                                       ${SDK_HEADERS_DIR}/owning_list_factory.h
                                       ${SDK_HEADERS_DIR}/owning_list_impl.h
                                       owning_dict_impl.cpp
                                       owning_list_impl.cpp
)

source_group("unit" FILES ${SDK_HEADERS_DIR}/unit.h
                          ${SDK_HEADERS_DIR}/unit_impl.h
                          ${SDK_HEADERS_DIR}/unit_factory.h
                          ${SDK_HEADERS_DIR}/unit_builder.h
                          ${SDK_HEADERS_DIR}/unit_builder_impl.h
                          unit_impl.cpp
                          unit_builder_impl.cpp
)

source_group("component_type" FILES ${SDK_HEADERS_DIR}/component_type.h
                                    ${SDK_HEADERS_DIR}/component_type_impl.h
)

source_group("core_event_args" FILES ${SDK_HEADERS_DIR}/core_event_args.h
                                     ${SDK_HEADERS_DIR}/core_event_args_factory.h
                                     ${SDK_HEADERS_DIR}/core_event_args_impl.h
                                     ${SDK_HEADERS_DIR}/core_event_args_ids.h
                                     core_event_args_impl.cpp
)

set(SRC_Cpp version.cpp
            eval_value_impl.cpp
            eval_value_lexer.cpp
            eval_value_parser.cpp
            eval_nodes.cpp
            property_builder_impl.cpp
            property_impl.cpp
            property_object_impl.cpp
            property_object_class_impl.cpp
            property_object_class_builder_impl.cpp
            property_value_event_args_impl.cpp
            end_update_event_args_impl.cpp
            owning_list_impl.cpp
            owning_dict_impl.cpp
            callable_info_impl.cpp
            argument_info_impl.cpp
            validator_impl.cpp
            coercer_impl.cpp
            util.cpp
            unit_impl.cpp
            unit_builder_impl.cpp
			core_event_args_impl.cpp
)

set(SRC_PublicHeaders coreobjects.h
                      object_keys.h
                      version.h
                      serialization_utils.h
                      eval_value_factory.h
                      property_value.h
                      property_factory.h
                      property_ptr.fwd_declare.h
                      property_impl.h
                      property_value_event_args_factory.h
                      end_update_event_args_factory.h
                      property_object_impl.h
                      property_object_factory.h
                      property_object_class_factory.h
                      property_object_class_impl.h
                      property_object_ptr.custom.h
                      owning_list_factory.h
                      owning_dict_factory.h
                      callable_info_factory.h
                      argument_info_factory.h
                      validator.h
                      validator_factory.h
                      coercer.h
                      coercer_factory.h
                      util.h
                      exceptions.h
                      errors.h
                      unit_factory.h
                      component_type.h
                      core_event_args_factory.h 
                      core_event_args_ids.h 
					  core_event_args_impl.h
)

set(SRC_PrivateHeaders eval_value_impl.h
                       eval_nodes.h
                       eval_value_lexer.h
                       eval_value_parser.h
                       eval_value_helpers.h
                       owning_list_impl.h
                       owning_dict_impl.h
                       callable_info_impl.h
                       argument_info_impl.h
                       property_value_event_args_impl.h
                       end_update_event_args_impl.h
                       validator_impl.h
                       coercer_impl.h
                       unit_impl.h
                       unit_builder_impl.h
                       property_builder_impl.h
                       property_object_class_builder_impl.h
                       property_object_internal.h
                       component_type_impl.h
)

prepend_include(${TARGET_FOLDER_NAME} SRC_PrivateHeaders)
prepend_include(${TARGET_FOLDER_NAME} SRC_PublicHeaders)

list(APPEND SRC_PublicHeaders  ${ConfigHeader}
                               ${SRC_Property}
                               ${SRC_PropertyInternal}
                               ${SRC_PropertyBuilder}
                               ${SRC_PropertyObjectProtected}
                               ${SRC_PropertyObjectClass}
                               ${SRC_PropertyObjectClassBuilder}
                               ${SRC_PropertyObjectInternal}
                               ${SRC_PropertyObject_PublicHeaders}
                               ${SRC_Ownable}
                               ${SRC_EvalValue}
                               ${SRC_DelphiBindings}
                               ${SRC_PropertyValueEventArgs}
                               ${SRC_EndUpdateEventArgs}
                               ${SRC_CallableInfo}
                               ${SRC_ArgumentInfo}
                               ${SRC_Validator}
                               ${SRC_Coercer}
                               ${SRC_Unit}
                               ${SRC_UnitBuilder}
                               ${SRC_ComponentType}
                               ${SRC_CoreEventArgs}
                               coreobjects.natvis
)

add_library(${LIB_NAME} SHARED ${SRC_Cpp}
                               ${SRC_PrivateHeaders}
                               ${SRC_PublicHeaders}
                               ${ConfigHeaderSource}
)

add_library(${SDK_TARGET_NAMESPACE}::${BASE_NAME} ALIAS ${LIB_NAME})

rtgen_config(${PROJECT_NAME}
             ${LIB_NAME}
             ${PROJECT_VERSION_MAJOR}
             ${PROJECT_VERSION_MINOR}
             ${PROJECT_VERSION_PATCH}
)

target_link_libraries(${LIB_NAME} PUBLIC daq::coretypes
                                         tsl::ordered_map
                                  PRIVATE daq::core_containers
)

target_include_directories(${LIB_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
                                              $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include>
                                              $<BUILD_INTERFACE:${GENERATED_HEADERS_DIR}>

                                              $<INSTALL_INTERFACE:include>
)

target_compile_definitions(${LIB_NAME} PRIVATE BUILDING_SHARED_LIBRARY
                                       INTERFACE OPENDAQ_LINKS_CORE_OBJECTS
)
set_target_properties(${LIB_NAME} PROPERTIES PUBLIC_HEADER "${SRC_PublicHeaders}")

opendaq_set_output_lib_name(${LIB_NAME} ${PROJECT_VERSION_MAJOR})
create_version_header(${LIB_NAME} ONLY_RC)

install(TARGETS ${LIB_NAME}
        EXPORT ${SDK_NAME}
        RUNTIME
            DESTINATION ${CMAKE_INSTALL_BINDIR}
            COMPONENT ${SDK_NAME}_${BASE_NAME}_Runtime
        LIBRARY
            DESTINATION ${CMAKE_INSTALL_LIBDIR}
            COMPONENT          ${SDK_NAME}_${BASE_NAME}_Runtime
            NAMELINK_COMPONENT ${SDK_NAME}_${BASE_NAME}_Development
        ARCHIVE
            DESTINATION ${CMAKE_INSTALL_LIBDIR}
            COMPONENT ${SDK_NAME}_${BASE_NAME}_Development
        PUBLIC_HEADER
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${BASE_NAME}
            COMPONENT ${SDK_NAME}_${BASE_NAME}_Development
)
