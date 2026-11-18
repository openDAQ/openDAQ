# libssh2 (BSD-3-Clause) with the OpenSSL crypto backend, built as a static library.
#
# libssh2 declares its build switches with option(), and its cmake_minimum_required predates
# policy CMP0077, so option() would ignore plain variables. Setting the policy default lets the
# variables below stay local to this directory instead of forcing global cache entries such as
# BUILD_SHARED_LIBS onto the rest of the build.
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

set(BUILD_SHARED_LIBS OFF)
set(BUILD_STATIC_LIBS ON)
set(BUILD_EXAMPLES OFF)
set(BUILD_TESTING OFF)
set(ENABLE_ZLIB_COMPRESSION OFF)
set(CRYPTO_BACKEND "OpenSSL" CACHE STRING "libssh2 crypto backend" FORCE)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

opendaq_dependency(
    NAME                libssh2
    REQUIRED_VERSION    1.11.1
    GIT_REPOSITORY      https://github.com/libssh2/libssh2.git
    GIT_REF             libssh2-1.11.1
    EXPECT_TARGET       libssh2::libssh2
)

# On Windows libssh2 links the system library "bcrypt" by its bare name, but openDAQ defines its own
# target named bcrypt (external/bcrypt, password hashing), so CMake resolves the name to that target
# instead. This links the wrong library and breaks libssh2's export(). Use the explicit file name, as
# openDAQ does elsewhere (bcrypt.dll), so it resolves to the Windows system library.
if (WIN32 AND TARGET libssh2_static)
    get_target_property(_libssh2_imported libssh2_static IMPORTED)
    if (NOT _libssh2_imported)
        foreach(_property LINK_LIBRARIES INTERFACE_LINK_LIBRARIES)
            get_target_property(_libs libssh2_static ${_property})
            if (_libs)
                list(TRANSFORM _libs REPLACE "^bcrypt$" "bcrypt.dll")
                list(TRANSFORM _libs REPLACE "^\\$<LINK_ONLY:bcrypt>$" "$<LINK_ONLY:bcrypt.dll>")
                set_target_properties(libssh2_static PROPERTIES ${_property} "${_libs}")
            endif()
        endforeach()
    endif()
endif()
