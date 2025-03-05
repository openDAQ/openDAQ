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
    dest_dir_headers="${BINDINGS_DIR}/include"
    dest_dir_sources="${BINDINGS_DIR}/src"

    # ./rtgen.exe -v --language=c --library=copendaq --namespace=daq --source=float.h --outputDir=out --output=float1 --extension=.cpp

    command_h="${RTGEN} -v --language=c --library=${lib_name} --namespace=daq --source=${source_dir}/${file_h} --outputDir=${dest_dir_headers} --output=${out_name} --extension=.h"
    command_cpp="${RTGEN} -v --language=c --library=${lib_name} --namespace=daq --source=${source_dir}/${file_h} --outputDir=${dest_dir_sources} --output=${out_name} --extension=.cpp"
    
    echo "$command_h"
    $command_h
    echo "$command_cpp"
    $command_cpp
}

run_rtgen copendaq coretypes coretypes baseobject base 
run_rtgen copendaq coretypes coretypes boolean boolean 
run_rtgen copendaq coretypes coretypes complex_number complex_number 
# run_rtgen copendaq coretypes coretypes converter converter

run_rtgen copendaq corecontainers coretypes dictobject dictobject 
run_rtgen copendaq coretypes coretypes float float1
run_rtgen copendaq coretypes coretypes function function 
run_rtgen copendaq coretypes coretypes integer integer1

run_rtgen copendaq coretypes coretypes iterable iterable 
run_rtgen copendaq coretypes coretypes iterator iterator 
run_rtgen copendaq corecontainers coretypes listobject listobject 
run_rtgen copendaq coretypes coretypes number number1 

# run_rtgen copendaq coretypes coretypes opendaq_daq opendaq_daq 
run_rtgen copendaq coretypes coretypes procedure procedure 
run_rtgen copendaq coretypes coretypes ratio ratio 
run_rtgen copendaq coretypes coretypes stringobject stringobject 

# run_rtgen copendaq coretypes coretypes opendaq_daq opendaq_daq 
# run_rtgen copendaq coretypes coretypes procedure procedure 
# run_rtgen copendaq coretypes coretypes ratio ratio 
# run_rtgen copendaq coretypes coretypes string string 

# run_rtgen copendaq coretypes coretypes opendaq_daq opendaq_daq 
# run_rtgen copendaq coretypes coretypes procedure procedure 
# run_rtgen copendaq coretypes coretypes ratio ratio 
# run_rtgen copendaq coretypes coretypes string string 

# run_rtgen copendaq coretypes coretypes opendaq_daq opendaq_daq 
# run_rtgen copendaq coretypes coretypes procedure procedure 
# run_rtgen copendaq coretypes coretypes ratio ratio 
# run_rtgen copendaq coretypes coretypes string string 

# run_rtgen copendaq coretypes coretypes opendaq_daq opendaq_daq 
# run_rtgen copendaq coretypes coretypes procedure procedure 
# run_rtgen copendaq coretypes coretypes ratio ratio 
# run_rtgen copendaq coretypes coretypes string string 



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
 