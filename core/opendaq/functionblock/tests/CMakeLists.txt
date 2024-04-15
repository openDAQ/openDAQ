set_cmake_folder_context(TARGET_FOLDER_NAME)
set(MODULE_NAME function_block)

set(TEST_HEADERS
)

set(TEST_MOCKS mock/mock_channel.h
               mock/mock_channel.cpp
)

set(TEST_SOURCES test_factories.cpp
				 test_channel.cpp
                 test_fb_wrapper.cpp
                 test_function_block.cpp
				 ${TEST_HEADERS}
				 ${TEST_MOCKS}
)

opendaq_prepare_test_runner(TEST_APP FOR ${MODULE_NAME}
                       SOURCES
                           test_app.cpp
                           ${TEST_SOURCES}
                           ${TEST_HEADERS}
)

target_link_libraries(${TEST_APP}
    PRIVATE
        daq::coreobjects
        daq::coretypes
        daq::opendaq_mocks
)

if(CMAKE_COMPILER_IS_GNUCXX)
    set(STDCFS_LIBS "stdc++fs")
endif()

if (MSVC)
    target_compile_options(${TEST_APP} PRIVATE /bigobj)
endif()

target_link_libraries(${TEST_DLL_TARGET}
    PRIVATE $<TARGET_OBJECTS:daq::module_manager>
            daq::discovery_ping
            ${CMAKE_DL_LIBS}
            ${STDCFS_LIBS}
)

add_test(NAME ${TEST_APP}
         COMMAND $<TARGET_FILE_NAME:${TEST_APP}>
         WORKING_DIRECTORY bin
)

source_group("Mocks" FILES ${TEST_MOCKS})

if (OPENDAQ_ENABLE_COVERAGE)
    setup_target_for_coverage(${TEST_APP}coverage ${TEST_APP} ${TEST_APP}coverage)
endif()
