set(LIB_NAME property_system_example_module)
set(MODULE_HEADERS_DIR ../include/${TARGET_FOLDER_NAME})

set(SRC_Include common.h
                module_dll.h
                property_system_example_module.h
                properties_fb_basic_types.h
                properties_fb_container_types.h
                properties_fb_objects_and_classes.h
                properties_fb_reference_properties.h
)

set(SRC_Srcs module_dll.cpp
             property_system_example_module.cpp
             properties_fb_basic_types.cpp
             properties_fb_container_types.cpp
             properties_fb_objects_and_classes.cpp
             properties_fb_reference_properties.cpp
)

prepend_include(${TARGET_FOLDER_NAME} SRC_Include)

source_group("module" FILES ${MODULE_HEADERS_DIR}/common.h
                            ${MODULE_HEADERS_DIR}/property_system_example_module.h
                            ${MODULE_HEADERS_DIR}/properties_fb_basic_types.h
                            ${MODULE_HEADERS_DIR}/properties_fb_container_types.h
                            ${MODULE_HEADERS_DIR}/properties_fb_objects_and_classes.h
                            ${MODULE_HEADERS_DIR}/properties_fb_reference_properties.h
                            ${MODULE_HEADERS_DIR}/module_dll.h
                            module_dll.cpp
                            property_system_example_module.cpp
                            properties_fb_basic_types.cpp
                            properties_fb_container_types.cpp
                            properties_fb_objects_and_classes.cpp
                            properties_fb_reference_properties.cpp
)


add_library(${LIB_NAME} SHARED ${SRC_Include}
                               ${SRC_Srcs}
)


add_library(${SDK_TARGET_NAMESPACE}::${LIB_NAME} ALIAS ${LIB_NAME})

if (MSVC)
    target_compile_options(${LIB_NAME} PRIVATE /bigobj)
endif()

if(NOT TARGET daq::opendaq) 
    find_package(openDAQ) 
endif()

target_link_libraries(${LIB_NAME} PUBLIC daq::opendaq
)

target_include_directories(${LIB_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
                                              $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include>
                                              $<INSTALL_INTERFACE:include>
)

opendaq_set_module_properties(${LIB_NAME} ${PROJECT_VERSION_MAJOR})
create_version_header(${LIB_NAME})
