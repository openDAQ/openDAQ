#!/usr/bin/env python

import opendaq_test
import opendaq as daq
import unittest


class TestFunctionBlockType(opendaq_test.TestCase):

    def test_input_policy_enum(self):
        self.assertEqual(list(daq.FunctionBlockInputPolicy.__members__),
                         ['Unspecified', 'Fixed', 'OneAlwaysEmpty', 'SettingsDriven'])

    def test_default_options(self):
        fb_type = daq.FunctionBlockType('Id', 'Name', 'Description', None)
        self.assertEqual(fb_type.input_policy, daq.FunctionBlockInputPolicy.Unspecified)
        self.assertFalse(fb_type.singleton)
        self.assertIsNone(fb_type.common_settings_type_id)


if __name__ == '__main__':
    unittest.main()
