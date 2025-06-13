#set_cmake_folder_context(TARGET_FOLDER_NAME)

set(GTest_REQUIREDVERSION "1.15.0")

find_package(GTest ${GTest_REQUIREDVERSION} GLOBAL)
if(GTest_FOUND)
    message(STATUS "Found GTest: ${GTest_VERSION} ${GTest_CONFIG}")
else()
    message(STATUS "Fetching GTest version ${GTest_REQUIREDVERSION}")

    include(FetchContent)
    get_custom_fetch_content_params(GTest FC_PARAMS)

    set(GTest_WITH_POST_BUILD_UNITTEST OFF)
    set(GTest_WITH_TESTS OFF)

    # set(BUILD_GMOCK OFF)
    set(INSTALL_GTEST OFF)
    set(gtest_force_shared_crt ON)
    FetchContent_Declare(
            GTest
            URL https://github.com/google/googletest/archive/refs/tags/v${GTest_REQUIREDVERSION}.zip
            URL_HASH SHA256=ed98258b96c6dc8af33eeb03c8d94a05b185866604382e12980f576595bdc509
            ${FC_PARAMS}
    )

    FetchContent_MakeAvailable(GTest)
    add_library(GTest::GTest ALIAS gtest)
    add_library(GTest::Main ALIAS gtest_main)

    # GMock somehow doesn't have proper include paths set for GTest dependency
    get_target_property(GTEST_INCLUDE_DIRS gtest INTERFACE_INCLUDE_DIRECTORIES)
    target_include_directories(gmock PRIVATE ${GTEST_INCLUDE_DIRS})
    target_include_directories(gmock_main PRIVATE ${GTEST_INCLUDE_DIRS})
endif()
