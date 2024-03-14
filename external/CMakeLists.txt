set_mode(ANCIENT)
set(CMAKE_FOLDER "external")

include(FetchContent)
include(opendaq_dependency.cmake)

list(APPEND CMAKE_MESSAGE_CONTEXT external)
set(CURR_MESSAGE_CONTEXT ${CMAKE_MESSAGE_CONTEXT})

if (NOT TARGET daq::testlibs AND OPENDAQ_ENABLE_TEST_UTILS)
    set(CMAKE_MESSAGE_CONTEXT ${CURR_MESSAGE_CONTEXT} googletest)

    if (BUILDING_AS_SUBMODULE)
        set(INSTALL_GTEST Off CACHE BOOL "" FORCE)
    endif()
    set(gtest_force_shared_crt On CACHE BOOL "Force shared CRT")

    set(gtest_EXTERNAL_PATH "" CACHE PATH "Path to external gtest")
    mark_as_advanced(gtest_EXTERNAL_PATH)

    if ("${gtest_EXTERNAL_PATH}" STREQUAL "")
        add_subdirectory(gtest)
    else()
        add_subdirectory("${gtest_EXTERNAL_PATH}" googletest_external)
    endif()

    set(GTEST_EXTERNAL_FOLDER "external/gtest")
    if (NOT gtest IN_LIST TESTING_LIBS)
        set(TESTING_LIBS gtest gmock ${TESTING_LIBS})
    endif()

    set(GTEST_TARGET GTest::gtest)
    set(GMOCK_TARGET GTest::gmock)

    if (TARGET gtest)
        set(GTEST_TARGET gtest)
        set(GMOCK_TARGET gmock)
    endif()

    set_target_properties(${GTEST_TARGET} PROPERTIES FOLDER ${GTEST_EXTERNAL_FOLDER})
    set_target_properties(${GTEST_TARGET}_main PROPERTIES FOLDER ${GTEST_EXTERNAL_FOLDER})
    set_target_properties(${GMOCK_TARGET} PROPERTIES FOLDER ${GTEST_EXTERNAL_FOLDER})
    set_target_properties(${GMOCK_TARGET}_main PROPERTIES FOLDER ${GTEST_EXTERNAL_FOLDER})

    target_include_directories(${GTEST_TARGET} INTERFACE ${TESTING_INCLUDES})

    opendaq_set_target_postfixes(${GTEST_TARGET})
    opendaq_set_target_postfixes(${GMOCK_TARGET}_main)

    opendaq_set_target_postfixes(${GMOCK_TARGET})
    opendaq_set_target_postfixes(${GMOCK_TARGET}_main)

    set(TEST_LIBS_TARGET testlibs)
    add_library(${TEST_LIBS_TARGET} INTERFACE)
    add_library(${SDK_TARGET_NAMESPACE}::${TEST_LIBS_TARGET} ALIAS ${TEST_LIBS_TARGET})
    target_link_libraries(${TEST_LIBS_TARGET} INTERFACE ${TESTING_LIBS})
endif()

set(CMAKE_MESSAGE_CONTEXT ${CURR_MESSAGE_CONTEXT})

if(OPENDAQ_ENABLE_OPCUA)
    add_subdirectory(open62541 EXCLUDE_FROM_ALL)
endif()

add_subdirectory(boost)
add_subdirectory(date)
add_subdirectory(fmt)
add_subdirectory(mdns EXCLUDE_FROM_ALL)
if (OPENDAQ_GENERATE_PYTHON_BINDINGS)
    add_subdirectory(pybind11 EXCLUDE_FROM_ALL)
endif()
#
add_subdirectory(rapidjson)
add_subdirectory(spdlog EXCLUDE_FROM_ALL)
add_subdirectory(xxHash)
add_subdirectory(taskflow EXCLUDE_FROM_ALL)
add_subdirectory(tsl-ordered-map)

if (DAQMODULES_REF_FB_MODULE AND DAQMODULES_REF_FB_MODULE_ENABLE_RENDERER)
    add_subdirectory(sfml EXCLUDE_FROM_ALL)
endif()

if (DAQMODULES_AUDIO_DEVICE_MODULE)
    add_subdirectory(miniaudio EXCLUDE_FROM_ALL)
endif()

if(OPENDAQ_ENABLE_OPCUA)
    add_subdirectory(opcua_daq_types EXCLUDE_FROM_ALL)
endif()

if(OPENDAQ_ENABLE_WEBSOCKET_STREAMING)
    add_subdirectory(streaming_protocol EXCLUDE_FROM_ALL)
endif()

if(OPENDAQ_MIMALLOC_SUPPORT)
    add_subdirectory(mimalloc)
endif()

if(OPENDAQ_ENABLE_NATIVE_STREAMING)
    add_subdirectory(native_streaming EXCLUDE_FROM_ALL)
endif()
