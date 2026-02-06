set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
cmake_minimum_required(VERSION 3.25)

if (DAQMODULES_LT_LEGACY_MODULES)
    add_subdirectory(legacy)
else()
    add_subdirectory(modern)
endif()
