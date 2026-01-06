if (OPENDAQ_LINK_3RD_PARTY_LIBS_STATICALY)
    set(BUILD_SHARED_LIBS OFF)
    set(Boost_USE_STATIC_LIBS ON CACHE BOOL "")
    message(STATUS "Linking Boost statically")
else()
    set(BUILD_SHARED_LIBS ON)
    set(Boost_USE_STATIC_LIBS OFF CACHE BOOL "")
    message(STATUS "Linking Boost dynamically")
endif()

set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME ${OPENDAQ_LINK_RUNTIME_STATICALLY})
set(Boost_NO_WARN_NEW_VERSIONS ON)

if (OPENDAQ_LINK_RUNTIME_STATICALLY)
    set(BOOST_RUNTIME_LINK static)
else()
    set(BOOST_RUNTIME_LINK shared)
endif()

if (BUILD_ARM)
    if (BUILD_64Bit)
        set(CONTEXT_ARCHITECTURE arm64)
    else()
        set(CONTEXT_ARCHITECTURE arm)
    endif()

    message(STATUS "CMAKE_COMPILER_IS_GNUCXX: ${CMAKE_COMPILER_IS_GNUCXX}")

    if (CMAKE_COMPILER_IS_GNUCXX)
        # Need C++ compiler to pre-process ASM files
        set(CMAKE_ASM_COMPILER ${CMAKE_CXX_COMPILER})
        set(CMAKE_ASM_FLAGS "-x assembler-with-cpp" CACHE STRING "")
    endif()

    set(BOOST_CONTEXT_ABI aapcs CACHE STRING "Boost.Context binary format (elf, mach-o, pe, xcoff)")
    set(BOOST_CONTEXT_ARCHITECTURE ${CONTEXT_ARCHITECTURE} CACHE STRING "Boost.Context architecture (arm, arm64, loongarch64, mips32, mips64, ppc32, ppc64, riscv64, s390x, i386, x86_64, combined)")
endif()

set(NEEDED_LIBRARIES algorithm
                     asio
                     beast
                     dll
                     program_options
                     uuid
                     locale
                     align

)

if(DAQMODULES_PARQUET_RECORDER_MODULE AND MSVC)
    list(APPEND NEEDED_LIBRARIES
                     multiprecision
                     predef
                     scope_exit
                     typeof
    )
endif()

set(BOOST_INCLUDE_LIBRARIES "${NEEDED_LIBRARIES}"
    CACHE STRING
    "List of libraries to build (default: all but excluded and incompatible)"
    FORCE
)

opendaq_dependency(
    NAME                Boost
    REQUIRED_VERSION    1.71.0
    URL                 https://github.com/boostorg/boost/releases/download/boost-1.82.0/boost-1.82.0.tar.xz
    URL_HASH            SHA256=fd60da30be908eff945735ac7d4d9addc7f7725b1ff6fcdcaede5262d511d21e
    EXPECT_TARGET       Boost::headers
    OVERRIDE_FIND_PACKAGE
)

if (Boost_FETCHED)
    message("Boost FETCHED")

    # dont treat warnings as errors
    set(BOOST_TARGETS
        boost_container
        boost_thread
    )
    foreach(BOOST_TARGET ${BOOST_TARGETS})
        if (TARGET ${BOOST_TARGET})
            if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
                target_compile_options(${BOOST_TARGET} PRIVATE /WX-)
            else()
                target_compile_options(${BOOST_TARGET} PRIVATE -Wno-error)
            endif()
        endif()
    endforeach()

    if(DAQMODULES_PARQUET_RECORDER_MODULE)
        set(boost_libs 
            config
            algorithm 
            assert 
            bind 
            concept_check 
            core 
            detail 
            function
            iterator 
            locale 
            mpl 
            numeric_conversion 
            preprocessor 
            range 
            smart_ptr
            static_assert 
            throw_exception 
            tokenizer 
            type_traits 
            utility 
            mp11
            uuid
        )

        if(MSVC)
            list(APPEND boost_libs
                    multiprecision
                    predef
                    scope_exit
                    typeof
            
            )
        endif()

        add_library(daq::boost_headers INTERFACE IMPORTED GLOBAL)

        foreach(boost_lib ${boost_libs})
            if (TARGET Boost::${boost_lib})
                get_target_property(include_dirs Boost::${boost_lib} INTERFACE_INCLUDE_DIRECTORIES)
                target_include_directories(daq::boost_headers INTERFACE "${include_dirs}")
            endif()
        endforeach()
    endif()
endif()

if (Boost_FOUND)

    foreach(BOOST_TARGET ${BOOST_INCLUDE_LIBRARIES})
        if (NOT TARGET Boost::${BOOST_TARGET} AND NOT TARGET boost_${BOOST_TARGET})
            add_library(Boost::${BOOST_TARGET} ALIAS Boost::headers)
        endif()
    endforeach()
endif()
