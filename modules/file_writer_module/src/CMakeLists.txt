set(LIB_NAME file_writer_module)
set(MODULE_HEADERS_DIR ../include/${TARGET_FOLDER_NAME})
set(MODULE_BINARY_HEADER_DIR ${CMAKE_CURRENT_BINARY_DIR}/../include/${TARGET_FOLDER_NAME})

set(SRC_Include common.h
                module_dll.h
                file_writer_module_impl.h
                parquet_fb_impl.h

)

set(SRC_Srcs module_dll.cpp
             file_writer_module_impl.cpp
             parquet_fb_impl.cpp
)

prepend_include(${TARGET_FOLDER_NAME} SRC_Include)

set(MODULE_FILES ${MODULE_HEADERS_DIR}/file_writer_module_common.h
                            ${MODULE_HEADERS_DIR}/file_writer_module_impl.h
                            ${MODULE_HEADERS_DIR}/module_dll.h
                            module_dll.cpp
                            file_writer_module_impl.cpp
                            fft_parquet_fb_impl.cpp)

source_group("module" FILES ${MODULE_FILES})

add_library(${LIB_NAME} SHARED ${SRC_Include}
                               ${SRC_Srcs}
                               
)

add_library(${SDK_TARGET_NAMESPACE}::${LIB_NAME} ALIAS ${LIB_NAME})
 
target_link_libraries(${LIB_NAME} PUBLIC  daq::opendaq
                                  PRIVATE Arrow::arrow_static parquet_static
)

target_include_directories(${LIB_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
                                              $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include>
                                              $<BUILD_INTERFACE:${arrow_SOURCE_DIR}/cpp/src>
                                              $<INSTALL_INTERFACE:include>
)

opendaq_set_module_properties(${LIB_NAME} ${PROJECT_VERSION_MAJOR})
create_version_header(${LIB_NAME})
