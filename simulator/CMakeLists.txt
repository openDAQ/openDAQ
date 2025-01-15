set_cmake_folder_context(TARGET_FOLDER_NAME)

option(DAQSIMULATOR_ENABLE_SIMULATOR_APP "Enable device simulator application" OFF)

if (DAQSIMULATOR_ENABLE_SIMULATOR_APP)
    add_subdirectory(simulator_app)
endif()
