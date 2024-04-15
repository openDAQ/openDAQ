#!/usr/bin/env python

import unittest
import opendaq_test
import opendaq

class TestDocumentationHowTo(opendaq_test.TestCase):
    # Corresponding document: Antora/modules/howto_guides/pages/howto_add_function_block.adoc
    def test_add_function_block(self):
        # Create an openDAQ(TM) Instance, loading modules from the current directory
        instance = opendaq.Instance()

        # Add simulated device
        device = instance.add_device('daqref://device0')
        
        # Get available Function Block types
        function_block_types = instance.available_function_block_types
        for function_block_type in function_block_types.keys():
            print(function_block_type)

        # If there is no Statistics Function Block available, exit with an error
        if not "ref_fb_module_statistics" in function_block_types.keys():
            self.assertTrue(False, "Function block not found")

        # Add Function Block on the host computer
        function_block = instance.add_function_block("ref_fb_module_statistics")

        # Print Function Block type info
        function_block_type = function_block.function_block_type
        print(function_block_type.id)
        print(function_block_type.name)
        print(function_block_type.description)

    def test_configure_function_block(self):
        # Create an openDAQ(TM) instance, loading modules from the current directory
        instance = opendaq.Instance()

        # add simulated device
        device = instance.add_device('daqref://device0')

        # add function block on the host computer
        function_block = instance.add_function_block("ref_fb_module_statistics")

        #list properties of the function block
        function_block_properties = function_block.visible_properties
        for prop in function_block_properties:
            print(prop.name)

        # print current block size
        currentBlockSize = function_block.get_property_value("BlockSize")
        print("Current block size is %d" % (currentBlockSize))

        # configure the properties of the function block
        function_block.set_property_value("BlockSize", 100)

        # connect the first signal of the first channel from the device to the first input port on the function block
        function_block.input_ports[0].connect(device.channels[0].signals[0])

        # get the output signal of the function block
        output_signal = function_block.signals[0]

        print(output_signal.descriptor.name)

    def test_save_load_configuration(self):
        instance = opendaq.Instance()

        # save configuration to string
        json_str = instance.save_configuration();

        # write configuration string to file
        config_file = open("config.json", "w")
        config_file.write(json_str)
        config_file.close()

        instance = opendaq.Instance()

        # read configuration from file
        config_file = open("config.json", "r")
        json_str = config_file.read();
        config_file.close()

        # load configuration from string
        instance.load_configuration(json_str);

if __name__ == '__main__':
    unittest.main()
