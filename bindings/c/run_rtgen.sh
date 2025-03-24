#!/usr/bin/bash

export RTGEN="$PWD/../../shared/tools/RTGen/bin/rtgen.exe"
export CORE_DIR="$PWD/../../core"
export BINDINGS_DIR="$PWD"
#${RTGEN} --help

# 1: library name (CamelCase)
# 2: library/subdir
# 3: library
# 4: destination for python bindings
# 5: name
function run_rtgen {
    lib_name=$1
    lib_dir1=$2
    lib_dir2=$3
    name=$4
    out_name=$5

    file_h=$name.h
    file_c_h=${out_name}.h
    file_c_cpp=${out_name}.cpp
    source_dir="${CORE_DIR}/${lib_dir1}/include/${lib_dir2}"
    dest_dir_headers="${BINDINGS_DIR}/include/${lib_name}"
    dest_dir_sources="${BINDINGS_DIR}/src/${lib_name}"

    command_h="${RTGEN} --language=c --library=${lib_name} --namespace=daq --source=${source_dir}/${file_h} --outputDir=${dest_dir_headers} --output=${out_name} --extension=.h"
    command_cpp="${RTGEN} --language=c --library=${lib_name} --namespace=daq --source=${source_dir}/${file_h} --outputDir=${dest_dir_sources} --output=${out_name} --extension=.cpp"
    
    echo "------------------------------------------"
    echo "$command_h"
    $command_h
    echo "$command_cpp"
    $command_cpp
}

