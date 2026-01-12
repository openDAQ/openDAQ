macro(setup_repo REPO_OPTION_PREFIX)
    if (NOT DEFINED PROJECT_SOURCE_DIR)
        message(FATAL_ERROR "Must be run inside a project()")
    endif()

    # Additional build options
    option(${REPO_OPTION_PREFIX}_DISABLE_DEBUG_POSTFIX "Disable debug ('-debug') postfix" OFF)
    option(${REPO_OPTION_PREFIX}_DEBUG_WARNINGS_AS_ERRORS "Treat debug warnings as errors" OFF)
    option(${REPO_OPTION_PREFIX}_ENABLE_TESTS "Enable unit-tests for ${REPO_OPTION_PREFIX}" ON)

    get_filename_component(ROOT_DIR ${CMAKE_SOURCE_DIR} REALPATH)

    if (NOT ${PROJECT_SOURCE_DIR} STREQUAL ${ROOT_DIR})
        set(BUILDING_AS_SUBMODULE ON PARENT_SCOPE)
        message(STATUS "Building as submodule")
    else()
        message(STATUS "Building standalone")
        set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER ".CMakePredefinedTargets")
        set_property(GLOBAL PROPERTY USE_FOLDERS ON)

        get_property(IS_MULTICONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

        message(STATUS "Platform: ${CMAKE_SYSTEM_PROCESSOR} | ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_VERSION}")
        message(STATUS "Generator: ${CMAKE_GENERATOR} | ${CMAKE_GENERATOR_PLATFORM}")

        if (IS_MULTICONFIG)
            message(STATUS "Configuration types:")

            block()
                list(APPEND CMAKE_MESSAGE_INDENT "\t")

                foreach(CONFIG_TYPE ${CMAKE_CONFIGURATION_TYPES})
                    message(STATUS ${CONFIG_TYPE})
                endforeach()
            endblock()
        else()
            message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
        endif()

        string(TIMESTAMP CONFIGURE_DATE)
        string(TIMESTAMP CURRENT_YEAR "%Y")
    endif()

    if (UNIX AND CMAKE_SYSTEM_PROCESSOR MATCHES "^arm.*$")  #e.g. armv7l
        set(BUILD_ARM On CACHE INTERNAL "Build for ARM architecture")
    endif()

    if (UNIX AND CMAKE_SYSTEM_PROCESSOR MATCHES "^aarch.*$")  #e.g. aarch64
        set(BUILD_ARM On CACHE INTERNAL "Build for ARM architecture")
    endif()

    set(BUILD_64Bit Off)

    if("${CMAKE_SIZEOF_VOID_P}" EQUAL 8)
        set(BUILD_64Bit On)
    endif()

    if (UNIX AND CMAKE_SYSTEM_PROCESSOR MATCHES "^aarch64$")  # arm architecture 64bit
        set(BUILD_64Bit On)
    endif()

    if(BUILD_64Bit OR BUILD_ARM)
        message(STATUS "Position independent code flag is set")
        set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    else()
        message(STATUS "Position independent code flag is not set")
    endif()

    set(CMAKE_CXX_STANDARD 17)
    if (WIN32)
        add_compile_definitions(NOMINMAX
                                _WIN32_WINNT=0x0601 # Windows 7 Compat
        )

        add_compile_definitions(UNICODE _UNICODE)
    endif()

    if(NOT CMAKE_DEBUG_POSTFIX AND NOT ${REPO_OPTION_PREFIX}_DISABLE_DEBUG_POSTFIX)
      set(CMAKE_DEBUG_POSTFIX -debug)
    endif()

    if (MSVC)
        # As above CMAKE_CXX_STANDARD but for VS
        add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/std:c++17>)

        foreach (flag IN ITEMS
            # Set source and execution character sets to UTF-8
            # https://learn.microsoft.com/en-us/cpp/build/reference/utf-8-set-source-and-executable-character-sets-to-utf-8
            /utf-8
            # Display level 1, level 2, and level 3 warnings, and all level 4 (informational) warnings that aren't off by default.
            # https://learn.microsoft.com/en-us/cpp/build/reference/compiler-option-warning-level
            /W4
            # data member 'member1' will be initialized after data member 'member2'
            # https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/c5038
            #/w15038
            # Supress warnings
            # https://learn.microsoft.com/en-us/cpp/build/reference/compiler-option-warning-level
            #
            # 'class1' : inherits 'class2::member' via dominance
            # https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4250
            #/wd4250
            # Your code uses a function, class member, variable, or typedef that's marked deprecated.
            # https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-3-c4996
            /wd4996
            # declaration of 'identifier' hides class member
            # https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-4-c4458
            /wd4458
            # nonstandard extension used : nameless struct/union
            # https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-4-c4201
            #/wd4201
            # unreachable code
            # https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-4-c4702
            #/wd4702
            # declaration of 'identifier' hides global declaration
            # https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-4-c4459
            #/wd4459
            # 'function' : unreferenced local function has been removed
            # https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-4-c4505
            #/wd4505
            # conditional expression is constant
            # https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-4-c4127
            #/wd4127
            # assignment within conditional expression
            # https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-4-c4706
            #/wd4706
            # loss of data / precision, unsigned <--> signed
            #
            # 'argument' : conversion from 'type1' to 'type2', possible loss of data
            # https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4244
            /wd4244
            # 'var' : conversion from 'size_t' to 'type', possible loss of data
            # https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-3-c4267
            #/wd4267
            # 'identifier' : unreferenced formal parameter
            # https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-4-c4100
            /wd4100
        )
            add_compile_options($<$<COMPILE_LANGUAGE:C,CXX>:${flag}>)
        endforeach()

        if (NOT OPENDAQ_MSVC_SINGLE_PROCESS_BUILD)
            # Build with multiple processes
            # https://learn.microsoft.com/en-us/cpp/build/reference/mp-build-with-multiple-processes
            add_compile_options($<$<COMPILE_LANGUAGE:C,CXX>:/MP>)
        endif()

        # Treat warnings as errors if not Debug or OPENDAQ_DEBUG_WARNINGS_AS_ERRORS is ON
        add_compile_options($<$<OR:$<NOT:$<CONFIG:Debug>>,$<BOOL:${${REPO_OPTION_PREFIX}_DEBUG_WARNINGS_AS_ERRORS}>>:/WX>)

        add_compile_definitions($<$<CONFIG:Debug>:_DEBUG>)

        if (MSVC_VERSION GREATER_EQUAL 1910)
            # /Zc:__cplusplus forces MSVC to use the correct value of __cplusplus macro (otherwise always C++98)
            add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/Zc:__cplusplus>)
            if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
                # /Zf (Faster PDB generation) is not supported by ClangCL
                set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Zf")
            endif()

            # Produce diagnostic messages with exact location
            add_compile_options($<$<COMPILE_LANGUAGE:C,CXX>:/diagnostics:caret>)
        endif()

        # set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /ignore:4221")
        # set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /ignore:4221")
        # set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} /ignore:4221")
    endif()
    
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

    if (${REPO_OPTION_PREFIX}_ENABLE_TESTS)
        set(OPENDAQ_ENABLE_TEST_UTILS ON CACHE BOOL "Enable testing utils library")
    endif()

    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    if ((CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANGXX) AND NOT MSVC)
        if (NOT WIN32)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
        endif()
    endif()

    find_package(Git REQUIRED)
endmacro()
