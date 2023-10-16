set(BUILD_SHARED_LIBS OFF)
set(XXHASH_BUNDLED_MODE OFF)
set(XXHASH_BUILD_XXHSUM OFF CACHE BOOL "" FORCE)

set(PATCH_FILES)
if (UNIX AND NOT APPLE)
    set(PATCH_FILES ${CMAKE_CURRENT_SOURCE_DIR}/patches/001-fix-static-assert.patch)
endif()

opendaq_dependency(
    NAME                xxHash
    PKGCONFIG_NAME      libxxhash
    REQUIRED_VERSION    0.8.1
    GIT_REPOSITORY      https://github.com/Cyan4973/xxHash.git
    GIT_REF             v0.8.1
    EXPECT_TARGET       xxHash::xxhash
    PATCH_FILES         "${PATCH_FILES}"
)

if (xxHash_FETCHED)

    add_subdirectory(${xxhash_SOURCE_DIR}/cmake_unofficial ${xxhash_BINARY_DIR})

    install(TARGETS xxhash
        EXPORT ${SDK_NAME}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

endif()
