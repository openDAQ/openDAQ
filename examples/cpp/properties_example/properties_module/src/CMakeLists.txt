set(LIB_NAME properties_module)
set(MODULE_HEADERS_DIR ../include/${TARGET_FOLDER_NAME})

set(SRC_Include common.h
                module_dll.h
                properties_module.h
                properties_fb_1.h
                properties_fb_2.h
                properties_fb_3.h
                properties_fb_4.h
)

set(SRC_Srcs module_dll.cpp
             properties_module.cpp
             properties_fb_1.cpp
             properties_fb_2.cpp
             properties_fb_3.cpp
             properties_fb_4.cpp
)

prepend_include(${TARGET_FOLDER_NAME} SRC_Include)

source_group("module" FILES ${MODULE_HEADERS_DIR}/common.h
                            ${MODULE_HEADERS_DIR}/properties_module.h
                            ${MODULE_HEADERS_DIR}/properties_fb_1.h
                            ${MODULE_HEADERS_DIR}/properties_fb_2.h
                            ${MODULE_HEADERS_DIR}/properties_fb_3.h
                            ${MODULE_HEADERS_DIR}/properties_fb_4.h
                            ${MODULE_HEADERS_DIR}/module_dll.h
                            module_dll.cpp
                            properties_module.cpp
                            properties_fb_1.cpp
                            properties_fb_2.cpp
                            properties_fb_3.cpp
                            properties_fb_4.cpp
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
set_target_properties(${LIB_NAME} PROPERTIES FOLDER ${PROPERTIES_FOLDER_PATH})
