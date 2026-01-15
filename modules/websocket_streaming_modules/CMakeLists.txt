set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
cmake_minimum_required(VERSION 3.25)

if (DAQMODULES_LT_LEGACY_MODULES)
    opendaq_fetch_module(
        NAME                LtStreamingModulesLegacy
        GIT_REPOSITORY      https://github.com/openDAQ/LTStreamingModulesLegacy.git
        GIT_TAG             other/complete-missing-init-files
        GIT_SHALLOW         OFF
    )
else()
    opendaq_fetch_module(
        NAME                LtStreamingModulesModern
        GIT_REPOSITORY      https://github.com/openDAQ/LTStreamingModulesModern.git
        GIT_TAG             other/complete-missing-init-files
        GIT_SHALLOW         OFF
    )
endif()
