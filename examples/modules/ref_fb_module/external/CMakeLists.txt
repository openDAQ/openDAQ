set(CMAKE_FOLDER external)
list(APPEND CMAKE_MESSAGE_CONTEXT external)

if (NOT ${REPO_OPTION_PREFIX}_BUILDING_AS_SUBMODULE)
    resolve_opendaq_dependency(main)
endif()

suppress_ext_lib_warnings()
if (${REPO_OPTION_PREFIX}_ENABLE_TESTS)
    add_subdirectory(gtest)
endif()

if (DAQMODULES_REF_FB_MODULE_ENABLE_RENDERER)
    add_subdirectory(sfml EXCLUDE_FROM_ALL)
endif()
add_subdirectory(kissfft EXCLUDE_FROM_ALL)
