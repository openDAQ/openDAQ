set(CMAKE_FOLDER external)
list(APPEND CMAKE_MESSAGE_CONTEXT external)

if (PROJECT_IS_TOP_LEVEL OR NOT TARGET "${OPENDAQ_SDK_TARGET_NAMESPACE}::${OPENDAQ_SDK_TARGET_NAME}")
    add_subdirectory(opendaq)
endif()

opendaq_ignore_compiler_warnings()

if (${REPO_OPTION_PREFIX}_ENABLE_TESTS AND NOT TARGET gtest)
    opendaq_module_setup_gtest("1.12.1")
endif()

if (DAQMODULES_REF_FB_MODULE_ENABLE_RENDERER)
    add_subdirectory(sfml)
endif()
add_subdirectory(kissfft)
