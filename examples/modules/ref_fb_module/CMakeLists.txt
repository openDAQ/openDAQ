set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
cmake_minimum_required(VERSION 3.25)

# set project variables
set(REPO_NAME RefFbModule)
set(REPO_OPTION_PREFIX REF_FB_MODULE)

add_subdirectory(cmake)
setup_repo_version("${REPO_OPTION_PREFIX}" ${REPO_NAME} "module_version")

if (NOT DEFINED ${REPO_OPTION_PREFIX}_VERSION AND DEFINED OPENDAQ_PACKAGE_VERSION)
    set(${REPO_OPTION_PREFIX}_VERSION ${OPENDAQ_PACKAGE_VERSION})
endif()

project(${REPO_NAME} VERSION ${${REPO_OPTION_PREFIX}_VERSION} LANGUAGES CXX)

setup_build_mode(${REPO_OPTION_PREFIX} ${REPO_NAME})
if (NOT ${REPO_OPTION_PREFIX}_BUILDING_AS_SUBMODULE)
    setup_repo(${REPO_OPTION_PREFIX})
endif()

option(DAQMODULES_REF_FB_MODULE_ENABLE_RENDERER "Enable renderer function block" ON)
if (DAQMODULES_REF_FB_MODULE_ENABLE_RENDERER AND APPLE)
    enable_language(OBJC OBJCXX)
endif()

add_subdirectory(external)
add_subdirectory(modules)

if (${REPO_OPTION_PREFIX}_BUILDING_AS_SUBMODULE AND OPENDAQ_ENABLE_TESTS OR ${REPO_OPTION_PREFIX}_ENABLE_TESTS)
#    add_subdirectory(tests)
endif()

if (${REPO_OPTION_PREFIX}_ENABLE_EXAMPLE_APP)
#    add_subdirectory(examples)
endif()

#add_subdirectory(docs)	# FIXME no docs infrastructure to be handled by cmake
