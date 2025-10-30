set(TEST_APP test_ref_licensing)

set(TEST_SOURCES test_app.cpp
)

get_directory_property(LIC_EXAMPLE_SOURCE_CPP DIRECTORY ${CMAKE_SOURCE_DIR}/examples/applications/cpp/licensing_example/src DEFINITION MODULE_AUTHENTICATOR_SOURCE)
get_directory_property(LIC_EXAMPLE_SOURCE_H DIRECTORY ${CMAKE_SOURCE_DIR}/examples/applications/cpp/licensing_example/src DEFINITION MODULE_AUTHENTICATOR_HEADER)
get_directory_property(LIC_MODULE_LIB DIRECTORY ${CMAKE_SOURCE_DIR}/examples/modules/licensing_module/src DEFINITION LIB_NAME)

set(OUT_FILE_NAME "$<TARGET_FILE_NAME:${LIC_MODULE_LIB}>")

message(LIC_EXAMPLE_SOURCE_CPP="${LIC_EXAMPLE_SOURCE_CPP}")
message(OUT_FILE_NAME="${OUT_FILE_NAME}")

add_executable(${TEST_APP} ${TEST_SOURCES}
				${CMAKE_SOURCE_DIR}/examples/applications/cpp/licensing_example/include/licensing_example/${LIC_EXAMPLE_SOURCE_H}
				${CMAKE_SOURCE_DIR}/examples/applications/cpp/licensing_example/src/${LIC_EXAMPLE_SOURCE_CPP}
)

target_include_directories(${TEST_APP} PUBLIC ${CMAKE_SOURCE_DIR}/examples/applications/cpp/licensing_example/include/)

target_link_libraries(${TEST_APP} PRIVATE daq::test_utils
                                          daq::opendaq
)

add_dependencies(${TEST_APP} daq::licensing_module)

add_test(NAME ${TEST_APP}
         COMMAND $<TARGET_FILE_NAME:${TEST_APP}>
         WORKING_DIRECTORY $<TARGET_FILE_DIR:${TEST_APP}>
)

if (MSVC)
    target_compile_options(${TEST_APP} 
        PRIVATE 
            /bigobj 
            /wd4100     #Avoid 'unreferenced formal parameter' compiler warning  (Otherwise will not compile in VS in Release build...)
            /ZI         #Enable Hot Reload per default (see: ) https://learn.microsoft.com/en-us/visualstudio/debugger/edit-and-continue-visual-cpp?view=vs-2022#BKMK_Enable_or_disable_automatic_invocation_of_Edit_and_Continue
    )

    target_link_libraries(${TEST_APP}
        PRIVATE 
            wintrust        # Required for License Certificate verification
            Crypt32         # Required for License Certificate verification
    )
endif()

if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()

if (LINUX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GPGME REQUIRED gpgme)

    target_include_directories(${TEST_APP} PRIVATE ${GPGME_INCLUDE_DIRS})
    target_link_libraries(${TEST_APP} PRIVATE ${GPGME_LIBRARIES})
endif()

set_target_properties(${TEST_APP} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:${TEST_APP}>)

if (OPENDAQ_ENABLE_COVERAGE)
    setup_target_for_coverage(${TEST_APP}coverage ${TEST_APP} ${TEST_APP}coverage)
endif()
