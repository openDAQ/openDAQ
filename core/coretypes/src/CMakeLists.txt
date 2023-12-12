set(BASE_NAME coretypes)
set(LIB_NAME ${SDK_TARGET_NAMESPACE}${BASE_NAME})

set(SDK_HEADERS_DIR ../include/${BASE_NAME})

set(ConfigHeaderSource ${SDK_HEADERS_DIR}/coretypes_config.h.in)
set(ConfigHeader ${CMAKE_CURRENT_BINARY_DIR}/${SDK_HEADERS_DIR}/coretypes_config.h)

configure_file(${ConfigHeaderSource} ${ConfigHeader})

set(SRC_Cpp version.cpp
            binarydata_impl.cpp
            boolean_impl.cpp
            integer_impl.cpp
            float_impl.cpp
            complex_number_impl.cpp
            stringobject_impl.cpp
            baseobject_impl.cpp
            json_serializer_impl.cpp
            json_deserializer_impl.cpp
            deserializer.cpp
            json_serialized_object.cpp
            json_serialized_list.cpp
            errorinfo_impl.cpp
            ratio_impl.cpp
            customalloc.cpp
            intfs.cpp
            event_impl.cpp
            event_args_impl.cpp
            function_custom_impl.cpp
            procedure_custom_impl.cpp
            mem.cpp
            cycle_detector.cpp
            version_info_impl.cpp
            struct_impl.cpp
            struct_type_impl.cpp
            type_manager_impl.cpp
            simple_type_impl.cpp
            util.cpp
            struct_builder_impl.cpp
)

set(SRC_PublicHeaders
    version.h
    formatter.h
    mem.h
    sha1.h
    customalloc.h
    convertible.h
    comparable.h
    iterator.h
    iterable.h
    iterable_ptr.h
    coretype.h
    freezable.h

    inspectable.h
    inspectable_ptr.h

    ratio.h
    ratio_ptr.h
    ratio_factory.h
    simplified_ratio_ptr.h

    serializable.h
    serializable_ptr.h
    serialized_list_ptr.h
    serialized_object_ptr.h
    serializer.h
    serializer_ptr.h
    serialized_list.h
    serialized_object.h
    serialization.h

    json_serializer.h
    json_serialized_list.h
    json_serialized_object.h
    json_serializer_factory.h

    deserializer.h
    deserializer_ptr.h

    json_deserializer.h
    json_deserializer_factory.h

    binarydata.h
    binarydata_factory.h
    binarydata_ptr.h
    binary_data_ptr.h

    callback.h

    function.h
    function_factory.h
    function_custom_impl.h
    function_impl.h
    function_ptr.h

    boolean.h
    boolean_factory.h

    integer.h
    integer_factory.h

    float.h
    float_factory.h

    number.h
    number_ptr.h

    complex_number.h
    complex_number_ptr.h
    complex_number_factory.h

    stringobject.h
    stringobject_factory.h
    string_ptr.h

    baseobject.h
    baseobject_factory.h

    weakref.h
    weakrefobj.h
    weakrefptr.h
    weakref_impl.h

    errorinfo.h
    errorinfo_factory.h

    factory.h

    factoryselectors.h

    common.h
    ctutils.h
    errors.h
    coretypes.h
    intfs.h
    intfid.h

    delegate.hpp
    event.h
    event_ptr.h
    event_wrapper.h
    event_factory.h

    event_args.h
    event_args_ptr.h
    event_args_impl.h
    event_args_factory.h

    event_handler.h
    event_emitter.h
    event_handler_ptr.h
    event_handler_impl.h

    exceptions.h
    error_code_to_exception.h
    bb_exception.h

    procedure.h
    procedure_factory.h
    procedure_ptr.h
    procedure_custom_impl.h
    procedure_impl.h

    cycle_detector.h
    anonymous_variable.h
    updatable.h
    updatable_ptr.h

    macro_utils.h
    span.h
    arguments.h
    type_name.h
    type_name_detail.h

    constexpr_string.h
    constexpr_utils.h

    function_traits.h
    coretype_traits.h

    impl.h
    objectptr.h
    iterator_support.h
    object_decorator.h
    multi_ptr.h

    version_info.h
    version_info_ptr.h
    version_info_factory.h

    cloneable.h
    complex_number_type.h
    coretype_utils.h

    validation.h
    list_element_type.h
    dict_element_type.h

    struct.h
    struct_ptr.h
    struct_factory.h
	
    struct_builder.h
    struct_builder_impl.h
    struct_builder_ptr.h

    type.h
    type_ptr.h

    struct_type.h
    struct_type_ptr.h
    struct_type_factory.h

    type_manager.h
    type_manager_ptr.h
    type_manager_factory.h

    simple_type.h
    simple_type_factory.h
)

