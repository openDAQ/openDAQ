#!/usr/bin/env python
import unittest
import opendaq_test
import opendaq as daq

class IntegerTest(opendaq_test.TestCase):

    def test_create_int(self):
        i = daq.Integer(42)
        self.assertEqual(i.value, 42)

    def test_equal(self):
        i = daq.Integer(42)
        i2 = daq.Integer(42)
        j = daq.Integer(43)

        self.assertEqual(i, 42)
        self.assertEqual(int(i), 42)
        self.assertEqual(i.value, 42)

        self.assertNotEqual(i, 0)
        self.assertNotEqual(i, 21)
        self.assertEqual(i, 42)

        self.assertFalse(i == 0)
        self.assertFalse(i == 21)
        self.assertTrue(i == 42)

        self.assertEqual(i, i2)
        self.assertNotEqual(i, j)

if __name__ == '__main__':
    unittest.main()
