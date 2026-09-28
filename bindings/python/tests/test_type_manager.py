#!/usr/bin/env python
import unittest

import opendaq as daq
import opendaq_test


class TypeManager(opendaq_test.TestCase):

    def test_find_enum_type(self):
        instance = daq.Instance()
        type_manager = instance.context.type_manager
        self.assertTrue(type_manager.has_type("ComponentStatusType"))
        component_status_type = type_manager.get_type("ComponentStatusType")
        self.assertIsNotNone(component_status_type)
        self.assertIsInstance(component_status_type, daq.IEnumerationType)

    def test_find_struct_type(self):
        instance = daq.Instance()
        type_manager = instance.context.type_manager
        self.assertTrue(type_manager.has_type("PtpConfigurationStructure"))
        ptp_config_type = type_manager.get_type("PtpConfigurationStructure")
        self.assertIsNotNone(ptp_config_type)
        # TODO: self.assertIsInstance(ptp_config_type, daq.IStructType)
        self.assertIsInstance(ptp_config_type, daq.IType)

    def test_find_nonexistent_type(self):
        instance = daq.Instance()
        type_manager = instance.context.type_manager
        self.assertFalse(type_manager.has_type("NonExistentType"))
        with self.assertRaises(RuntimeError):
            type_manager.get_type("NonExistentType")


if __name__ == '__main__':
    unittest.main()