set(SRC_PrivateHeaders baseobject_impl.h
                       stringobject_impl.h
                       ordinalobject_impl.h
                       number_impl.h
                       float_impl.h
                       complex_number_impl.h
                       integer_impl.h
                       boolean_impl.h
                       binarydata_impl.h
                       json_serializer_impl.h
                       json_deserializer_impl.h
                       ratio_impl.h
                       event_impl.h
                       event_args_impl.h
                       iterator_helper.h
                       iterator_base_impl.h
                       errorinfo_impl.h
                       version_info_impl.h
                       struct_impl.h
                       type_impl.h
                       struct_type_impl.h
                       type_manager_impl.h
                       simple_type_impl.h
)

prepend_include(${TARGET_FOLDER_NAME} SRC_PrivateHeaders)
prepend_include(${TARGET_FOLDER_NAME} SRC_PublicHeaders)

set(SRC_PublicHeaders ${SRC_PublicHeaders}
                      ${ConfigHeader}
                      coretypes.natvis
)

add_library(${LIB_NAME} SHARED ${SRC_Cpp}
                               ${SRC_PublicHeaders}
                               ${SRC_PrivateHeaders}
                               ${ConfigHeaderSource}
)
add_library(${SDK_TARGET_NAMESPACE}::${BASE_NAME} ALIAS ${LIB_NAME})

target_link_libraries(${LIB_NAME} PRIVATE daq::core_containers
                                          rapidjson
                                          tsl::ordered_map
                                  PUBLIC xxHash::xxhash
                                         $<BUILD_INTERFACE:fmt::fmt>
                                         $<INSTALL_INTERFACE:fmt::fmt-header-only>
)

get_target_property(CORE_CONTAINERS_INCLUDE daq::core_containers INTERFACE_INCLUDE_DIRECTORIES)

target_include_directories(${LIB_NAME} PUBLIC    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
                                                 $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include>

                                                 $<INSTALL_INTERFACE:include>
                                       INTERFACE ${CORE_CONTAINERS_INCLUDE}
)

if (CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options(${LIB_NAME} PRIVATE -Wno-type-limits)
endif()

target_compile_definitions(${LIB_NAME} PUBLIC OPENDAQ_SKIP_LIB_LINK
                                       PRIVATE BUILDING_SHARED_LIBRARY
                                               coretypes_EXPORTS
)

if (MSVC)
    set_source_files_properties(intfs.cpp PROPERTIES COMPILE_FLAGS /wd4100)
endif()

if (WIN32)
    if (MSVC OR CMAKE_COMPILER_IS_CLANGXX)
        target_compile_definitions(${LIB_NAME} PUBLIC _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)
    endif()
endif()

target_compile_features(${LIB_NAME} PRIVATE cxx_thread_local
                                    PUBLIC cxx_std_17
)

set_target_properties(${LIB_NAME} PROPERTIES PUBLIC_HEADER "${SRC_PublicHeaders}")
set_target_properties(${LIB_NAME} PROPERTIES LIB_MAJOR_VERSION ${PROJECT_VERSION_MAJOR})

set_source_files_properties(${ConfigHeader} PROPERTIES GENERATED TRUE)
source_group("Generated\\Header Files" FILES ${ConfigHeader})

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
