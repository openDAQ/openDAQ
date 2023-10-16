set(LIB_NAME opcuatms_server)

set(SRC_Cpp tms_server.cpp
)

set(SRC_PublicHeaders
)

set(SRC_PrivateHeaders tms_server.h
)

# objects

set(OBJECT_SRC_DIR "objects")

set(SRC_Objects_Headers ${OBJECT_SRC_DIR}/tms_server_object.h
                        ${OBJECT_SRC_DIR}/tms_server_variable.h
                        ${OBJECT_SRC_DIR}/tms_server_signal.h
                        ${OBJECT_SRC_DIR}/tms_server_function_block.h
                        ${OBJECT_SRC_DIR}/tms_server_channel.h
                        ${OBJECT_SRC_DIR}/tms_server_input_port.h
                        ${OBJECT_SRC_DIR}/tms_server_property_object.h
                        ${OBJECT_SRC_DIR}/tms_server_device.h
                        ${OBJECT_SRC_DIR}/tms_server_folder.h
                        ${OBJECT_SRC_DIR}/tms_server_component.h
                        ${OBJECT_SRC_DIR}/tms_server_property.h
                        ${OBJECT_SRC_DIR}/tms_server_eval_value.h
)

set(SRC_Objects ${OBJECT_SRC_DIR}/tms_server_object.cpp
                ${OBJECT_SRC_DIR}/tms_server_variable.cpp
                ${OBJECT_SRC_DIR}/tms_server_signal.cpp
                ${OBJECT_SRC_DIR}/tms_server_function_block.cpp
                ${OBJECT_SRC_DIR}/tms_server_channel.cpp
                ${OBJECT_SRC_DIR}/tms_server_input_port.cpp
                ${OBJECT_SRC_DIR}/tms_server_property_object.cpp
                ${OBJECT_SRC_DIR}/tms_server_device.cpp
                ${OBJECT_SRC_DIR}/tms_server_folder.cpp
                ${OBJECT_SRC_DIR}/tms_server_component.cpp
                ${OBJECT_SRC_DIR}/tms_server_property.cpp
                ${OBJECT_SRC_DIR}/tms_server_eval_value.cpp
)

set(SRC_PublicHeaders ${SRC_PublicHeaders} ${SRC_Objects_Headers})
set(SRC_Cpp ${SRC_Cpp} ${SRC_Objects})

source_group("objects\\server_object" "${OBJECT_SRC_DIR}/tms_server_object.*")
source_group("objects\\server_variable" "${OBJECT_SRC_DIR}/tms_server_variable.*")
source_group("objects\\signal" "${OBJECT_SRC_DIR}/tms_server_signal.*")
source_group("objects\\function_block" "${OBJECT_SRC_DIR}/tms_server_function_block.*")
source_group("objects\\channel" "${OBJECT_SRC_DIR}/tms_server_channel.*")
source_group("objects\\input_port" "${OBJECT_SRC_DIR}/tms_server_input_port.*")
source_group("objects\\property_object" "${OBJECT_SRC_DIR}/tms_server_property_object.*")
source_group("objects\\device" "${OBJECT_SRC_DIR}/tms_server_device.*")
source_group("objects\\folder" "${OBJECT_SRC_DIR}/tms_server_folder.*")
source_group("objects\\component" "${OBJECT_SRC_DIR}/tms_server_component.*")
source_group("objects\\property" "${OBJECT_SRC_DIR}/tms_server_property.*")
source_group("objects\\eval_value" "${OBJECT_SRC_DIR}/tms_server_eval_value.*")

# /objects

prepend_include(${LIB_NAME} SRC_PrivateHeaders)
prepend_include(${LIB_NAME} SRC_PublicHeaders)


add_library(${LIB_NAME} STATIC ${SRC_Cpp}
                               ${SRC_PublicHeaders}
                               ${SRC_PrivateHeaders}
)

add_library(${SDK_TARGET_NAMESPACE}::${LIB_NAME} ALIAS ${LIB_NAME})

if(BUILD_64Bit OR BUILD_ARM)
    set_target_properties(${LIB_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
else()
    set_target_properties(${LIB_NAME} PROPERTIES POSITION_INDEPENDENT_CODE OFF)
endif()

target_include_directories(${LIB_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
                                              $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include>

                                              $<INSTALL_INTERFACE:include>
)

target_link_libraries(${LIB_NAME} PUBLIC daq::opcuatms
                                         daq::opcuaserver
)

set_target_properties(${LIB_NAME} PROPERTIES PUBLIC_HEADER "${SRC_PublicHeaders}")

opendaq_set_output_lib_name(${LIB_NAME} ${PROJECT_VERSION_MAJOR})
