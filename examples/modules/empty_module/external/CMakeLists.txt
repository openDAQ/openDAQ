set(CMAKE_FOLDER external)
list(APPEND CMAKE_MESSAGE_CONTEXT external)

if (NOT ${REPO_OPTION_PREFIX}_BUILDING_AS_SUBMODULE)
    resolve_opendaq_dependency(main)
endif()

suppress_ext_lib_warnings()
if (${REPO_OPTION_PREFIX}_ENABLE_TESTS)
    add_subdirectory(gtest)
endif()
#add_subdirectory(ext_dependency)
