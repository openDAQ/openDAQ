if (WIN32 OR MINGW)
    remove_definitions(-DWIN32_LEAN_AND_MEAN)
endif()

set(SFML_BUILD_WINDOW ON)
set(SFML_BUILD_GRAPHICS ON)
set(SFML_BUILD_NETWORK OFF)
set(SFML_BUILD_DOC OFF)
set(SFML_BUILD_EXAMPLES OFF)
set(SFML_BUILD_AUDIO OFF)
set(WARNINGS_AS_ERRORS OFF CACHE BOOL "SFML treat warnings as errors")

opendaq_dependency(
    NAME                sfml
    REQUIRED_VERSION    2.6.x
    GIT_REPOSITORY      https://github.com/SFML/SFML.git
    GIT_REF             2.6.x
    EXPECT_TARGET       sfml-graphics
)

if (WIN32)
    set(SFML_WIN sfml-main)
endif()

set(SFML_TAGETS sfml-graphics
                sfml-window
                sfml-system
                ${SFML_WIN}
)

foreach(TARGET_NAME ${SFML_TAGETS})
    set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "external/sfml")

    if (MSVC)
        target_compile_options(${TARGET_NAME} PRIVATE /wd4456 /wd4701 /wd4703 /wd4100 /wd4242 /WX-)
    else()
        # old versions of freetype2 contain int instead of unsigned int
        target_compile_options(${TARGET_NAME} PRIVATE -Wno-error=sign-conversion)
    endif()
endforeach()
