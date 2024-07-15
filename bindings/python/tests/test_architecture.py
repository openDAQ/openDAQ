#!/usr/bin/env python

import unittest
import opendaq_test
import opendaq
import sys

class TestDocumentationArchitectureGuide(opendaq_test.TestCase):

    def test_start_simulator(self):
        # Create openDAQ instance
        instance = opendaq.Instance()

        # Add a reference device and set it as root
        instance.set_root_device('daqref://device0')

        # Start a web-socket streaming server
        instance.add_server('OpenDAQLTStreaming', None)

        # Start an openDAQ OPC UA and native streaming servers
        instance.add_standard_servers()

    def test_discovering_devices(self):
        # Create openDAQ instance
        instance = opendaq.Instance()

        # Find and output the names and connection strings of all available devices
        available_devices_info = instance.available_devices
        for device_info in available_devices_info:
            print("Name:", device_info.name, "Connection string:", device_info.connection_string)

    def test_connecting_to_device(self):
        # Create a fresh openDAQ instance we will use for all the interactions with the openDAQ SDK
        instance = opendaq.Instance()

        # Find and connect to a device hosting an OPC UA TMS server
        available_devices_info = instance.available_devices
        for device_info in available_devices_info:
            if 'daqref://' in device_info.connection_string:
                device = instance.add_device(device_info.connection_string)
                break
        else:
            # Exit if no device is found
            exit(1)

        # Output the name of the added device
        print(device.info.name)

    def test_read_signal(self):
        import time

        instance = opendaq.Instance()
        device = instance.add_device('daqref://device0')
        signal = device.signals_recursive[0]

        reader = opendaq.StreamReader(signal)

        time.sleep(0.1)
        samples = reader.read(100)
        if len(samples) > 0:
            print(samples[-1])

    # Corresponding document: Antora/modules/background_info/pages/function_blocks.adoc
    def test_channels(self):
        instance = opendaq.Instance()
        # doc code
        device = instance.add_device("daqref://device0")
        channels = device.channels
        #
        self.assertGreater(len(channels), 0)

    # Corresponding document: Antora/modules/background_info/pages/function_blocks.adoc
    def test_connect_signal(self):
        instance = opendaq.Instance()
        device = instance.add_device("daqref://device0")
        fb = instance.add_function_block("RefFBModuleStatistics")
        signal = device.signals_recursive[0]
        input_port = fb.input_ports[0]
        # doc code
        input_port.connect(signal)
        #

    # Corresponding document: Antora/modules/background_info/pages/function_blocks.adoc
    def test_create_fb(self):
        instance = opendaq.Instance()
        # doc code
        fb = instance.add_function_block("RefFBModuleStatistics")
        fbs = instance.function_blocks
        fb1 = fbs[-1]
        assert fb1 == fb
        #

    # Corresponding document: Antora/modules/background_info/pages/data_path.adoc
    def test_input_port_connection(self):
        instance = opendaq.Instance()
        device = instance.add_device("daqref://device0")
        fb = instance.add_function_block("RefFBModuleStatistics")
        signal = device.signals_recursive[0]
        input_port = fb.input_ports[0]
        # doc code
        input_port.connect(signal)
        connection = input_port.connection
        self.assertTrue(connection is not None)
        #

    # Corresponding document: Antora/modules/background_info/pages/data_path.adoc
    def test_connection_dequeue(self):
        instance = opendaq.Instance()
        device = instance.add_device("daqref://device0")
        fb = instance.add_function_block("RefFBModuleStatistics")
        signal = device.signals_recursive[0]
        input_port = fb.input_ports[0]
        input_port.connect(signal)
        # doc code
        packet = input_port.connection.dequeue()
        #
    
    # Corresponding document: Antora/modules/background_info/pages/components.adoc
    def test_search_filter(self):
        instance = opendaq.Instance()
        device = instance.add_device("daqref://device0")
        search_filter = opendaq.RecursiveSearchFilter(opendaq.AnySearchFilter())
        sigs = device.get_signals(search_filter)
        self.assertTrue(len(sigs) > 0)
        
    # Corresponding document: Antora/modules/howto_guides/pages/howto_configure_instance_providers.adoc
    def test_json_config_provider(self):
        config_path = "opendaq-config.json"
        config_provider = opendaq.JsonConfigProvider(opendaq.String(config_path))
        instance_builder = opendaq.InstanceBuilder()
        instance_builder.add_config_provider(config_provider)
        instance = instance_builder.build()
        
    # Corresponding document: Antora/modules/howto_guides/pages/howto_configure_instance_providers.adoc
    def test_env_config_provider(self):
        config_provider = opendaq.EnvConfigProvider()
        instance_builder = opendaq.InstanceBuilder()
        instance_builder.add_config_provider(config_provider)
        instance = instance_builder.build()
    
    # Corresponding document: Antora/modules/howto_guides/pages/howto_configure_instance_providers.adoc
    def create_cmd_line_args_config_provider(self):
        list = opendaq.List()
        for arg in sys.argv[1:]:
            list.push_back(arg)
        return opendaq.CmdLineArgsConfigProvider(list)

    def test_cmd_line_args_config_provider(self):
        config_provider = self.create_cmd_line_args_config_provider()
        instance_builder = opendaq.InstanceBuilder()
        instance_builder.add_config_provider(config_provider)
        instance = instance_builder.build()

if __name__ == '__main__':
    unittest.main()
