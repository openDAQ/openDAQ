set(CMAKE_FOLDER external)
list(APPEND CMAKE_MESSAGE_CONTEXT external)

opendaq_ignore_compiler_warnings()

if (${REPO_OPTION_PREFIX}_ENABLE_TESTS AND (NOT TARGET gtest OR NOT TARGET gmock))
    set(BUILD_GMOCK ON)
    opendaq_module_setup_gtest("1.12.1")
endif()

if (DAQMODULES_REF_FB_MODULE_ENABLE_RENDERER)
    add_subdirectory(sfml EXCLUDE_FROM_ALL)
endif()
add_subdirectory(kissfft EXCLUDE_FROM_ALL)
