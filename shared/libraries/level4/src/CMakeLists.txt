set(BASE_NAME level4)
set(MODULE_NAME ${SDK_TARGET_NAME}_${BASE_NAME})

set(LEVEL4_HEADERS_DIR ../include/${BASE_NAME})

set(RTGEN_LIBRARY_NAME ${BASE_NAME})
set(GENERATED_HEADERS_DIR ${CMAKE_CURRENT_BINARY_DIR}/${LEVEL4_HEADERS_DIR})

set(RTGEN_HEADERS_DIR ${LEVEL4_HEADERS_DIR})
set(RTGEN_OUTPUT_DIR ${GENERATED_HEADERS_DIR})
set(RTGEN_OUTPUT_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(RTGEN_RELATIVE_PARENT_DIR shared/libraries/${BASE_NAME})

rtgen(SRC_Level4View property_object_view.h)

set(SRC_PublicHeaders property_object_view_impl.h
)

# set(SRC_PrivateHeaders 
# )                       


set(SRC_Cpp property_object_view_impl.cpp
)

prepend_include(${BASE_NAME} SRC_PublicHeaders)
prepend_include(${BASE_NAME} SRC_PrivateHeaders)

list(APPEND SRC_PublicHeaders ${SRC_Level4View_PublicHeaders}
)

list(APPEND SRC_PrivateHeaders ${SRC_Level4View_PrivateHeaders}
)

add_library(${MODULE_NAME} STATIC ${SRC_HEADERS} ${SRC_Cpp} ${SRC_PublicHeaders} ${SRC_PrivateHeaders})
add_library(${SDK_TARGET_NAMESPACE}::${BASE_NAME} ALIAS ${MODULE_NAME})

if (MSVC)
    target_compile_options(${MODULE_NAME} PRIVATE /bigobj)
elseif (MINGW AND CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options(${MODULE_NAME} PRIVATE -Wa,-mbig-obj)
endif()

target_include_directories(${MODULE_NAME} PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_BINARY_DIR}/../include)

target_link_libraries(${MODULE_NAME} 
    PUBLIC 
        daq::opendaq
)
