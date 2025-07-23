set(LIB_NAME parquet_recorder_module)
set(MODULE_HEADERS_DIR ../include/${TARGET_FOLDER_NAME})

set(SRC_Include module_dll.h
    common.h
    parquet_recorder_module_impl.h
    parquet_recorder_impl.h
    parquet_writer.h
    type_resolver.h
)

set(SRC_Srcs module_dll.cpp
    parquet_recorder_module_impl.cpp
    parquet_recorder_impl.cpp
    parquet_writer.cpp
)

prepend_include(${TARGET_FOLDER_NAME} SRC_Include)

source_group("module" FILES ${MODULE_HEADERS_DIR}/parquet_recorder_module_impl.h
    ${MODULE_HEADERS_DIR}/module_dll.h
    ${SRC_Srcs}
)


add_library(${LIB_NAME} SHARED ${SRC_Include}
    ${SRC_Srcs}
)
add_library(${SDK_TARGET_NAMESPACE}::${LIB_NAME} ALIAS ${LIB_NAME})

if(MSVC)
    target_compile_options(${LIB_NAME} PRIVATE /bigobj)
endif()

target_link_libraries(${LIB_NAME} PRIVATE Boost::algorithm PUBLIC daq::opendaq Arrow::arrow_static parquet_static
)

target_include_directories(${LIB_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include>
    $<INSTALL_INTERFACE:include>
)

opendaq_set_module_properties(${LIB_NAME} ${PROJECT_VERSION_MAJOR})
create_version_header(${LIB_NAME})

