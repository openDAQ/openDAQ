set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
cmake_minimum_required(VERSION 3.25)

set(REPO_NAME RefFbModule)
set(REPO_OPTION_PREFIX DAQMODULES_REF_FB_MODULE)
add_subdirectory(cmake)

setup_module_project(${REPO_OPTION_PREFIX} ${REPO_NAME})

if (DAQMODULES_REF_FB_MODULE_ENABLE_RENDERER AND APPLE)
    enable_language(OBJC OBJCXX)
endif()

setup_module_subfolders(${REPO_OPTION_PREFIX} ${REPO_NAME})
