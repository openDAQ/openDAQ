set(LIB_NAME ref_fb_module)
set(MODULE_HEADERS_DIR ../include/${TARGET_FOLDER_NAME})
set(MODULE_BINARY_HEADER_DIR ${CMAKE_CURRENT_BINARY_DIR}/../include/${TARGET_FOLDER_NAME})

include(Resources)
embed_resource("arial.ttf" ${MODULE_BINARY_HEADER_DIR}/arial.ttf.h "ARIAL_TTF")

set(SRC_Include common.h
                module_dll.h
                ref_fb_module_impl.h
                renderer_fb_impl.h
                polyline.h
                polyline_impl.h
                polyline_impl_generic.h
                polyline_impl_line.h
                power_fb_impl.h
                statistics_fb_impl.h
                scaling_fb_impl.h
                classifier_fb_impl.h
                dispatch.h
                trigger_fb_impl.h
)

set(SRC_Srcs module_dll.cpp
             ref_fb_module_impl.cpp
             renderer_fb_impl.cpp
             polyline.cpp
             polyline_impl.cpp
             polyline_impl_generic.cpp
             polyline_impl_line.cpp
             power_fb_impl.cpp
             statistics_fb_impl.cpp
             classifier_fb_impl.cpp
             scaling_fb_impl.cpp
             trigger_fb_impl.cpp
)

prepend_include(${TARGET_FOLDER_NAME} SRC_Include)

source_group("module" FILES ${MODULE_HEADERS_DIR}/ref_fb_module_common.h
                            ${MODULE_HEADERS_DIR}/ref_fb_module_impl.h
                            ${MODULE_HEADERS_DIR}/renderer_fb_impl.h
                            ${MODULE_HEADERS_DIR}/power_fb_impl.h
                            ${MODULE_HEADERS_DIR}/statistics_fb_impl.h
                            ${MODULE_HEADERS_DIR}/module_dll.h
                            ${MODULE_HEADERS_DIR}/scaling_fb_impl.h
                            ${MODULE_HEADERS_DIR}/dispatch.h
                            ${MODULE_HEADERS_DIR}/trigger_fb_impl.h
                            ${MODULE_BINARY_HEADER_DIR}/arial.ttf.h
                            module_dll.cpp
                            ref_fb_module_impl.cpp
                            renderer_fb_impl.cpp
                            power_fb_impl.cpp
                            statistics_fb_impl.cpp
                            scaling_fb_impl.cpp
                            classifier_fb_impl.cpp
                            trigger_fb_impl.cpp
)

source_group("polyline" FILES ${MODULE_HEADERS_DIR}/polyline.h
                              ${MODULE_HEADERS_DIR}/polyline_impl.h
                              ${MODULE_HEADERS_DIR}/polyline_impl_generic.h
                              ${MODULE_HEADERS_DIR}/polyline_impl_line.h
                              polyline.cpp
                              polyline_impl.cpp
                              polyline_impl_generic.cpp
                              polyline_impl_line.cpp
)

add_library(${LIB_NAME} SHARED ${SRC_Include}
                               ${SRC_Srcs}
                               ${MODULE_BINARY_HEADER_DIR}/arial.ttf.h
)

add_library(${SDK_TARGET_NAMESPACE}::${LIB_NAME} ALIAS ${LIB_NAME})

target_link_libraries(${LIB_NAME} PUBLIC daq::opendaq
                                  PRIVATE sfml-graphics
                                          sfml-window
                                          date::date
)

target_include_directories(${LIB_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
                                              $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include>
                                              $<INSTALL_INTERFACE:include>
)

opendaq_set_module_properties(${LIB_NAME} ${PROJECT_VERSION_MAJOR})
create_version_header(${LIB_NAME})

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/arial.ttf
        DESTINATION ${CMAKE_INSTALL_BINDIR}
)
