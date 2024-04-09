set_cmake_folder_context(TARGET_FOLDER_NAME)

set(EXTERNAL_LIB kissfft)
project(${EXTERNAL_LIB} VERSION 2.0.1 LANGUAGES C CXX)

set(SRC_Include _kiss_fft_guts.h
                kiss_fft.h
                kiss_fft_log.h
)

set(SRC_Srcs kiss_fft.c
)

add_library(${EXTERNAL_LIB} ${SRC_Include}
                            ${SRC_Srcs}
)

add_library(${EXTERNAL_LIB}::${EXTERNAL_LIB} ALIAS ${EXTERNAL_LIB})

target_include_directories(${EXTERNAL_LIB} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

if (CMAKE_DL_LIBS)
    target_link_libraries(${EXTERNAL_LIB} INTERFACE ${CMAKE_DL_LIBS})
endif()