# run_rtgen ccoretypes coretypes coretypes anonymous_variable anonymous_variable
# run_rtgen ccoretypes coretypes coretypes arguments arguments
# run_rtgen ccoretypes coretypes coretypes baseobject baseobject
# run_rtgen ccoretypes coretypes coretypes baseobject_factory baseobject_factory
# run_rtgen ccoretypes coretypes coretypes baseobject_impl baseobject_impl
# run_rtgen ccoretypes coretypes coretypes bb_exception bb_exception
# run_rtgen ccoretypes coretypes coretypes binary_data_ptr binary_data_ptr
run_rtgen ccoretypes coretypes coretypes binarydata binarydata
# run_rtgen ccoretypes coretypes coretypes binarydata_factory binarydata_factory
# run_rtgen ccoretypes coretypes coretypes binarydata_impl binarydata_impl
# run_rtgen ccoretypes coretypes coretypes binarydata_ptr binarydata_ptr
run_rtgen ccoretypes coretypes coretypes boolean boolean
# run_rtgen ccoretypes coretypes coretypes boolean_factory boolean_factory
# run_rtgen ccoretypes coretypes coretypes boolean_impl boolean_impl
# run_rtgen ccoretypes coretypes coretypes callback callback
# run_rtgen ccoretypes coretypes coretypes cloneable cloneable
# run_rtgen ccoretypes coretypes coretypes common common
# run_rtgen ccoretypes coretypes coretypes comparable comparable
run_rtgen ccoretypes coretypes coretypes complex_number complex_number
# run_rtgen ccoretypes coretypes coretypes complex_number_factory complex_number_factory
# run_rtgen ccoretypes coretypes coretypes complex_number_impl complex_number_impl
# run_rtgen ccoretypes coretypes coretypes complex_number_ptr complex_number_ptr
# run_rtgen ccoretypes coretypes coretypes complex_number_type complex_number_type
# run_rtgen ccoretypes coretypes coretypes constexpr_string constexpr_string
# run_rtgen ccoretypes coretypes coretypes constexpr_utils constexpr_utils
run_rtgen ccoretypes coretypes coretypes convertible convertible
# run_rtgen ccoretypes coretypes coretypes coretype coretype
# run_rtgen ccoretypes coretypes coretypes coretype_traits coretype_traits
# run_rtgen ccoretypes coretypes coretypes coretype_utils coretype_utils
# run_rtgen ccoretypes coretypes coretypes coretypes coretypes
# run_rtgen coretypes coretypes coretypes coretypes_configc.h coretypes_config.h
# run_rtgen ccoretypes coretypes coretypes ctutils ctutils
# run_rtgen ccoretypes coretypes coretypes customalloc customalloc
# run_rtgen ccoretypes coretypes coretypes cycle_detector cycle_detector
# run_rtgen ccoretypes coretypes coretypes delegate delegate
# run_rtgen ccoretypes coretypes coretypes deserializer deserializer
# run_rtgen ccoretypes coretypes coretypes deserializer_ptr deserializer_ptr
# run_rtgen ccoretypes coretypes coretypes dict_element_type dict_element_type
run_rtgen ccoretypes coretypes coretypes enumeration enumeration
# run_rtgen ccoretypes coretypes coretypes enumeration_factory enumeration_factory
# run_rtgen ccoretypes coretypes coretypes enumeration_impl enumeration_impl
# run_rtgen ccoretypes coretypes coretypes enumeration_ptr enumeration_ptr
run_rtgen ccoretypes coretypes coretypes enumeration_type enumeration_type
# run_rtgen ccoretypes coretypes coretypes enumeration_type_factory enumeration_type_factory
# run_rtgen ccoretypes coretypes coretypes enumeration_type_impl enumeration_type_impl
# run_rtgen ccoretypes coretypes coretypes enumeration_type_ptr enumeration_type_ptr
# run_rtgen ccoretypes coretypes coretypes error_code_to_exception error_code_to_exception
# run_rtgen ccoretypes coretypes coretypes errorinfo errorinfo
# run_rtgen ccoretypes coretypes coretypes errorinfo_factory errorinfo_factory
# run_rtgen ccoretypes coretypes coretypes errorinfo_impl errorinfo_impl
# run_rtgen ccoretypes coretypes coretypes errors errors
run_rtgen ccoretypes coretypes coretypes event event
run_rtgen ccoretypes coretypes coretypes event_args event_args
# run_rtgen ccoretypes coretypes coretypes event_args_factory event_args_factory
# run_rtgen ccoretypes coretypes coretypes event_args_impl event_args_impl
# run_rtgen ccoretypes coretypes coretypes event_args_ptr event_args_ptr
# run_rtgen ccoretypes coretypes coretypes event_emitter event_emitter
# run_rtgen ccoretypes coretypes coretypes event_factory event_factory
run_rtgen ccoretypes coretypes coretypes event_handler event_handler
# run_rtgen ccoretypes coretypes coretypes event_handler_impl event_handler_impl
# run_rtgen ccoretypes coretypes coretypes event_handler_ptr event_handler_ptr
# run_rtgen ccoretypes coretypes coretypes event_impl event_impl
# run_rtgen ccoretypes coretypes coretypes event_ptr event_ptr
# run_rtgen ccoretypes coretypes coretypes event_wrapper event_wrapper
# run_rtgen ccoretypes coretypes coretypes exceptions exceptions
# run_rtgen ccoretypes coretypes coretypes factory factory
# run_rtgen ccoretypes coretypes coretypes factoryselectors factoryselectors
# run_rtgen ccoretypes coretypes coretypes filesystem filesystem
# run_rtgen ccoretypes coretypes coretypes float float
# run_rtgen ccoretypes coretypes coretypes float_factory float_factory
# run_rtgen ccoretypes coretypes coretypes float_impl float_impl
# run_rtgen ccoretypes coretypes coretypes formatter formatter
run_rtgen ccoretypes coretypes coretypes freezable freezable
# run_rtgen ccoretypes coretypes coretypes function function
# run_rtgen ccoretypes coretypes coretypes function_custom_impl function_custom_impl
# run_rtgen ccoretypes coretypes coretypes function_factory function_factory
# run_rtgen ccoretypes coretypes coretypes function_impl function_impl
# run_rtgen ccoretypes coretypes coretypes function_ptr function_ptr
# run_rtgen ccoretypes coretypes coretypes function_traits function_traits
# run_rtgen ccoretypes coretypes coretypes impl impl
# run_rtgen ccoretypes coretypes coretypes inspectable inspectable
# run_rtgen ccoretypes coretypes coretypes inspectable_ptr inspectable_ptr
run_rtgen ccoretypes coretypes coretypes integer integer
# run_rtgen ccoretypes coretypes coretypes integer_factory integer_factory
# run_rtgen ccoretypes coretypes coretypes integer_impl integer_impl
# run_rtgen ccoretypes coretypes coretypes intfid intfid
# run_rtgen ccoretypes coretypes coretypes intfs intfs
run_rtgen ccoretypes coretypes coretypes iterable iterable
# run_rtgen ccoretypes coretypes coretypes iterable_ptr iterable_ptr
run_rtgen ccoretypes coretypes coretypes iterator iterator
# run_rtgen ccoretypes coretypes coretypes iterator_base_impl iterator_base_impl
# run_rtgen ccoretypes coretypes coretypes iterator_helper iterator_helper
# run_rtgen ccoretypes coretypes coretypes iterator_support iterator_support
# run_rtgen ccoretypes coretypes coretypes json_deserializer json_deserializer
# run_rtgen ccoretypes coretypes coretypes json_deserializer_factory json_deserializer_factory
# run_rtgen ccoretypes coretypes coretypes json_deserializer_impl json_deserializer_impl
# run_rtgen ccoretypes coretypes coretypes json_serialized_list json_serialized_list
# run_rtgen ccoretypes coretypes coretypes json_serialized_object json_serialized_object
# run_rtgen ccoretypes coretypes coretypes json_serializer json_serializer
# run_rtgen ccoretypes coretypes coretypes json_serializer_factory json_serializer_factory
# run_rtgen ccoretypes coretypes coretypes json_serializer_impl json_serializer_impl
# run_rtgen ccoretypes coretypes coretypes list_element_type list_element_type
# run_rtgen ccoretypes coretypes coretypes macro_utils macro_utils
# run_rtgen ccoretypes coretypes coretypes mem mem
# run_rtgen ccoretypes coretypes coretypes multi_ptr multi_ptr
run_rtgen ccoretypes coretypes coretypes number number
# run_rtgen ccoretypes coretypes coretypes number_impl number_impl
# run_rtgen ccoretypes coretypes coretypes number_ptr number_ptr
# run_rtgen ccoretypes coretypes coretypes object_decorator object_decorator
# run_rtgen ccoretypes coretypes coretypes objectptr objectptr
# run_rtgen ccoretypes coretypes coretypes ordinalobject_impl ordinalobject_impl
# run_rtgen ccoretypes coretypes coretypes procedure procedure
# run_rtgen ccoretypes coretypes coretypes procedure_custom_impl procedure_custom_impl
# run_rtgen ccoretypes coretypes coretypes procedure_factory procedure_factory
# run_rtgen ccoretypes coretypes coretypes procedure_impl procedure_impl
# run_rtgen ccoretypes coretypes coretypes procedure_ptr procedure_ptr
run_rtgen ccoretypes coretypes coretypes ratio ratio
# run_rtgen ccoretypes coretypes coretypes ratio_factory ratio_factory
# run_rtgen ccoretypes coretypes coretypes ratio_impl ratio_impl
# run_rtgen ccoretypes coretypes coretypes ratio_ptr ratio_ptr
run_rtgen ccoretypes coretypes coretypes serializable serializable
# run_rtgen ccoretypes coretypes coretypes serializable_ptr serializable_ptr
# run_rtgen ccoretypes coretypes coretypes serialization serialization
run_rtgen ccoretypes coretypes coretypes serialized_list serialized_list
# run_rtgen ccoretypes coretypes coretypes serialized_list_ptr serialized_list_ptr
run_rtgen ccoretypes coretypes coretypes serialized_object serialized_object
# run_rtgen ccoretypes coretypes coretypes serialized_object_ptr serialized_object_ptr
# run_rtgen ccoretypes coretypes coretypes serializer serializer
# run_rtgen ccoretypes coretypes coretypes serializer_ptr serializer_ptr
# run_rtgen ccoretypes coretypes coretypes sha1 sha1
run_rtgen ccoretypes coretypes coretypes simple_type simple_type
# run_rtgen ccoretypes coretypes coretypes simple_type_factory simple_type_factory
# run_rtgen ccoretypes coretypes coretypes simple_type_impl simple_type_impl
# run_rtgen ccoretypes coretypes coretypes simplified_ratio_ptr simplified_ratio_ptr
# run_rtgen ccoretypes coretypes coretypes span span
# run_rtgen ccoretypes coretypes coretypes string_ptr string_ptr
run_rtgen ccoretypes coretypes coretypes stringobject stringobject
# run_rtgen ccoretypes coretypes coretypes stringobject_factory stringobject_factory
# run_rtgen ccoretypes coretypes coretypes stringobject_impl stringobject_impl
run_rtgen ccoretypes coretypes coretypes struct struct
# run_rtgen ccoretypes coretypes coretypes struct_builder struct_builder
# run_rtgen ccoretypes coretypes coretypes struct_builder_impl struct_builder_impl
# run_rtgen ccoretypes coretypes coretypes struct_builder_ptr struct_builder_ptr
# run_rtgen ccoretypes coretypes coretypes struct_factory struct_factory
# run_rtgen ccoretypes coretypes coretypes struct_impl struct_impl
# run_rtgen ccoretypes coretypes coretypes struct_ptr struct_ptr
run_rtgen ccoretypes coretypes coretypes struct_type struct_type
# run_rtgen ccoretypes coretypes coretypes struct_type_factory struct_type_factory
# run_rtgen ccoretypes coretypes coretypes struct_type_impl struct_type_impl
# run_rtgen ccoretypes coretypes coretypes struct_type_ptr struct_type_ptr
run_rtgen ccoretypes coretypes coretypes type type
# run_rtgen ccoretypes coretypes coretypes type_impl type_impl
run_rtgen ccoretypes coretypes coretypes type_manager type_manager
# run_rtgen ccoretypes coretypes coretypes type_manager_factory type_manager_factory
# run_rtgen ccoretypes coretypes coretypes type_manager_impl type_manager_impl
# run_rtgen ccoretypes coretypes coretypes type_manager_private type_manager_private
# run_rtgen ccoretypes coretypes coretypes type_manager_ptr type_manager_ptr
# run_rtgen ccoretypes coretypes coretypes type_name type_name
# run_rtgen ccoretypes coretypes coretypes type_name_detail type_name_detail
# run_rtgen ccoretypes coretypes coretypes type_ptr type_ptr
run_rtgen ccoretypes coretypes coretypes updatable updatable
# run_rtgen ccoretypes coretypes coretypes updatable_ptr updatable_ptr
# run_rtgen ccoretypes coretypes coretypes utility_sync utility_sync
# run_rtgen ccoretypes coretypes coretypes validation validation
# run_rtgen ccoretypes coretypes coretypes version version
run_rtgen ccoretypes coretypes coretypes version_info version_info
# run_rtgen ccoretypes coretypes coretypes version_info_factory version_info_factory
# run_rtgen ccoretypes coretypes coretypes version_info_impl version_info_impl
# run_rtgen ccoretypes coretypes coretypes version_info_ptr version_info_ptr
# run_rtgen ccoretypes coretypes coretypes weakref weakref
# run_rtgen ccoretypes coretypes coretypes weakref_impl weakref_impl
# run_rtgen ccoretypes coretypes coretypes weakrefobj weakrefobj
# run_rtgen ccoretypes coretypes coretypes weakrefptr weakrefptr

