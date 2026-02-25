set(CMAKE_FOLDER external)
list(APPEND CMAKE_MESSAGE_CONTEXT external)

opendaq_ignore_compiler_warnings()
if (${REPO_OPTION_PREFIX}_ENABLE_TESTS AND NOT TARGET gtest)
    opendaq_module_setup_gtest("1.12.1")
endif()
