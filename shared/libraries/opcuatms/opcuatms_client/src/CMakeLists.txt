set(LIB_NAME opcuatms_client)

set(SRC_Cpp tms_client.cpp
            tms_attribute_collector.cpp
)

set(SRC_PublicHeaders
)

set(SRC_PrivateHeaders tms_client.h
                       tms_attribute_collector.h
)

# objects

set(OBJECT_SRC_DIR "objects")

set(SRC_Objects_Headers ${OBJECT_SRC_DIR}/tms_client_object_impl.h
                        ${OBJECT_SRC_DIR}/tms_client_context.h

                        ${OBJECT_SRC_DIR}/tms_client_signal_impl.h
                        ${OBJECT_SRC_DIR}/tms_client_signal_factory.h

                        ${OBJECT_SRC_DIR}/tms_client_function_block_impl.h
                        ${OBJECT_SRC_DIR}/tms_client_function_block_factory.h

                        ${OBJECT_SRC_DIR}/tms_client_function_block_type_impl.h
                        ${OBJECT_SRC_DIR}/tms_client_function_block_type_factory.h

                        ${OBJECT_SRC_DIR}/tms_client_channel_impl.h
                        ${OBJECT_SRC_DIR}/tms_client_channel_factory.h

                        ${OBJECT_SRC_DIR}/tms_client_property_impl.h
                        ${OBJECT_SRC_DIR}/tms_client_property_factory.h

                        ${OBJECT_SRC_DIR}/tms_client_input_port_impl.h
                        ${OBJECT_SRC_DIR}/tms_client_input_port_factory.h

                        ${OBJECT_SRC_DIR}/tms_client_property_object_impl.h
                        ${OBJECT_SRC_DIR}/tms_client_property_object_factory.h

                        ${OBJECT_SRC_DIR}/tms_client_device_impl.h
                        ${OBJECT_SRC_DIR}/tms_client_device_factory.h

                        ${OBJECT_SRC_DIR}/tms_client_folder_impl.h
                        ${OBJECT_SRC_DIR}/tms_client_folder_factory.h
						
                        ${OBJECT_SRC_DIR}/tms_client_io_folder_impl.h
                        ${OBJECT_SRC_DIR}/tms_client_io_folder_factory.h

                        ${OBJECT_SRC_DIR}/tms_client_component.h
                        ${OBJECT_SRC_DIR}/tms_client_component_impl.h
                        ${OBJECT_SRC_DIR}/tms_client_component_factory.h

                        ${OBJECT_SRC_DIR}/tms_client_server_capability_impl.h
                        ${OBJECT_SRC_DIR}/tms_client_server_capability_factory.h
						
                        ${OBJECT_SRC_DIR}/tms_client_function_impl.h
                        ${OBJECT_SRC_DIR}/tms_client_function_factory.h

                        ${OBJECT_SRC_DIR}/tms_client_procedure_impl.h
                        ${OBJECT_SRC_DIR}/tms_client_procedure_factory.h
						
                        ${OBJECT_SRC_DIR}/tms_client_tags_impl.h
                        ${OBJECT_SRC_DIR}/tms_client_tags_factory.h
)

set(SRC_Objects ${OBJECT_SRC_DIR}/tms_client_object_impl.cpp
                ${OBJECT_SRC_DIR}/tms_client_context.cpp
                ${OBJECT_SRC_DIR}/tms_client_signal_impl.cpp
                ${OBJECT_SRC_DIR}/tms_client_function_block_impl.cpp
                ${OBJECT_SRC_DIR}/tms_client_channel_impl.cpp
                ${OBJECT_SRC_DIR}/tms_client_input_port_impl.cpp
                ${OBJECT_SRC_DIR}/tms_client_property_impl.cpp
                ${OBJECT_SRC_DIR}/tms_client_property_object_impl.cpp
                ${OBJECT_SRC_DIR}/tms_client_device_impl.cpp
                ${OBJECT_SRC_DIR}/tms_client_folder_impl.cpp
                ${OBJECT_SRC_DIR}/tms_client_io_folder_impl.cpp
                ${OBJECT_SRC_DIR}/tms_client_component_impl.cpp
                ${OBJECT_SRC_DIR}/tms_client_server_capability_impl.cpp
                ${OBJECT_SRC_DIR}/tms_client_function_impl.cpp
                ${OBJECT_SRC_DIR}/tms_client_procedure_impl.cpp
                ${OBJECT_SRC_DIR}/tms_client_tags_impl.cpp
                ${OBJECT_SRC_DIR}/tms_client_function_block_type_impl.cpp
)

set(SRC_PublicHeaders ${SRC_PublicHeaders} ${SRC_Objects_Headers})
set(SRC_Cpp ${SRC_Cpp} ${SRC_Objects})

source_group("objects\\client_object" "${OBJECT_SRC_DIR}/(tms_client_object.*|tms_client_context.*)")
source_group("objects\\signal" "${OBJECT_SRC_DIR}/tms_client_signal.*")
source_group("objects\\function_block" "${OBJECT_SRC_DIR}/tms_client_function_block.*")
source_group("objects\\channel" "${OBJECT_SRC_DIR}/tms_client_channel.*")
source_group("objects\\input_port" "${OBJECT_SRC_DIR}/tms_client_input_port.*")
source_group("objects\\property" "${OBJECT_SRC_DIR}/tms_client_property.*")
source_group("objects\\property_object" "${OBJECT_SRC_DIR}/tms_client_property_object.*")
source_group("objects\\device" "${OBJECT_SRC_DIR}/tms_client_device.*")
source_group("objects\\folder" "${OBJECT_SRC_DIR}/tms_client_folder.*")
source_group("objects\\component" "${OBJECT_SRC_DIR}/tms_client_component.*")
source_group("objects\\io_folder" "${OBJECT_SRC_DIR}/tms_client_io_folder.*")
source_group("objects\\server_capability" "${OBJECT_SRC_DIR}/tms_client_server_capability.*")
source_group("objects\\tags" "${OBJECT_SRC_DIR}/tms_client_tags.*")
source_group("objects\\function" "${OBJECT_SRC_DIR}/(tms_client_function_impl.*|tms_client_function_factory.h|tms_client_procedure.*)")

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

if (MSVC)
    target_compile_options(${LIB_NAME} PRIVATE /bigobj)
elseif (MINGW AND CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options(${LIB_NAME} PRIVATE -Wa,-mbig-obj)
endif()

target_link_libraries(${LIB_NAME}
        PUBLIC
            daq::opcuatms
            daq::opcuaclient
        PRIVATE
            daq::signal_dev
            daq::streaming_dev
            daq::device_dev
            daq::component_dev
)

target_include_directories(${LIB_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
                                              $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include>

                                              $<INSTALL_INTERFACE:include>
)

set_target_properties(${LIB_NAME} PROPERTIES PUBLIC_HEADER "${SRC_PublicHeaders}")

opendaq_set_output_lib_name(${LIB_NAME} ${PROJECT_VERSION_MAJOR})
