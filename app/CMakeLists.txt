set_cmake_folder_context(TARGET_FOLDER_NAME)

option(APP_ENABLE_TEST_APP "Enable ${SDK_NAME} test application" OFF)
option(APP_ENABLE_EXAMPLE_APPS "Enable example ${SDK_NAME} applications" OFF)
option(APP_ENABLE_AUDIO_APP "Enable ${SDK_NAME} audio application" OFF)

if(APP_ENABLE_TEST_APP)
    add_subdirectory(test_app)
endif()

if(APP_ENABLE_EXAMPLE_APPS)
    add_subdirectory(example_apps)
endif()

if(APP_ENABLE_AUDIO_APP)
    add_subdirectory(audio_application)
endif()
