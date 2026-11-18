# libssh (LGPL-2.1) with the OpenSSL crypto backend, built as a shared library: linking an LGPL library
# statically would bring re-linking obligations onto the rest of the build.
#
# Like libssh2 (see external/libssh2), libssh declares its build switches with option(); the policy default
# lets the variables below stay local to this directory instead of becoming global cache entries.
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

set(BUILD_SHARED_LIBS ON)
set(BUILD_STATIC_LIB OFF)
set(WITH_SERVER OFF)
set(WITH_GSSAPI OFF)
set(WITH_ZLIB OFF)
set(WITH_PCAP OFF)
set(WITH_NACL OFF)
set(WITH_EXAMPLES OFF)
set(WITH_DEBUG_CALLTRACE OFF)
set(UNIT_TESTING OFF)
set(CLIENT_TESTING OFF)
set(SERVER_TESTING OFF)
set(WITH_BENCHMARKS OFF)

# libssh includes CPack, which would otherwise overwrite openDAQ's CPack configuration in the top build directory.
set(CPACK_OUTPUT_CONFIG_FILE ${CMAKE_CURRENT_BINARY_DIR}/libssh-CPackConfig.cmake)
set(CPACK_SOURCE_OUTPUT_CONFIG_FILE ${CMAKE_CURRENT_BINARY_DIR}/libssh-CPackSourceConfig.cmake)

# 0001: libssh includes netioapi.h directly in src/misc.c, which the Windows SDK only supports through iphlpapi.h;
# with MSVC that fails to compile. The patch includes iphlpapi.h instead.
#
# openDAQ compiles every target, libssh included, with _WIN32_WINNT=0x0601 (Windows 7). libssh's configure checks
# (check_symbol_exists & co.) do not see directory compile definitions, so they would run against the SDK's default
# Windows version and report htonll/ntohll as available, while winsock2.h only provides them from Windows 8 on and
# the actual compile fails to link them. Forward the Windows version definitions to the checks so that their
# results match the build; libssh then falls back to its own htonll/ntohll.
if (WIN32)
    get_directory_property(_opendaq_compile_definitions COMPILE_DEFINITIONS)
    foreach(_definition IN LISTS _opendaq_compile_definitions)
        if (_definition MATCHES "^(_WIN32_WINNT|WINVER|NTDDI_VERSION)=")
            list(APPEND CMAKE_REQUIRED_DEFINITIONS "-D${_definition}")
        endif()
    endforeach()
endif()

# The installed package config exports the plain "ssh" target (the ssh::ssh alias exists only in a source build),
# so "ssh" is what both a found and a fetched libssh provide.
opendaq_dependency(
    NAME                libssh
    REQUIRED_VERSION    0.12.2
    GIT_REPOSITORY      https://git.libssh.org/projects/libssh.git
    GIT_REF             libssh-0.12.2
    EXPECT_TARGET       ssh
    PATCH_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/patches/0001-misc-include-iphlpapi-instead-of-netioapi.patch
)

# libssh calls pthread_create (ProxyJump support) but links Threads only on MinGW. Before glibc 2.34 that needs
# -pthread, and openDAQ links shared libraries with --no-undefined, so the link fails without it.
if (TARGET ssh AND NOT WIN32)
    get_target_property(_libssh_imported ssh IMPORTED)
    if (NOT _libssh_imported)
        find_package(Threads REQUIRED)
        target_link_libraries(ssh PRIVATE Threads::Threads)
    endif()
endif()
