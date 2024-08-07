#!/usr/bin/env python

import opendaq_test
import opendaq
import unittest


class TestBeginUpdateEndUpdate(opendaq_test.TestCase):

    def test_props(self):
        property_object = opendaq.PropertyObject()
        property_object.add_property(opendaq.StringProperty(opendaq.String(
            'property1'), opendaq.String('value1'), opendaq.Boolean(True)))
        property_object.add_property(opendaq.IntProperty(opendaq.String(
            'property2'), opendaq.Integer(2), opendaq.Boolean(True)))

        self.assertEqual(property_object.get_property_value('property1'), 'value1')
        self.assertEqual(property_object.get_property_value('property2'), 2)

        property_object.begin_update()
        property_object.set_property_value('property1', 'value')
        property_object.set_property_value('property2', 3)
        self.assertEqual(property_object.get_property_value('property1'), 'value1')
        self.assertEqual(property_object.get_property_value('property2'), 2)
        property_object.end_update()

        self.assertEqual(property_object.get_property_value('property1'), 'value')
        self.assertEqual(property_object.get_property_value('property2'), 3)

    def test_updating(self):
        property_object = opendaq.PropertyObject()

        self.assertFalse(property_object.updating)

        property_object.begin_update()
        self.assertTrue(property_object.updating)
        property_object.end_update()

        self.assertFalse(property_object.updating)

if __name__ == '__main__':
    unittest.main()
