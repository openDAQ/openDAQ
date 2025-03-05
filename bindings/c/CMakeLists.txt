set(LIB_NAME copendaq)
set_cmake_folder_context(TARGET_FOLDER_NAME copendaq)

set(SRC_Headers
    ${CMAKE_CURRENT_LIST_DIR}/include/copendaq.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccommon.h
    # ${CMAKE_CURRENT_LIST_DIR}/include/base_object.h

    # ${CMAKE_CURRENT_LIST_DIR}/include/integer.h
    # ${CMAKE_CURRENT_LIST_DIR}/include/list.h
    # ${CMAKE_CURRENT_LIST_DIR}/include/number.h
    # ${CMAKE_CURRENT_LIST_DIR}/include/str.h
    ${CMAKE_CURRENT_LIST_DIR}/include/base_object.h
    ${CMAKE_CURRENT_LIST_DIR}/include/boolean.h
    ${CMAKE_CURRENT_LIST_DIR}/include/complex_number.h
    ${CMAKE_CURRENT_LIST_DIR}/include/dictobject.h
    ${CMAKE_CURRENT_LIST_DIR}/include/float1.h
    ${CMAKE_CURRENT_LIST_DIR}/include/integer1.h
    ${CMAKE_CURRENT_LIST_DIR}/include/iterable.h
    ${CMAKE_CURRENT_LIST_DIR}/include/iterator.h
    ${CMAKE_CURRENT_LIST_DIR}/include/listobject.h
    ${CMAKE_CURRENT_LIST_DIR}/include/number1.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ratio.h
    ${CMAKE_CURRENT_LIST_DIR}/include/stringobject.h
)

set(SRC_Cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/base_object.cpp

    ${CMAKE_CURRENT_LIST_DIR}/src/boolean.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/complex_number.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/dictobject.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/float1.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/integer1.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/iterable.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/iterator.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/listobject.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/number1.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ratio.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/stringobject.cpp

    # ${CMAKE_CURRENT_LIST_DIR}/src/integer.cpp
    # ${CMAKE_CURRENT_LIST_DIR}/src/list.cpp
    # ${CMAKE_CURRENT_LIST_DIR}/src/number.cpp
    # ${CMAKE_CURRENT_LIST_DIR}/src/str.cpp
)

add_library(${LIB_NAME} SHARED ${SRC_Headers} ${SRC_Cpp})
add_library(${SDK_TARGET_NAMESPACE}::${LIB_NAME} ALIAS ${LIB_NAME})

target_link_libraries(${LIB_NAME}
    PUBLIC
        daq::opendaq
)

# if(NOT DEFINED CMAKE_CXX_VISIBILITY_PRESET)
#     set_target_properties(${LIB_NAME} PROPERTIES CXX_VISIBILITY_PRESET "hidden")
# endif()

target_include_directories(${LIB_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
)

target_compile_definitions(${LIB_NAME} PRIVATE BUILDING_SHARED_LIBRARY)

set_target_properties(${LIB_NAME} PROPERTIES PUBLIC_HEADER "${SRC_Headers}")

install(TARGETS ${LIB_NAME}
        LIBRARY
            DESTINATION ${CMAKE_INSTALL_LIBDIR}
            COMPONENT          ${SDK_NAME}_${BASE_NAME}_Runtime
            NAMELINK_COMPONENT ${SDK_NAME}_${BASE_NAME}_Development
        PUBLIC_HEADER
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${BASE_NAME}/copendaq
            COMPONENT ${SDK_NAME}_${BASE_NAME}_Development
)

add_subdirectory(test_app)