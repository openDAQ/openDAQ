set(CMAKE_FOLDER external)
list(APPEND CMAKE_MESSAGE_CONTEXT external)

setup_module_default_dependencies(${REPO_OPTION_PREFIX})

if (DAQMODULES_REF_FB_MODULE_ENABLE_RENDERER)
    add_subdirectory(sfml EXCLUDE_FROM_ALL)
endif()
add_subdirectory(kissfft EXCLUDE_FROM_ALL)
