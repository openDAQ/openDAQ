opendaq_set_cmake_folder_context(TARGET_FOLDER_NAME)

if (MSVC)
    add_compile_options(/wd4100)

    # loss of data / precision, unsigned <--> signed
    #
    # 'argument' : conversion from 'type1' to 'type2', possible loss of data
    # https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4244
    add_compile_options(/wd4244)

    # 'var' : conversion from 'size_t' to 'type', possible loss of data
    # https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-3-c4267
    add_compile_options(/wd4267)
endif()

add_subdirectory(libraries)
