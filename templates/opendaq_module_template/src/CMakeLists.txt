set(BASE_NAME module_template)
set(LIB_NAME ${SDK_TARGET_NAMESPACE}${BASE_NAME})

set(MODULE_HEADERS_DIR ../include/${TARGET_FOLDER_NAME})

set(SRC_Include
    channel_template.h
    device_template.h
    function_block_template.h
    module_template.h
    io_folder_template.h
    component_template.h
    component_template_base.h
    component_helpers.h
    utils.h
)

set(SRC_Srcs
    channel_template.cpp
    device_template.cpp
    function_block_template.cpp
    module_template.cpp
    component_template_base.cpp
    component_helpers.cpp
    io_folder_template.cpp
    component_template.cpp
    utils.cpp
)

prepend_include(${TARGET_FOLDER_NAME} SRC_Include)

source_group("templates" FILES ${MODULE_HEADERS_DIR}/channel_template.h
                               ${MODULE_HEADERS_DIR}/component_template_base.h
                               ${MODULE_HEADERS_DIR}/function_block_template.h
                               ${MODULE_HEADERS_DIR}/module_template.h
                               ${MODULE_HEADERS_DIR}/device_template.h
                               ${MODULE_HEADERS_DIR}/io_folder_template.h
                               ${MODULE_HEADERS_DIR}/component_template.h
                               channel_template.cpp
                               component_template_base.cpp
                               function_block_template.cpp
                               module_template.cpp
                               device_template.cpp
                               component_template.cpp
                               io_folder_template.cpp
)

add_library(${LIB_NAME} STATIC ${SRC_Include}
                               ${SRC_Srcs}
)

add_library(${SDK_TARGET_NAMESPACE}::${LIB_NAME} ALIAS ${LIB_NAME})

target_link_libraries(${LIB_NAME}
        PUBLIC
            daq::opendaq
)

if (CMAKE_COMPILER_IS_GNUCXX AND MINGW)
    target_compile_options(${LIB_NAME} PRIVATE -Wno-error)
endif()

target_include_directories(${LIB_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
                                              $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include>
                                              $<INSTALL_INTERFACE:include>
                                              ${Boost_INCLUDE_DIRS}
)

opendaq_set_output_lib_name(${LIB_NAME} ${PROJECT_VERSION_MAJOR})

install(TARGETS ${LIB_NAME}
        EXPORT ${SDK_NAME}
        RUNTIME
            DESTINATION ${CMAKE_INSTALL_BINDIR}
            COMPONENT ${SDK_NAME}_${BASE_NAME}_Runtime
        LIBRARY
            DESTINATION ${CMAKE_INSTALL_LIBDIR}
            COMPONENT          ${SDK_NAME}_${BASE_NAME}_Runtime
            NAMELINK_COMPONENT ${SDK_NAME}_${BASE_NAME}_Development
        ARCHIVE
            DESTINATION ${CMAKE_INSTALL_LIBDIR}
            COMPONENT ${SDK_NAME}_${BASE_NAME}_Development
        PUBLIC_HEADER
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${BASE_NAME}
            COMPONENT ${SDK_NAME}_${BASE_NAME}_Development
)
