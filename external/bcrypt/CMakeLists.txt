set_cmake_folder_context(TARGET_FOLDER_NAME)

set(EXTERNAL_LIB bcrypt)
project(${EXTERNAL_LIB} VERSION 1.0.0)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(HEADERS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/bcrypt)

set(SRC_HEADERS ${HEADERS_DIR}/bcrypt.h
                ${HEADERS_DIR}/BCrypt.hpp
                ${HEADERS_DIR}/crypt.h
                ${HEADERS_DIR}/crypt_blowfish.h
                ${HEADERS_DIR}/crypt_gensalt.h
                ${HEADERS_DIR}/ow-crypt.h
                ${HEADERS_DIR}/winbcrypt.h
)

set(SRC_CPPS ${SRC_DIR}/bcrypt.c
             ${SRC_DIR}/crypt_blowfish.c
             ${SRC_DIR}/crypt_gensalt.c
             ${SRC_DIR}/wrapper.c
             #${SRC_DIR}/x86.S
)

add_library(${EXTERNAL_LIB} ${SRC_CPPS} ${SRC_HEADERS})
add_library(${EXTERNAL_LIB}::${EXTERNAL_LIB} ALIAS ${EXTERNAL_LIB})

target_include_directories(${EXTERNAL_LIB} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if (NOT WIN32)
    target_include_directories(${EXTERNAL_LIB} PRIVATE ${HEADERS_DIR})
endif()

if (OPENDAQ_ENABLE_TESTS)
    add_subdirectory(tests)
endif()

include(GNUInstallDirs)

install(TARGETS ${EXTERNAL_LIB} EXPORT ${EXTERNAL_LIB}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
  
install(
    DIRECTORY ${CMAKE_INSTALL_INCLUDEDIR}/${EXTERNAL_LIB}    
    DESTINATION include
)
