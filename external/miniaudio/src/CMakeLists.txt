set(EXTERNAL_LIB miniaudio)

add_library(${EXTERNAL_LIB} INTERFACE)
add_library(${EXTERNAL_LIB}::${EXTERNAL_LIB} ALIAS ${EXTERNAL_LIB})

target_include_directories(${EXTERNAL_LIB} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/../include)

if (CMAKE_DL_LIBS)
    target_link_libraries(${EXTERNAL_LIB} INTERFACE ${CMAKE_DL_LIBS})
endif()