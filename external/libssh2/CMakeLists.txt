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
