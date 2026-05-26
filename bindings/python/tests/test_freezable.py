#!/usr/bin/env python

import opendaq_test
import opendaq
import unittest


class TestFreezable(opendaq_test.TestCase):

    def test_type_exists(self):
        """IFreezable type should be available in the opendaq module."""
        self.assertTrue(hasattr(opendaq, 'IFreezable'))

    def test_can_cast_from_property_object(self):
        """PropertyObject implements IFreezable, so can_cast_from should return True."""
        prop_obj = opendaq.PropertyObject()
        self.assertTrue(opendaq.IFreezable.can_cast_from(prop_obj))

    def test_cast_from_property_object(self):
        """Casting a PropertyObject to IFreezable should succeed."""
        prop_obj = opendaq.PropertyObject()
        freezable = opendaq.IFreezable.cast_from(prop_obj)
        self.assertIsNotNone(freezable)

    def test_not_frozen_by_default(self):
        """A fresh PropertyObject should not be frozen."""
        prop_obj = opendaq.PropertyObject()
        freezable = opendaq.IFreezable.cast_from(prop_obj)
        self.assertFalse(freezable.is_frozen)

    def test_freeze(self):
        """Calling freeze() should make is_frozen return True."""
        prop_obj = opendaq.PropertyObject()
        freezable = opendaq.IFreezable.cast_from(prop_obj)

        freezable.freeze()
        self.assertTrue(freezable.is_frozen)

    def test_freeze_twice_no_error(self):
        """Freezing an already frozen object should not raise."""
        prop_obj = opendaq.PropertyObject()
        freezable = opendaq.IFreezable.cast_from(prop_obj)

        freezable.freeze()
        freezable.freeze()  # should not raise
        self.assertTrue(freezable.is_frozen)

    def test_write_blocked_after_freeze(self):
        """Writing to a frozen PropertyObject should raise."""
        prop_obj = opendaq.PropertyObject()
        prop_obj.add_property(
            opendaq.StringProperty(opendaq.String('testProp'),
                                   opendaq.String('initial'),
                                   opendaq.Boolean(True)))

        # Writing before freeze works
        prop_obj.set_property_value('testProp', 'new value')
        self.assertEqual(prop_obj.get_property_value('testProp'), 'new value')

        # Freeze the object
        freezable = opendaq.IFreezable.cast_from(prop_obj)
        freezable.freeze()

        # Writing after freeze should raise
        with self.assertRaises(BaseException):
            prop_obj.set_property_value('testProp', 'should fail')

        # Value should be unchanged
        self.assertEqual(prop_obj.get_property_value('testProp'), 'new value')

    def test_add_property_blocked_after_freeze(self):
        """Adding a property to a frozen PropertyObject should raise."""
        prop_obj = opendaq.PropertyObject()
        freezable = opendaq.IFreezable.cast_from(prop_obj)
        freezable.freeze()

        with self.assertRaises(BaseException):
            prop_obj.add_property(
                opendaq.StringProperty(opendaq.String('newProp'),
                                       opendaq.String('value'),
                                       opendaq.Boolean(True)))


if __name__ == '__main__':
    unittest.main()