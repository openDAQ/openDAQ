if (UNIX AND CMAKE_COMPILER_IS_CLANGXX)
    # Round about way of disabling clang sanitizers as open62541 doesn't have a way to do that properly
    set(UA_BUILD_UNIT_TESTS OFF CACHE BOOL "Build the unit tests")
    set(UA_ENABLE_UNIT_TESTS_MEMCHECK ON CACHE BOOL "Use Valgrind (Linux) or DrMemory (Windows) to detect memory leaks when running the unit tests")
endif()

set(UA_NAMESPACE_ZERO FULL CACHE STRING "" FORCE)
set(UA_ENABLE_TYPEDESCRIPTION ON CACHE STRING "" FORCE)
set(UA_ENABLE_STATUSCODE_DESCRIPTIONS ON CACHE STRING "" FORCE)
set(UA_MULTITHREADING 100 CACHE STRING "" FORCE)

set_directory_properties(PROPERTIES EXCLUDE_FROM_ALL YES)

opendaq_dependency(
    NAME                open62541
    REQUIRED_VERSION    1.3.6
    GIT_REPOSITORY      https://github.com/openDAQ/open62541.git
    GIT_REF             v1.3.6-opendaq-2
    GIT_SUBMODULES      ""
    EXPECT_TARGET       open62541
)

if (open62541_FETCHED)

    target_compile_definitions(open62541 PRIVATE UA_ENABLE_AMALGAMATION=OFF)

    if (WIN32)
        target_compile_definitions(open62541 PUBLIC UA_ARCHITECTURE="win32")
    endif()

    # dont treat warnings as errors

    set(OPEN62541_DISABLED_WARNINGS "-Wno-error")
    if(MSVC)
        set(OPEN62541_DISABLED_WARNINGS /WX- /wd4100 /wd4214 /wd4200 /wd4204 /wd4057 /wd4701 /wd4133)
    endif()

    target_compile_options(open62541 PRIVATE ${OPEN62541_DISABLED_WARNINGS})
    target_compile_options(open62541-plugins PRIVATE ${OPEN62541_DISABLED_WARNINGS})
    target_compile_options(open62541-object PRIVATE ${OPEN62541_DISABLED_WARNINGS})

    set(OPEN62541_DISABLED_WARNINGS ${OPEN62541_DISABLED_WARNINGS} PARENT_SCOPE)

    # fix library folders

    set_target_properties(doc doc_latex doc_pdf PROPERTIES FOLDER "${CMAKE_FOLDER}/doc")

    set(LIB_TARGETS
        open62541-plugins
        open62541
        open62541-object
    )

    foreach(LIB_TARGET ${LIB_TARGETS})
        set_target_properties(${LIB_TARGET} PROPERTIES FOLDER "${CMAKE_FOLDER}/lib")

        if (UNIX)
            set_target_properties(${LIB_TARGET} PROPERTIES INTERPROCEDURAL_OPTIMIZATION OFF)
        endif()
    endforeach()

    if (UA_ENABLE_AMALGAMATION)
        set_target_properties(open62541-amalgamation-header PROPERTIES FOLDER "${CMAKE_FOLDER}/lib")
        set_target_properties(open62541-amalgamation-source PROPERTIES FOLDER "${CMAKE_FOLDER}/lib")
    endif()

    set(GENERATOR_TARGETS
        open62541-generator-namespace
        open62541-generator-statuscode
        open62541-generator-transport
        open62541-generator-types
        open62541-code-generation
    )

    foreach(TARGET_NAME ${GENERATOR_TARGETS})
        set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "${CMAKE_FOLDER}/generators")
    endforeach()

    set(open62541_TOOLS_DIR "${open62541_SOURCE_DIR}/tools" PARENT_SCOPE)
    set(open62541_NODESET_DIR "${open62541_SOURCE_DIR}/deps/ua-nodeset" PARENT_SCOPE)
    include("${open62541_BINARY_DIR}/open62541Macros.cmake")

endif()
