if(OPENDAQ_LINK_3RD_PARTY_LIBS_STATICALY)
    set(BUILD_SHARED_LIBS OFF)
    set(Boost_USE_STATIC_LIBS ON)
    message(STATUS "Linking Boost statically")
else()
    set(BUILD_SHARED_LIBS ON)
    set(Boost_USE_STATIC_LIBS OFF)
    message(STATUS "Linking Boost dynamically")
endif()

set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME ${OPENDAQ_LINK_RUNTIME_STATICALLY})
set(Boost_NO_WARN_NEW_VERSIONS ON)

if(OPENDAQ_LINK_RUNTIME_STATICALLY)
    set(BOOST_RUNTIME_LINK static)
else()
    set(BOOST_RUNTIME_LINK shared)
endif()

if(BUILD_ARM)
    if(BUILD_64Bit)
        set(CONTEXT_ARCHITECTURE arm64)
    else()
        set(CONTEXT_ARCHITECTURE arm)
    endif()

    message(STATUS "CMAKE_CXX_COMPILER_ID: ${CMAKE_CXX_COMPILER_ID}")

    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        # Need C++ compiler to pre-process ASM files
        set(CMAKE_ASM_COMPILER ${CMAKE_CXX_COMPILER})
        set(CMAKE_ASM_FLAGS "-x assembler-with-cpp" CACHE STRING "")
    endif()

    set(BOOST_CONTEXT_ABI aapcs CACHE STRING "Boost.Context binary format (elf, mach-o, pe, xcoff)")
    set(BOOST_CONTEXT_ARCHITECTURE ${CONTEXT_ARCHITECTURE} CACHE STRING "Boost.Context architecture (arm, arm64, loongarch64, mips32, mips64, ppc32, ppc64, riscv64, s390x, i386, x86_64, combined)")
endif()

# This list should only ever include compiled Boost components/targets and the
# headers target. Header-only components/targets should be included in the list
# below to avoid find_package errors
set(REQUIRED_COMPILED_COMPONENTS
    "headers"
)

# This list should only ever include header-only Boost components/targets. This
# avoids find_package issues for "missing" components which only occur when
# Boost is build from source (FetchContent)
set(REQUIRED_HEADER_COMPONENTS
    "algorithm"
    "align"
    "asio"
    "beast"
    "dll"
    "locale"
    "program_options"
    "uuid"
)

# This list should only ever include Boost components/targets that are or will
# be deprecated. This avoids find_package issues for "missing" components across
# different versions of Boost
set(OPTIONAL_DEPRECATED_COMPONENTS
#    "system" # Deprecation started in Boost 1.85
)

if(DAQMODULES_PARQUET_RECORDER_MODULE)
    list(APPEND REQUIRED_HEADER_COMPONENTS
        "assert"
        "bind"
        "concept_check"
        "config"
        "core"
        "detail"
        "function"
        "io"
        "iterator"
        "mp11"
        "mpl"
        "numeric_conversion"
        "preprocessor"
        "range"
        "smart_ptr"
        "static_assert"
        "throw_exception"
        "tokenizer"
        "type_traits"
        "utility"
    )

    if(MSVC)
        list(APPEND REQUIRED_HEADER_COMPONENTS
            "multiprecision"
            "predef"
            "scope_exit"
            "typeof"
        )
    endif()
endif()

# Tell Boost which libraries to include when building from source (FetchContent)
# This is also used for boost-extra.cmake.in to generate a list of components
# that have been fetched when using FetchContent
set(BOOST_INCLUDE_LIBRARIES
    ${REQUIRED_COMPILED_COMPONENTS}
    ${REQUIRED_HEADER_COMPONENTS}
    ${OPTIONAL_DEPRECATED_COMPONENTS}
)

# Use the full version number, x.xx.x, for proper FetchContent URLs
set(BOOST_REQUESTED_VERSION 1.82.0)

opendaq_dependency(
    NAME                Boost
    REQUIRED_VERSION    1.71.0
    URL                 https://github.com/boostorg/boost/releases/download/boost-${BOOST_REQUESTED_VERSION}/boost-${BOOST_REQUESTED_VERSION}.tar.xz
    URL_HASH            SHA256=fd60da30be908eff945735ac7d4d9addc7f7725b1ff6fcdcaede5262d511d21e
    EXPECT_TARGET       Boost::headers
    OVERRIDE_FIND_PACKAGE
)

if(Boost_FETCHED)
    message("Boost FETCHED")

    # Set the requested Boost version for the following script to use it
    set(Boost_VERSION ${BOOST_REQUESTED_VERSION})

    # FetchContent allows for similar find_package functionality only if the user
    # manually defines the variables from the find module
    set(BOOST_EXTRA_FILENAME "boost-extra.cmake")
    configure_file(
        "${CMAKE_CURRENT_LIST_DIR}/${BOOST_EXTRA_FILENAME}.in"
        "${CMAKE_FIND_PACKAGE_REDIRECTS_DIR}/${BOOST_EXTRA_FILENAME}"
        @ONLY
    )

    # Now that boost-extra.cmake exists, use it with find_package to define
    # commonly used Boost variables from boost-config.cmake
    find_package("Boost" ${BOOST_REQUESTED_VERSION} QUIET GLOBAL
        COMPONENTS
            ${REQUIRED_COMPILED_COMPONENTS}
        OPTIONAL_COMPONENTS
            ${REQUIRED_HEADER_COMPONENTS}
            ${OPTIONAL_DEPRECATED_COMPONENTS}
        CONFIG # Boost 1.70+ should use the new generated boost-config.cmake find module
    )

    if(Boost_FOUND)
        message(STATUS "Found Boost: ${Boost_VERSION} at ${Boost_CONFIG}")
    endif()
endif()
