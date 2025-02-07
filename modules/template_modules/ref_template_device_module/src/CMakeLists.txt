set(LIB_NAME ref_template_template_device_module)
set(MODULE_HEADERS_DIR ../include/${TARGET_FOLDER_NAME})

set(SRC_Include common.h
                module_dll.h
                ref_template_template_device_module_impl.h
                ref_template_template_device_impl.h
                ref_template_template_channel_impl.h
				ref_template_template_can_channel_impl.h
)

set(SRC_Srcs module_dll.cpp
             ref_template_template_device_module_impl.cpp
             ref_template_template_device_impl.cpp
             ref_template_template_channel_impl.cpp
			 ref_template_template_can_channel_impl.cpp
)

prepend_include(${TARGET_FOLDER_NAME} SRC_Include)

source_group("module" FILES ${MODULE_HEADERS_DIR}/ref_template_template_device_module_common.h
                            ${MODULE_HEADERS_DIR}/ref_template_template_device_module_impl.h
                            ${MODULE_HEADERS_DIR}/ref_template_template_channel_impl.h
                            ${MODULE_HEADERS_DIR}/ref_template_template_can_channel_impl.h
                            ${MODULE_HEADERS_DIR}/ref_template_template_device_impl.h
                            ${MODULE_HEADERS_DIR}/module_dll.h
                            module_dll.cpp
                            ref_template_template_device_module_impl.cpp
                            ref_template_template_device_impl.cpp
                            ref_template_template_channel_impl.cpp
							ref_template_template_can_channel_impl.cpp
)


add_library(${LIB_NAME} SHARED ${SRC_Include}
                               ${SRC_Srcs}
)

add_library(${SDK_TARGET_NAMESPACE}::${LIB_NAME} ALIAS ${LIB_NAME})

if (MSVC)
    target_compile_options(${LIB_NAME} PRIVATE /bigobj)
endif()

target_link_libraries(${LIB_NAME} PUBLIC daq::daqmodule_template
)

target_include_directories(${LIB_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
                                              $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include>
                                              $<INSTALL_INTERFACE:include>
)

opendaq_set_module_properties(${LIB_NAME} ${PROJECT_VERSION_MAJOR})
create_version_header(${LIB_NAME})
