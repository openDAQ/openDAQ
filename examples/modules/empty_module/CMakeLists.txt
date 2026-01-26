set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
cmake_minimum_required(VERSION 3.25)

# set project variables
set(REPO_NAME EmptyModule)
set(REPO_OPTION_PREFIX EMPTY_MODULE)

add_subdirectory(cmake)

opendaq_setup_module_project(${REPO_OPTION_PREFIX} ${REPO_NAME})
opendaq_setup_module_subfolders(${REPO_OPTION_PREFIX} ${REPO_NAME})
