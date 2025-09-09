cmake_minimum_required(VERSION 3.10)
set_cmake_folder_context(TARGET_FOLDER_NAME)
project(WebsocketStreamingClientModule VERSION ${OPENDAQ_PACKAGE_VERSION} LANGUAGES C CXX)

if (MSVC)
    # loss of data / precision, unsigned <--> signed
    #
    # 'argument' : conversion from 'type1' to 'type2', possible loss of data
    # https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4244
    add_compile_options(/wd4244)

    # 'var' : conversion from 'size_t' to 'type', possible loss of data
    # https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-3-c4267
    add_compile_options(/wd4267)
endif()

add_subdirectory(src)

if (OPENDAQ_ENABLE_TESTS)
    add_subdirectory(tests)
endif()
