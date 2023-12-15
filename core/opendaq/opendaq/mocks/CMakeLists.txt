set(MOCK_LIB_NAME ${SDK_TARGET_NAME}_mocks)

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
)

opendaq_prepend_path(include/${SDK_TARGET_NAME}/mock MOCK_HEADERS)

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
)

source_group("mock" FILES ${MOCK_HEADERS}
                          ${MOCK_SOURCES}
)

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

opendaq_prepend_path(include/${SDK_TARGET_NAME}/gmock GMOCK_HEADERS)

source_group("gmock" FILES ${GMOCK_HEADERS}
)

add_library(${MOCK_LIB_NAME} STATIC ${GMOCK_HEADERS} ${MOCK_HEADERS} ${MOCK_SOURCES})
add_library(${SDK_TARGET_NAMESPACE}::${MOCK_LIB_NAME} ALIAS ${MOCK_LIB_NAME})

target_link_libraries(${MOCK_LIB_NAME} PRIVATE daq::opendaq
                                               daq::server_dev
                                               daq::test_utils
                                       PUBLIC  daq::objects_mocks
                                               daq::signal_generator
)

target_include_directories(${MOCK_LIB_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

