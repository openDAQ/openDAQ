set(MOCK_LIB_NAME ${SDK_TARGET_NAME}_mocks)
set(MOCK_INCLUDE_PREFIX ${SDK_TARGET_NAME}/mock)

set(MOCK_HEADERS mock_device_module.h
                 mock_fb_module.h
                 mock_fb.h
                 mock_fb_factory.h
                 mock_nested_fb.h
                 mock_nested_fb_factory.h
                 mock_channel.h
                 mock_channel_factory.h
                 mock_physical_device.h
                 mock_server_module.h
                 mock_server.h
                 mock_server_factory.h
                 mock_streaming.h
                 mock_streaming_factory.h
                 default_mocks.h
                 default_mocks_factory.h
                 mock_fb_dynamic_input_ports.h
                 mock_fb_dynamic_output_ports.h
                 advanced_components_setup_utils.h
)

prepend_include(${MOCK_INCLUDE_PREFIX} MOCK_HEADERS)

set(MOCK_SOURCES mock_device_module.cpp
                 mock_fb_module.cpp
                 mock_fb.cpp
                 mock_nested_fb.cpp
                 mock_channel.cpp
                 mock_physical_device.cpp
                 mock_server_module.cpp
                 mock_server.cpp
                 mock_streaming.cpp
                 default_mocks.cpp
                 mock_fb_dynamic_input_ports.cpp
                 mock_fb_dynamic_output_ports.cpp
                 advanced_components_setup_utils.cpp
)

source_group("mock" FILES ${MOCK_HEADERS}
                          ${MOCK_SOURCES}
)

add_library(${MOCK_LIB_NAME} STATIC ${GMOCK_HEADERS} ${MOCK_HEADERS} ${MOCK_SOURCES})
add_library(${SDK_TARGET_NAMESPACE}::${MOCK_LIB_NAME} ALIAS ${MOCK_LIB_NAME})

if (MSVC)
    target_compile_options(${MOCK_LIB_NAME} PRIVATE /bigobj)
endif()

target_link_libraries(${MOCK_LIB_NAME} PRIVATE daq::opendaq
                                       PUBLIC  daq::signal_generator
)

target_include_directories(${MOCK_LIB_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include>

        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

install(TARGETS ${MOCK_LIB_NAME}
        EXPORT ${SDK_NAME}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(FILES ${MOCK_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${MOCK_INCLUDE_PREFIX}
)


set(GMOCK_LIB_NAME ${SDK_TARGET_NAME}_gmocks)
set(GMOCK_INCLUDE_PREFIX ${SDK_TARGET_NAME}/gmock)

set(GMOCK_HEADERS context.h
                  input_port.h
                  input_port_notifications.h
                  packet.h
                  scheduler.h
                  signal.h
                  task_graph.h
                  component.h
                  device.h
                  allocator.h
                  function_block.h
                  streaming.h
)

prepend_include(${GMOCK_INCLUDE_PREFIX} GMOCK_HEADERS)

set(GMOCK_DUMMY_SOURCES dummy.cpp
)

source_group("gmock" FILES ${GMOCK_HEADERS}
)

add_library(${GMOCK_LIB_NAME} STATIC ${GMOCK_HEADERS} ${GMOCK_DUMMY_SOURCES})
add_library(${SDK_TARGET_NAMESPACE}::${GMOCK_LIB_NAME} ALIAS ${GMOCK_LIB_NAME})

if (MSVC)
    target_compile_options(${GMOCK_LIB_NAME} PRIVATE /bigobj)
endif()

target_link_libraries(${GMOCK_LIB_NAME} PRIVATE daq::opendaq
                                                gmock
                                        PUBLIC  daq::objects_gmocks
)

target_include_directories(${GMOCK_LIB_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include>

        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

