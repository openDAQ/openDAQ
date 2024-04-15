#!/usr/bin/env python

import unittest
import opendaq_test
import opendaq


# Corresponding document: Antora/modules/howto_guides/pages/howto_measure_single_value.adoc
class TestDocumentationHowToLastValue(opendaq_test.TestCase):
    def test_last_value_signal(self):
        mock = opendaq.MockSignal()
        mock.add_data([0.1, 0.2, 0.3])
        my_signal = mock.signal

        # START DOCS CODE

        # Get last value of a Signal
        my_last_value = my_signal.last_value

        # END DOCS CODE

        self.assertEqual(my_last_value, 0.3)

    '''
    # ONLY WORKS IF APPROPRIATE SERVER IS RUNNING ON "daq.opcua://localhost"
    def test_last_value_signal_list(self):
        instance = opendaq.Instance()
        dev = instance.add_device("daq.opcua://localhost")
        my_signal = dev.signals[0]

        # START DOCS CODE

        # Retrieve the Signal's Sample Type
        my_sample_type = my_signal.descriptor.sample_type

        # Check Dimensions count in Signal's Data Descriptor
        assert len(my_signal.descriptor.dimensions) == 1
        # Get last value of a Signal
        my_list = my_signal.last_value
        # Check the number of elements in List
        assert len(my_list) == 2
        # Extract the second item on list
        my_item = my_list[1]

        # END DOCS CODE
        self.assertEqual(my_item, 44)
        self.assertEqual(my_sample_type, opendaq.SampleType.Int64)
    '''

if __name__ == '__main__':
    unittest.main()
