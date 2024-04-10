set_cmake_folder_context(TARGET_FOLDER_NAME)

set(RTGEN_NAMESPACE "${SDK_TARGET_NAMESPACE}")
set(OPENDAQ_CORE_VERSION 3.0.0)

add_subdirectory(corecontainers)
add_subdirectory(coretypes)
add_subdirectory(coreobjects)
add_subdirectory(opendaq)
