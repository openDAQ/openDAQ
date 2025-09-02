#!/usr/bin/env python3

##
# This script demonstrates the use of structs and enums in the OpenDAQ framework.
##

import opendaq as daq


if __name__ == '__main__':
    type_manager = daq.TypeManager()

    # creating and adding enum type
    enum_values = daq.List()
    enum_values.append(daq.String('Enum_1'))
    enum_values.append(daq.String('Enum_2'))
    enum_type = daq.EnumerationType(daq.String('Enum'), enum_values, 0)
    type_manager.add_type(enum_type)

    # exploring enum values
    # all the values are also available through autocomplete on daq.IEnumerationType
    print(enum_type.enumerator_names)

    # creating and adding struct type
    names = daq.List()
    names.append(daq.String('field1'))
    names.append(daq.String('field2'))

    types = daq.List()
    types.append(daq.SimpleType(daq.CoreType.ctInt))
    types.append(enum_type)

    struct_type = daq.StructTypeNoDefaults(
        daq.String('MyStruct'), names, types)
    type_manager.add_type(struct_type)

    # constructing struct
    struct_builder = daq.StructBuilder(daq.String('MyStruct'), type_manager)
    struct_builder.set('field1', 42)
    struct_builder.set('field2', enum_type.Enum_2)
    struct = struct_builder.build()

    # creating property object, setting and getting struct propperty
    property_object = daq.PropertyObject()
    property_object.add_property(
        daq.StructProperty('struct_property', struct, True))

    default_prop_value: daq.IStruct = property_object.get_property_value(
        'struct_property')
    print('Default struct property value:', default_prop_value)

    # updating and setting struct
    updated_struct_builder = daq.StructBuilderFromStruct(default_prop_value)
    updated_struct_builder.set('field1', 100)
    updated_struct_builder.set('field2', enum_type.Enum_1)

    property_object.set_property_value(
        'struct_property', updated_struct_builder.build())

    # getting updated struct
    updated_struct = property_object.get_property_value('struct_property')
    print('Updated struct property value:', updated_struct)

    # exploring enum values through updated struct
    print(updated_struct.field2.enumeration_type.enumerator_names)
