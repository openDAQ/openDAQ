#!/usr/bin/env python

import unittest
import opendaq_test
import opendaq as daq

class TestBaseObject(opendaq_test.TestCase):
    def test_create(self):
        daq.BaseObject()

    def test_hash(self):
        obj = daq.BaseObject()

        hash_code = obj.__hash__()
        self.assertNotEqual(hash_code, 0)

        hash_code = hash(obj)
        self.assertNotEqual(hash_code, 0)

    def test_core_type(self):
        obj = daq.BaseObject()

        core_type = obj.core_type
        self.assertEqual(core_type, daq.CoreType.ctObject)

    def test_can_cast_from(self):
        self.assertTrue(daq.IBaseObject.can_cast_from(daq.BaseObject()))

    '''
    def test_to_string(self):
        s = str(daq.BaseObject())
        self.assertEqual(s, "daq::IBaseObject")
    '''

    def test_to_int(self):
        with self.assertRaisesRegex(RuntimeError, 'Invalid cast'):
            int(daq.BaseObject())

    def test_compare(self):
        obj1 = daq.BaseObject()
        obj2 = daq.BaseObject()
        self.assertTrue(obj1 == obj1)
        self.assertTrue(obj1 != obj2)

class TestBoolean(opendaq_test.TestCase):
    def test_create(self):
        daq.Boolean(True)

    def test_hash(self):
        self.assertEqual(hash(daq.Boolean(True)), 1)
        self.assertEqual(hash(daq.Boolean(False)), 0)

    def test_bool_conversion(self):
        self.assertTrue(daq.Boolean(1))
        self.assertFalse(daq.Boolean(0))
        self.assertTrue(daq.Boolean(True))
        self.assertFalse(daq.Boolean(False))
        self.assertTrue(bool(daq.Boolean(True)))
        self.assertFalse(bool(daq.Boolean(False)))
        self.assertTrue(daq.Boolean(True).value)
        self.assertFalse(daq.Boolean(False).value)

    def test_equal(self):
        self.assertTrue(daq.Boolean(True) == True)
        self.assertTrue(daq.Boolean(False) == False)
        self.assertFalse(daq.Boolean(True) == False)
        self.assertFalse(daq.Boolean(False) == True)
        self.assertEqual(daq.Boolean(True), True)
        self.assertEqual(daq.Boolean(False), False)
        self.assertNotEqual(daq.Boolean(True), False)
        self.assertNotEqual(daq.Boolean(False), True)

    def test_core_type(self):
        self.assertEqual(daq.Boolean(True).core_type, daq.CoreType.ctBool)

    def test_can_cast_from(self):
        self.assertTrue(daq.IBaseObject.can_cast_from(daq.Boolean(True)))

class TestNumObjectWrapper:
    class TestNumObject(opendaq_test.TestCase):
        def test_create(self):
            self.createObject(0)

        def test_hash(self):
            self.customAssertEqual(hash(self.createObject(0)), 0)
            self.customAssertEqual(hash(self.createObject(1230)), 1230)

        def test_int_conversion(self):
            self.customAssertEqual(int(self.createObject(12)), 12)

        '''
        def test_add(self):
            value = self.createObject(2)
            sum = value + 3
            self.customAssertEqual(sum, 5)

        def test_sub(self):
            value = self.createObject(4)
            res = value - 3
            self.customAssertEqual(res, 1)

        def test_mul(self):
            value = self.createObject(2)
            res = value * 3
            self.customAssertEqual(res, 6)

        def test_div(self):
            value = self.createObject(6)
            res = value // 3
            self.customAssertEqual(res, 2)
        '''

class TestInteger(TestNumObjectWrapper.TestNumObject):
    def createObject(self, num):
        return daq.Integer(int(num))

    def customAssertEqual(self, val1, val2):
        self.assertEqual(val1, val2)

    def test_core_type(self):
        self.assertEqual(daq.Integer(0).core_type, daq.CoreType.ctInt)

    def test_can_cast_from(self):
        self.assertTrue(daq.IBaseObject.can_cast_from(daq.Integer(2)))

class TestFloat(TestNumObjectWrapper.TestNumObject):
    def createObject(self, num):
        return daq.Float(float(num))

    def customAssertEqual(self, val1, val2):
        self.assertEqual(val1, val2)

    def test_core_type(self):
        self.assertEqual(daq.Float(2.0).core_type, daq.CoreType.ctFloat)

    def test_can_cast_from(self):
        self.assertTrue(daq.IBaseObject.can_cast_from(daq.Float(2.0)))

