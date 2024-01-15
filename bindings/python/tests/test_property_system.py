#!/usr/bin/env python

import opendaq_test
import opendaq
import unittest
from fractions import Fraction


class TestPropertySystem(opendaq_test.TestCase):

    def test_simple_example(self):
        property_object = opendaq.PropertyObject()
        property_object.add_property(opendaq.StringProperty(opendaq.String(
            'property1'), opendaq.String('value1'), opendaq.Boolean(True)))
        property_object.add_property(opendaq.IntProperty(opendaq.String(
            'property2'), opendaq.Integer(2), opendaq.Boolean(True)))

        self.assertEqual(
            property_object.get_property_value('property1'), 'value1')
        self.assertEqual(property_object.get_property_value('property2'), 2)

        property_object.set_property_value('property1', 'value')
        property_object.set_property_value('property2', 3)
             
        self.assertEqual(
            property_object.get_property_value('property1'), 'value')
        self.assertEqual(property_object.get_property_value('property2'), 3)

    def test_numerical_props(self):
        property_object = opendaq.PropertyObject()
        int_property_builder = opendaq.IntPropertyBuilder(opendaq.String(
            'property1'), opendaq.Integer(10))
        int_property_builder.visible = opendaq.Boolean(True)

        # FIXME: not accepting types
        # int_property_builder.min_value = opendaq.Integer(0)
        # int_property_builder.max_value = opendaq.Integer(100)
        # int_property_builder.min_value = 0
        # int_property_builder.max_value = 100

        property_object.add_property(int_property_builder.build())

        float_property_builder = opendaq.FloatPropertyBuilder(opendaq.String(
            'property2'), opendaq.Float(10.0))
        float_property_builder.visible = opendaq.Boolean(True)
        suggested_values = opendaq.List()
        suggested_values.pushBack(1.0)
        suggested_values.pushBack(2.0)
        suggested_values.pushBack(3.0)
        float_property_builder.suggested_values = suggested_values
        property_object.add_property(float_property_builder.build())

        property_object.set_property_value('property1', 101)
        property_object.set_property_value('property2', 4.0)

        # Will fail because of the min/max values are not set
        # self.assertEqual(property_object.get_property_value('property1'), 100)
        self.assertEqual(property_object.get_property_value('property2'), 4.0)

    def test_selection_props(self):
        property_object = opendaq.PropertyObject()

        list = opendaq.List()
        list.pushBack('value1')
        list.pushBack('value2')
        list.pushBack('value3')

        selection_property = opendaq.SelectionProperty(opendaq.String(
            'property1'), list, opendaq.Integer(1), opendaq.Boolean(True))
        property_object.add_property(selection_property)

        dict = opendaq.Dict()
        dict[0] = 'value1'
        dict[10] = 'value2'
        property_object.add_property(opendaq.SparseSelectionProperty(
            opendaq.String('property2'), dict, opendaq.Integer(0), opendaq.Boolean(True)))

        self.assertEqual(property_object.get_property_value('property1'), 1)
        self.assertEqual(
            property_object.get_property_selection_value('property1'), 'value2')
        self.assertEqual(
            property_object.get_property_selection_value('property2'), 'value1')

        property_object.set_property_value('property1', 2)
        property_object.set_property_value('property2', 10)

        self.assertEqual(property_object.get_property_value('property1'), 2)
        self.assertEqual(
            property_object.get_property_selection_value('property1'), 'value3')
        self.assertEqual(
            property_object.get_property_selection_value('property2'), 'value2')

    def test_object_props(self):
        property_object = opendaq.PropertyObject()
        child1 = opendaq.PropertyObject()
        child2 = opendaq.PropertyObject()
        
        child2.add_property(opendaq.IntProperty(opendaq.String(
            'property1'), opendaq.Integer(10), opendaq.Boolean(True)))
        child1.add_property(opendaq.ObjectProperty(
            opendaq.String('child'), child2))
        property_object.add_property(
            opendaq.ObjectProperty(opendaq.String('child'), child1))
        
        self.assertEqual(property_object.get_property_value(
            'child.child.property1'), 10)

    def test_container_props(self):
        property_object = opendaq.PropertyObject()

        list = opendaq.List()
        list.pushBack('Banana')
        list.pushBack('Apple')
        list.pushBack('Kiwi')
        property_object.add_property(opendaq.ListProperty(
            opendaq.String('list'), list, opendaq.Boolean(True)))

        dict = opendaq.Dict()
        dict['Banana'] = 1
        dict['Apple'] = 2
        property_object.add_property(opendaq.DictProperty(
            opendaq.String('dict'), dict, opendaq.Boolean(True)))

        self.assertEqual(
            property_object.get_property_value('list')[0], 'Banana')
        self.assertEqual(
            property_object.get_property_value('dict')['Apple'], 2)

    def test_reference_props(self):
        property_object = opendaq.PropertyObject()
        property_object.add_property(opendaq.IntProperty(opendaq.String(
            'property1'), opendaq.Integer(0), opendaq.Boolean(True)))
        property_object.add_property(opendaq.StringProperty(opendaq.String(
            'property2'), opendaq.String('value2'), opendaq.Boolean(True)))
        property_object.add_property(opendaq.StringProperty(opendaq.String(
            'property3'), opendaq.String('value3'), opendaq.Boolean(True)))

        property_object.add_property(opendaq.ReferenceProperty(opendaq.String('reference'), opendaq.EvalValue(
            opendaq.String('switch($property1, 0, %property2, 1, %property3)'))))

        self.assertEqual(
            property_object.get_property_value('reference'), 'value2')

        property_object.set_property_value('property1', 1)

        self.assertEqual(
            property_object.get_property_value('reference'), 'value3')

    # TODO: define if implemented
    # def test_function_props(self):
    #     pass

    def test_other_props(self):
        property_object = opendaq.PropertyObject()
        property_object.add_property(opendaq.BoolProperty(opendaq.String(
            'property1'), opendaq.Boolean(True), opendaq.Boolean(True)))
        property_object.add_property(opendaq.StringProperty(opendaq.String(
            'property2'), opendaq.String('value2'), opendaq.Boolean(True)))
        property_object.add_property(opendaq.RatioProperty(opendaq.String(
            'property3'), opendaq.Ratio(1, 2), opendaq.Boolean(True)))

        self.assertEqual(property_object.get_property_value('property1'), True)
        self.assertEqual(
            property_object.get_property_value('property2'), 'value2')
        self.assertEqual(property_object.get_property_value(
            'property3'), Fraction(1, 2))

    def test_create_configure_prop(self):
        property_object = opendaq.PropertyObject()

        float_property_builder = opendaq.FloatPropertyBuilder(opendaq.String(
            'property1'), opendaq.Float(10.0))
        float_property_builder.visible = opendaq.Boolean(True)

        # FIXME: not accepting types
        # float_property_builder.min_value = opendaq.Float(0.0)
        # float_property_builder.max_value = 100.0

        property_object.add_property(float_property_builder.build())
        self.assertIsNotNone(property_object.get_property('property1'))

    def test_simulated_channel_prop(self):

        simulated_channel = opendaq.PropertyObject()
        waveform_list = opendaq.List()
        waveform_list.pushBack('Sine')
        waveform_list.pushBack('Counter')
        simulated_channel.add_property(opendaq.SelectionProperty(opendaq.String(
            'waveform'), waveform_list, opendaq.Integer(0), opendaq.Boolean(True)))
        simulated_channel.add_property(opendaq.ReferenceProperty(opendaq.String('settings'), opendaq.EvalValue(
            opendaq.String('if($waveform == 0, %sine_settings, %counter_settings)'))))

        frequency_property_builder = opendaq.FloatPropertyBuilder(opendaq.String(
            'frequency'), opendaq.Float(10.0))
        # frequency_property_builder.min_value = opendaq.Float(0.1)
        # frequency_property_builder.max_value = opendaq.Float(1000.0)
        frequency_property_builder.visible = opendaq.Boolean(True)
        frequency_property_builder.unit = opendaq.Unit(1, opendaq.String(
            'Hz'), opendaq.String(''), opendaq.String(''))
        suggested_values = opendaq.List()
        suggested_values.pushBack(0.1)
        suggested_values.pushBack(10.0)
        suggested_values.pushBack(100.0)
        suggested_values.pushBack(1000.0)
        frequency_property_builder.suggested_values = suggested_values
        simulated_channel.add_property(frequency_property_builder.build())

        # Sine settings

        sine_settings = opendaq.PropertyObject()
        unit_list = opendaq.List()
        unit_list.pushBack('V')
        unit_list.pushBack('mV')
        sine_settings.add_property(opendaq.SelectionProperty(opendaq.String(
            'amplitude_unit'), unit_list, opendaq.Integer(0), opendaq.Boolean(True)))

        amplitude_property = opendaq.FloatProperty(opendaq.String(
            'amplitude'), opendaq.Float(1.0), opendaq.Boolean(True))
        # TypeError: EvalValue instead of Unit
        # amplitude_property.unit = opendaq.EvalValue(opendaq.String('Unit(%amplitude_unit:SelectedValue)'))

        sine_settings.add_property(amplitude_property)

        sine_settings.add_property(opendaq.BoolProperty(opendaq.String(
            'enable_scaling'), opendaq.Boolean(False), opendaq.Boolean(True)))

        scaling_property_builder = opendaq.FloatPropertyBuilder(opendaq.String(
            'scaling'), opendaq.Float(1.0))
        scaling_property_builder.visible = opendaq.Boolean(True)
        # TypeError: EvalValue instead of Boolean
        # scaling_property_builder.visible = opendaq.EvalValue(opendaq.String('%enable_scaling'))
        # simulated_channel.add_property(scaling_property_builder.build())

        simulated_channel.add_property(opendaq.ObjectProperty(
            opendaq.String('sine_settings'), sine_settings))

        # Counter settings

        counter_settings = opendaq.PropertyObject()
        counter_settings.add_property(opendaq.IntProperty(opendaq.String(
            'increment'), opendaq.Integer(1), opendaq.Boolean(True)))
        mode_list = opendaq.List()
        mode_list.pushBack('Infinite')
        mode_list.pushBack('Loop')
        counter_settings.add_property(opendaq.SelectionProperty(opendaq.String(
            'mode'), mode_list, opendaq.Integer(0), opendaq.Boolean(True)))

        loop_threshold_property_builder = opendaq.IntPropertyBuilder(opendaq.String(
            'loop_threshold'), opendaq.Integer(100))
        loop_threshold_property_builder.visible = opendaq.Boolean(True)
        # loop_threshold_property.min_value = 1
        # loop_threshold_property.visible = opendaq.EvalValue(opendaq.String('%mode == 1'))
        counter_settings.add_property(loop_threshold_property_builder.build())

        # TODO: function property is not finished yet

        simulated_channel.add_property(opendaq.ObjectProperty(
            opendaq.String('counter_settings'), counter_settings))

    def test_validation_coercion(self):
        property_object = opendaq.PropertyObject()
        coerced_property_builder = opendaq.IntPropertyBuilder(opendaq.String(
            'coerced'), opendaq.Integer(5))
        coerced_property_builder.visible = opendaq.Boolean(True)
        coerced_property_builder.coercer = opendaq.Coercer(
            opendaq.String('if(Value < 10, Value, 10)'))
        property_object.add_property(coerced_property_builder.build())

        validated_property_builder = opendaq.IntPropertyBuilder(opendaq.String(
            'validated'), opendaq.Integer(5))
        validated_property_builder.visible = opendaq.Boolean(True)
        validated_property_builder.validator = opendaq.Validator(
            opendaq.String('Value < 10'))
        property_object.add_property(validated_property_builder.build())

        property_object.set_property_value('coerced', 11)
        self.assertEqual(property_object.get_property_value('coerced'), 10)

        self.assertRaises(
            BaseException, property_object.set_property_value, 'validated', 11)

    def test_remove_prop(self):
        property_object = opendaq.PropertyObject()
        property_object.add_property(opendaq.StringProperty(opendaq.String(
            'property1'), opendaq.String('value1'), opendaq.Boolean(True)))
        property_object.remove_property('property1')

    def test_listing_props(self):
        property_object = opendaq.PropertyObject()
        property_object.add_property(opendaq.StringProperty(opendaq.String(
            'string'), opendaq.String('value'), opendaq.Boolean(True)))
        property_object.add_property(opendaq.IntProperty(
            opendaq.String('int'), opendaq.Integer(1), opendaq.Boolean(False)))
        property_object.add_property(opendaq.FloatProperty(
            opendaq.String('float'), opendaq.Float(1.0), opendaq.Boolean(True)))
        property_object.add_property(opendaq.ReferenceProperty(
            opendaq.String('float_ref'), opendaq.EvalValue(opendaq.String('%float'))))

        self.assertEqual(len(property_object.all_properties), 4)
        self.assertEqual(len(property_object.visible_properties), 2)

    # TODO: Property order is not supported yet

    def test_read_write_props(self):
        property_object = opendaq.PropertyObject()

        property_object.add_property(opendaq.StringProperty(opendaq.String(
            'property1'), opendaq.String('value1'), opendaq.Boolean(True)))
        self.assertEqual(
            property_object.get_property_value('property1'), 'value1')

        property_object.set_property_value('property1', 'value2')
        self.assertEqual(
            property_object.get_property_value('property1'), 'value2')

    def test_nested_objects(self):
        property_object = opendaq.PropertyObject()
        child1 = opendaq.PropertyObject()
        child2 = opendaq.PropertyObject()
        
        child2.add_property(opendaq.StringProperty(opendaq.String(
            'property'), opendaq.String('value'), opendaq.Boolean(True)))
        child1.add_property(opendaq.ObjectProperty(
            opendaq.String('child'), child2))
        property_object.add_property(
            opendaq.ObjectProperty(opendaq.String('child'), child1))

        property_object.set_property_value('child.child.property', 'val')
        self.assertEqual(property_object.get_property_value(
            'child.child.property'), 'val')

    def test_selection_property_read(self):
        property_object = opendaq.PropertyObject()
        list = opendaq.List()
        list.pushBack('value1')
        list.pushBack('value2')
        property_object.add_property(opendaq.SelectionProperty(opendaq.String(
            'property'), list, opendaq.Integer(1), opendaq.Boolean(True)))

        self.assertEqual(
            property_object.get_property_selection_value('property'), 'value2')

    def test_list_property_read(self):
        property_object = opendaq.PropertyObject()
        list = opendaq.List()
        list.pushBack('value1')
        list.pushBack('value2')
        property_object.add_property(opendaq.ListProperty(
            opendaq.String('property'), list, opendaq.Boolean(True)))

        self.assertEqual(property_object.get_property_value(
            'property[0]'), 'value1')

    # TODO: events not supported yet

    def test_class(self):
        property_class_builder = opendaq.PropertyObjectClassBuilder(
            opendaq.String('Class'))
        property_class_builder.add_property(opendaq.IntProperty(opendaq.String(
            'property1'), opendaq.Integer(1), opendaq.Boolean(True)))

        list = opendaq.List()
        list.pushBack('value1')
        list.pushBack('value2')

        property_class_builder.add_property(opendaq.SelectionProperty(opendaq.String(
            'property2'), list, opendaq.Integer(1), opendaq.Boolean(True)))

        property_class = property_class_builder.build()

    def test_class_manager(self):
        type_manager = opendaq.TypeManager()

        property_class_builder = opendaq.PropertyObjectClassBuilder(
            opendaq.String('Class'))
        property_class_builder.add_property(opendaq.IntProperty(opendaq.String(
            'property1'), opendaq.Integer(1), opendaq.Boolean(True)))
        list = opendaq.List()
        list.pushBack('value1')
        list.pushBack('value2')
        property_class_builder.add_property(opendaq.SelectionProperty(opendaq.String(
            'property2'), list, opendaq.Integer(1), opendaq.Boolean(True)))
        property_class = property_class_builder.build()

        type_manager.add_type(property_class)

        property_object = opendaq.PropertyObjectWithClassAndManager(
            type_manager, opendaq.String('Class'))
        self.assertEqual(property_object.get_property_value('property1'), 1)

    def test_inheritance(self):
        type_manager = opendaq.TypeManager()

        property_class_builder = opendaq.PropertyObjectClassBuilderWithManager(
            type_manager, opendaq.String('Class'))
        property_class_builder.add_property(opendaq.IntProperty(opendaq.String(
            'inh_prop'), opendaq.Integer(1), opendaq.Boolean(True)))
        property_class = property_class_builder.build()
        type_manager.add_type(property_class)

        property_class_builder2 = opendaq.PropertyObjectClassBuilderWithManager(
            type_manager, opendaq.String('Class2'))
        property_class_builder2.add_property(opendaq.IntProperty(
            opendaq.String('prop'), opendaq.Integer(2), opendaq.Boolean(True)))
        property_class_builder2.parent_name = 'Class'
        property_class2 = property_class_builder2.build()
        type_manager.add_type(property_class2)

        property_object = opendaq.PropertyObjectWithClassAndManager(
            type_manager, opendaq.String('Class2'))

        self.assertEqual(property_object.get_property_value('prop'), 2)
        self.assertEqual(property_object.get_property_value('inh_prop'), 1)


if __name__ == '__main__':
    unittest.main()
