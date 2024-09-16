set(BASE_NAME idds)
set(LIB_NAME ${SDK_TARGET_NAME}_${BASE_NAME})

set(SRC_Cpp idds_server.cpp
)

set(SRC_PublicHeaders
    idds_server.h
)

set(INCLUDE_DIR ../include/idds)
prepend_include(${INCLUDE_DIR} SRC_PublicHeaders)


set(SRC_PrivateHeaders
)


add_library(${LIB_NAME} STATIC ${SRC_Cpp}
                               ${SRC_PublicHeaders}
                               ${SRC_PrivateHeaders}
)

add_library(${SDK_TARGET_NAMESPACE}::${BASE_NAME} ALIAS ${LIB_NAME})

if(BUILD_64Bit OR BUILD_ARM)
    set_target_properties(${LIB_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
else()
    set_target_properties(${LIB_NAME} PROPERTIES POSITION_INDEPENDENT_CODE OFF)
endif()

target_include_directories(${LIB_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
                                              $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include>

                                              $<INSTALL_INTERFACE:include>
)

target_link_libraries(${LIB_NAME} PUBLIC daq::opendaq
)

if (MSVC)
    target_compile_options(${LIB_NAME} PRIVATE /bigobj)
endif()

set_target_properties(${LIB_NAME} PROPERTIES PUBLIC_HEADER "${SRC_PublicHeaders}")

opendaq_set_output_lib_name(${LIB_NAME} ${PROJECT_VERSION_MAJOR})