class TestList(opendaq_test.TestCase):
    def test_create(self):
        daq.List()

    def test_hash(self):
        list = daq.List()
        h = hash(list)
        self.assertNotEqual(h, 0)

    def test_core_type(self):
        self.assertEqual(daq.List().core_type, daq.CoreType.ctList)

    '''
    def test_convert_fail(self):
        list = daq.List()
        with self.assertRaisesRegex(daq.Error, "No interface"):
            int(list)
    '''

    def test_get_count(self):
        self.assertEqual(len(daq.List()), 0)

    def test_push_and_pop(self):
        list = daq.List()
        val = daq.Integer(2)
        list.push_back(val)
        self.assertEqual(len(list), 1)
        val1 = list.pop_back()
        self.assertEqual(val, val1)
        self.assertEqual(len(list), 0)

    def test_push_python_object(self):
        list = daq.List()
        list.push_back(1)
        self.assertEqual(len(list), 1)
        value = list.pop_back()
        self.assertIsInstance(value, int)
        self.assertEqual(value, 1)

    def test_push_back(self):
        list = daq.List()
        list.push_back(3)
        list.push_back(4)
        self.assertEqual(len(list), 2)
        self.assertEqual(list[1], 4)

    def test_push_front(self):
        list = daq.List()
        list.push_front(3)
        list.push_front(4)
        self.assertEqual(len(list), 2)
        self.assertEqual(list[1], 3)

    def test_pop_back(self):
        list = daq.List()
        list.push_back(3)
        list.push_back(4)
        self.assertEqual(list.pop_back(), 4)
        self.assertEqual(len(list), 1)

    def test_pop_front(self):
        list = daq.List()
        list.push_back(3)
        list.push_back(4)
        self.assertEqual(list.pop_front(), 3)
        self.assertEqual(len(list), 1)

    def test_get_item_at_slice(self):
        list = daq.List()

        n = 20
        k = 2
        for i in range(n):
            list.append(i)

        slice = list[0:n:k]
        for i in range(n//k):
            self.assertEqual(slice[i], k*i)

    def test_get_item_at(self):
        list = daq.List()

        list.push_back(3)
        list.push_back(False)
        list.push_back(1.0)
        list.push_back(daq.BaseObject())
        list.push_back("Test")

        value = list[0]
        self.assertEqual(value, 3)
        self.assertTrue(isinstance(value, int))

        value = list[1]
        self.assertEqual(value, False)
        #self.assertTrue(isinstance(value, bool))

        value = list[2]
        self.assertEqual(value, 1.0)
        self.assertTrue(isinstance(value, float))

        value = list[3]
        self.assertTrue(isinstance(value, daq.IBaseObject))

        value = list[4]
        self.assertEqual(value, "Test")
        self.assertTrue(isinstance(value, str))

    def test_get_last_item(self):
        list = daq.List()

        list.push_back(3)
        list.push_back(2)
        list.push_back(1)
        self.assertEqual(list[-1], 1)

class TestIterable(opendaq_test.TestCase):
    def test_from_iterable(self):
        l = daq.List()
        l.push_back(1)
        l.push_back(2)
        l.push_back(3)
        iterable = daq.IIterable.cast_from(l)

        i = 1
        for item in iterable:
            self.assertEqual(item, i)
            i = i + 1

    def test_from_list(self):
        l = daq.List()
        l.push_back(1)
        l.push_back(2)
        l.push_back(3)

        i = 1
        for item in l:
            self.assertEqual(item, i)
            i = i + 1


class TestDict(opendaq_test.TestCase):
    def test_create(self):
        daq.Dict()

    def test_hash(self):
        dict = daq.Dict()
        h = hash(dict)
        self.assertNotEqual(h, 0)

    def test_core_type(self):
        self.assertEqual(daq.Dict().core_type, daq.CoreType.ctDict)

    def test_get_count(self):
        self.assertEqual(len(daq.Dict()), 0)

    def test_insert_get_and_remove(self):
        dict = daq.Dict()
        dict[2] = "Test"
        dict["A"] = 3
        dict[daq.Integer(8)] = daq.Integer(4)
        self.assertEqual(len(dict), 3)
        self.assertEqual(dict[2], "Test")
        self.assertEqual(dict["A"], 3)
        self.assertEqual(dict[8], 4)
        del dict[2]
        self.assertEqual(len(dict), 2)
        self.assertEqual(dict.pop("A"), 3)
        self.assertEqual(len(dict), 1)

    def test_keys(self):
        dict = daq.Dict()
        dict[0] = "_0"
        dict[1] = "_1"
        dict[2] = "_2"
        keys = dict.keys()
        self.assertCountEqual(keys, [0, 1, 2])

    def test_values(self):
        dict = daq.Dict()
        dict[0] = "_0"
        dict[1] = "_1"
        dict[2] = "_2"
        values = dict.values()
        self.assertCountEqual(values, ["_0", "_1", "_2"])

    def test_items(self):
        dict = daq.Dict()
        dict[0] = "_0"
        dict[1] = "_1"
        dict[2] = "_2"
        items = dict.items()
        self.assertCountEqual(items, [(0, "_0"), (1, "_1"), (2, "_2")])

    def test_forrange(self):
        dict = daq.Dict()
        dict[0] = "_0"
        dict[1] = "_1"
        dict[2] = "_2"
        i = 0
        for key in dict:
            self.assertEqual(key, i)
            i = i + 1

    def test_clear(self):
        dict = daq.Dict()
        dict[0] = "_0"
        dict[1] = "_1"
        dict[2] = "_2"
        dict.clear()
        self.assertEqual(len(dict), 0)

    def test_iterate(self):
        dict = daq.Dict()
        dict['a'] = daq.String('A')
        dict['b'] = daq.String('B')
        dict['c'] = daq.String('C')

        for key in dict:
            self.assertEqual(key.upper(), dict[key])
        for key in dict.keys():
            self.assertEqual(key.upper(), dict[key])
        for key, value in dict.items():
            self.assertEqual(key.upper(), value)

class TestLeaks(opendaq_test.TestCase):

    def test_leak1(self):
        l = daq.List()
        l.append(daq.List())

        a = l[0]
        a = l[0]

class TestProcedure(opendaq_test.TestCase):

    def test_no_params(self):

        value = 10
        def set_value():
            nonlocal value
            value = 20
        proc = daq.Procedure(set_value)
        proc()

        self.assertEqual(value, 20)

    def test_one_param(self):

        value = 10
        def set_value(v):
            nonlocal value
            value += v
        proc = daq.Procedure(set_value)
        proc(10)

        self.assertEqual(value, 20)

    def test_two_params(self):

        value = 10
        def set_value(v1, v2):
            nonlocal value
            value += v1 + v2
        proc = daq.Procedure(set_value)
        proc(8, 2)

        self.assertEqual(value, 20)

class TestFunction(opendaq_test.TestCase):

    def test_no_params(self):

        def set_value():
            return 10
        func = daq.Function(set_value)
        value = func()

        self.assertEqual(value, 10)

    def test_one_param(self):

        def set_value(v):
            return v
        func = daq.Function(set_value)
        value = func(10)

        self.assertEqual(value, 10)

    def test_two_param(self):

        def set_value(v1, v2):
            return v1 - v2
        func = daq.Function(set_value)
        value = func(20, 10)

        self.assertEqual(value, 10)

class TestStruct(opendaq_test.TestCase):

    def test_struct_types(self):
        type_manager = daq.TypeManager()
        
        #nested struct
        nested_names = daq.List()
        nested_names.append(daq.String("string"))
        nested_names.append(daq.String("int"))

        nested_typeList = daq.List()
        nested_typeList.append(daq.SimpleType(daq.CoreType.ctString))
        nested_typeList.append(daq.SimpleType(daq.CoreType.ctInt))

        nested_default_values = daq.List()
        nested_default_values.append(daq.String('string'))
        nested_default_values.append(daq.Integer(10))

        nested_type = daq.StructType(daq.String("nested"), nested_names, nested_default_values, nested_typeList)
        type_manager.add_type(nested_type)

        #main struct
        names = daq.List()
        names.append('bool')
        names.append('int')
        names.append('float')
        names.append('string')
        names.append('list')
        names.append('dict')
        names.append('ratio')
        names.append('complexnumber')
        names.append('struct')
        names.append('enumeration')
        names.append('undefined')

        typeList = daq.List()
        typeList.append(daq.SimpleType(daq.CoreType.ctBool))
        typeList.append(daq.SimpleType(daq.CoreType.ctInt))
        typeList.append(daq.SimpleType(daq.CoreType.ctFloat))
        typeList.append(daq.SimpleType(daq.CoreType.ctString))
        typeList.append(daq.SimpleType(daq.CoreType.ctList))
        typeList.append(daq.SimpleType(daq.CoreType.ctDict))
        typeList.append(daq.SimpleType(daq.CoreType.ctRatio))
        typeList.append(daq.SimpleType(daq.CoreType.ctComplexNumber))
        typeList.append(nested_type)
        typeList.append(daq.SimpleType(daq.CoreType.ctEnumeration))
        typeList.append(daq.SimpleType(daq.CoreType.ctUndefined))

        default_values = daq.List()
        default_values.append(daq.Boolean(True))
        default_values.append(daq.Integer(10))
        default_values.append(daq.Float(10))
        default_values.append(daq.String('string'))

        vals_list = daq.List()
        vals_list.append(daq.String('item0'))
        vals_list.append(daq.String('item1'))
        default_values.append(vals_list)

        vals_dict = daq.Dict()
        vals_dict[daq.String('key0')] = daq.String('val0')
        vals_dict[daq.String('key1')] = daq.String('val1')
        default_values.append(vals_dict)

        default_values.append(daq.Ratio(1, 2))
        default_values.append(daq.ComplexNumber(1.0, 1.0))

        nested_struct = daq.Struct(daq.String('nested'), daq.Dict(), type_manager)
        default_values.append(nested_struct)

        enum_vals = daq.List()
        enum_vals.append(daq.String('enum0'))
        enum_vals.append(daq.String('enum1'))

        enum_type = daq.EnumerationType(daq.String('enum'), enum_vals, 0)
        type_manager.add_type(enum_type)

        enum = daq.Enumeration(daq.String('enum'), daq.String('enum0'), type_manager)
        default_values.append(enum)

        default_values.append(None)

        type = daq.StructType(daq.String("foo"), names, default_values, typeList)
        type_manager.add_type(type)

        bldr = daq.StructBuilder(daq.String('foo'), type_manager)
        struct = bldr.build()

        self.assertEqual(struct.bool, True)
        self.assertEqual(struct.int, 10)
        self.assertEqual(struct.float, 10)
        self.assertEqual(struct.string, 'string')
        self.assertEqual(struct.list[0], vals_list[0])
        self.assertEqual(struct.dict['key0'], vals_dict['key0'])
        self.assertEqual(struct.ratio.numerator, 1)
        self.assertEqual(struct.complexnumber.real, 1.0)
        self.assertEqual(struct.struct.string, 'string')
        self.assertEqual(struct.enumeration.name, 'enum0')
        self.assertIsNone(struct.undefined)

    def test_typenames_validation(self):
        type_manager = daq.TypeManager()
        valid_names = daq.List()
        valid_names.append('_')

        typeList = daq.List()
        typeList.append(daq.SimpleType(daq.CoreType.ctBool))
        
        default_values = daq.List()
        default_values.append(daq.Boolean(True))

        #check typename validation
        type = daq.StructType(daq.String('invalid struct name'), valid_names, default_values, typeList)
        with self.assertRaisesRegex(RuntimeError, 'Validate failed'):
            type_manager.add_type(type)

        #check field names validation
        invalid_names = daq.List()
        invalid_names.append('1_invalid')

        with self.assertRaisesRegex(RuntimeError, '.*names.*incorrect.*'):
            type = daq.StructType(daq.String('valid_struct_name'), invalid_names, default_values, typeList)
            
        invalid_names_1 = daq.List()
        invalid_names_1.append('')

        with self.assertRaisesRegex(RuntimeError, '.*names.*incorrect.*'):
            type = daq.StructType(daq.String('valid_struct_name'), invalid_names_1, default_values, typeList)

        #should be ok here
        type = daq.StructType(daq.String('valid_struct_name'), valid_names, default_values, typeList)
        type_manager.add_type(type)

class TestEnumerations(opendaq_test.TestCase):

    def test_enumeration(self):

        type_manager = daq.TypeManager()

        color_list = daq.List()
        color_list.append(daq.String('RED'))
        color_list.append(daq.String('GREEN'))
        color_list.append(daq.String('BLUE'))

        enum_type = daq.EnumerationType(daq.String('Color'), color_list, 0)

        #unregistered type has no type manager inside
        with self.assertRaises(AttributeError):
            e = enum_type.RED
        type_manager.add_type(enum_type)

        with self.assertRaises(AttributeError):
            e = enum_type.WHITE
        with self.assertRaises(KeyError):
            e = enum_type['WHITE']

        enum_type_1 = type_manager.get_type('Color')

        self.assertEqual(enum_type.RED, enum_type_1.RED)
        self.assertNotEqual(enum_type.RED, enum_type.BLUE)
        self.assertEqual(enum_type.RED.value, 0)
        self.assertEqual(enum_type(0), enum_type.RED)
        self.assertEqual(enum_type(0).name, 'RED')
        self.assertEqual(enum_type['RED'], enum_type.RED)
        self.assertEqual(enum_type['RED'].value, 0)
        self.assertEqual(enum_type['RED'].name, 'RED')
        unknown = int(enum_type.RED) + 10
        self.assertEqual(unknown, 10)

if __name__ == '__main__':
    unittest.main()