# run_rtgen ccoretypes corecontainers coretypes dict_ptr dict_ptr
run_rtgen ccoretypes corecontainers coretypes dictobject dictobject
# run_rtgen ccoretypes corecontainers coretypes dictobject_factory dictobject_factory
# run_rtgen ccoretypes corecontainers coretypes dictobject_impl dictobject_impl
# run_rtgen ccoretypes corecontainers coretypes dictobject_iterable_impl dictobject_iterable_impl
# run_rtgen ccoretypes corecontainers coretypes dictobject_iterator_impl dictobject_iterator_impl
# run_rtgen ccoretypes corecontainers coretypes dictptr dictptr
# run_rtgen ccoretypes corecontainers coretypes list_factory list_factory
# run_rtgen ccoretypes corecontainers coretypes list_ptr list_ptr
run_rtgen ccoretypes corecontainers coretypes listobject listobject
# run_rtgen ccoretypes corecontainers coretypes listobject_factory listobject_factory
# run_rtgen ccoretypes corecontainers coretypes listobject_impl listobject_impl
# run_rtgen ccoretypes corecontainers coretypes listptr listptr




# run_rtgen copendaq coretypes coretypes base_object base 


# #
# # core types
# #
# core_type_files=(
# # "event"
# # "event_args"
# # "event_handler"
# "simple_type"
# #"struct"
# "struct_type"
# "type"
# #"type_manager"
# "struct_builder"
# #"enumeration"
# #"enumeration_type"
# )
# for file in "${core_type_files[@]}" 
# do
#     run_rtgen CoreTypes coretypes coretypes core_types/generated ${file}
# done

# run_rtgen CoreTypes coretypes coretypes core_types/generated version_info 
 
 