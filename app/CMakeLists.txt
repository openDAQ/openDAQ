set_cmake_folder_context(TARGET_FOLDER_NAME)

option(APP_ENABLE_EXAMPLE_APPS "Enable example ${SDK_NAME} applications" OFF)
option(APP_ENABLE_AUDIO_APP "Enable ${SDK_NAME} audio application" OFF)

if (APP_ENABLE_EXAMPLE_APPS AND NOT (DAQMODULES_REF_DEVICE_MODULE AND DAQMODULES_REF_FB_MODULE))
    message(WARNING "DAQMODULES_REF_DEVICE_MODULE and DAQMODULES_REF_FB_MODULE must be ON to enable Example applications")
endif()

if (APP_ENABLE_AUDIO_APP AND NOT DAQMODULES_AUDIO_DEVICE_MODULE)
    message(WARNING "DAQMODULES_AUDIO_DEVICE_MODULE must be ON to enable Audio application")
endif()

if(APP_ENABLE_EXAMPLE_APPS AND DAQMODULES_REF_DEVICE_MODULE AND DAQMODULES_REF_FB_MODULE)
    add_subdirectory(example_apps)
endif()

if(APP_ENABLE_AUDIO_APP AND DAQMODULES_AUDIO_DEVICE_MODULE)
    add_subdirectory(audio_application)
endif()
