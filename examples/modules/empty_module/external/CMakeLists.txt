set(CMAKE_FOLDER external)
list(APPEND CMAKE_MESSAGE_CONTEXT external)

if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
#    message(FATAL_ERROR "In-source build is not supported!")
endif()

if (IS_TOP_LEVEL_PROJECT)

if(NOT OPENDAQ_SDK_VERSION)
#    message(FATAL_ERROR "OPENDAQ_SDK_VERSION is not set")
endif()

include(FetchContent)
find_package(openDAQ ${OPENDAQ_SDK_VERSION} GLOBAL)

if (NOT openDAQ_FOUND)
    set(OPENDAQ_ENABLE_TESTS OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        openDAQ
        GIT_REPOSITORY https://github.com/openDAQ/openDAQ.git
        GIT_TAG main
        GIT_PROGRESS   ON
        SOURCE_DIR ${FETCHCONTENT_EXTERNALS_DIR}/src/openDAQ
    )

    FetchContent_MakeAvailable(openDAQ)

    message(STATUS "openDAQ fetched source directory is detected as: ${openDAQ_SOURCE_DIR}")
    # cmake incorrectly detects openDAQ_SOURCE_DIR at repo root/core/opendaq likely caused by similar targets: openDAQ (at root) vs opendaq (at root/core/opendaq), so modify path
    include(${openDAQ_SOURCE_DIR}/../../cmake/opendaq_dependency.cmake)
else()
    message(STATUS "Found openDAQ in ${openDAQ_DIR}")
    message(STATUS "Include opendaq_dependency macro from found ${openDAQ_DIR}/../../../share/cmake/opendaq_dependency.cmake")
    include("${openDAQ_DIR}/../../../share/cmake/opendaq_dependency.cmake")
endif()

if (MSVC AND NOT CMAKE_COMPILER_IS_CLANGXX)
    # As above CMAKE_CXX_STANDARD but for VS
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/std:c++17>)

    # suppress warnings
    foreach (flag IN ITEMS
        # loss of data / precision, unsigned <--> signed
        #
        # 'argument' : conversion from 'type1' to 'type2', possible loss of data
        # https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4244
        /wd4244
        # 'var' : conversion from 'size_t' to 'type', possible loss of data
        # https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-3-c4267
        /wd4267
    )
        add_compile_options($<$<COMPILE_LANGUAGE:C,CXX>:${flag}>)
    endforeach()
endif()

if (MSVC AND NOT CMAKE_COMPILER_IS_CLANGXX)
    add_compile_options($<$<COMPILE_LANGUAGE:C,CXX>:/WX->)
else()
    add_compile_options($<$<COMPILE_LANGUAGE:C,CXX>:-Wno-error>)
endif()

if (${REPO_OPTION_PREFIX}_ENABLE_TESTS)
    add_subdirectory(gtest)
endif()

endif()

#add_subdirectory(boost)
#add_subdirectory(spdlog EXCLUDE_FROM_ALL)
#add_subdirectory(nlohmann_json EXCLUDE_FROM_ALL)
#add_subdirectory(streaming_protocol EXCLUDE_FROM_ALL)

