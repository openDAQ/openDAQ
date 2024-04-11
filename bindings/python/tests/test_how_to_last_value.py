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


if __name__ == '__main__':
    unittest.main()
