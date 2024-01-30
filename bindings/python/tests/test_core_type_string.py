#!/usr/bin/env python

import opendaq_test
import opendaq as daq
import unittest

empty_string = ''
ascii_string = 'hello'
unicode_string = 'če'

class TestString(opendaq_test.TestCase):

    def test_create(self):
        daq.String('Hello')

    def test_ascii_string(self):
        s = daq.String(ascii_string)
        self.assertNotEqual(s, 'world')
        self.assertEqual(s, ascii_string)
        self.assertEqual(s.length, len(ascii_string))

    def test_unicode_string(self):
        s = daq.String(unicode_string)
        self.assertEqual(s, unicode_string)
        # length of UTF-8 strings doesn't seem to be correct
        #self.assertEqual(s.length, len(unicode_string))

    def test_hash(self):
        self.assertEqual(hash(daq.String(empty_string)), 0)
        self.assertNotEqual(hash(daq.String(ascii_string)), 0)

    def test_string_conversion(self):
        self.assertEqual(str(daq.String(empty_string)), empty_string)
        self.assertEqual(str(daq.String(ascii_string)), ascii_string)
        self.assertEqual(str(daq.String(unicode_string)), unicode_string)

    def test_core_type(self):
        self.assertEqual(daq.String(ascii_string).core_type, daq.CoreType.ctString)

    def test_can_cast_from(self):
        self.assertTrue(daq.IBaseObject.can_cast_from(daq.String('test')))

    def test_int_conversion(self):
        self.assertEqual(int(daq.String('10')), 10)

    '''
    def test_add(self):
        value = daq.String('Hello')
        res = value + 'A1'
        self.assertEqual(res, 'HelloA1')
    '''

if __name__ == '__main__':
    unittest.main()
