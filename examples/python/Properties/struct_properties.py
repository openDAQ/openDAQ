##
# Struct properties contain property values of type "Struct". In openDAQ, such properties contain a struct type
# that specifies a set of field names, field value types, and field default values. Each struct of a given struct
# type must have the requisite fields.
#
# Types are registered in the openDAQ type manager, and each struct type must have a unique ID. Struct properties
# in turn must always have values of the same type as the one of the properties default value.
#
# This example showcases how a struct-type property can be read and modified.
##

import sys
sys.path.append("..")
import daq_utils
import opendaq as daq

if __name__ == "__main__":
    # Create a simulator device
    simulator = daq_utils.setup_simulator()

    # Get the "CustomRange" property of device's channel
    ch = simulator.channels[0]
    custom_range = ch.get_property_value('CustomRange')

    # Print all fields (attributes) and their values of the struct
    for field in dir(custom_range):
        print(f'{field}: {getattr(custom_range, field)}')

    # Access the values by name.
    print(f'HighValue: {custom_range.HighValue}')

    # Structs are immutable, so we need to create new struct to replace the previous one
    bldr = daq.StructBuilderFromStruct(custom_range)
    bldr.set('HighValue', 15.0)
    bldr.set('LowValue', -1.0)
    struct = bldr.build()

    # TODO: BUG where we can't modify the actual value of the CustomRange prop
    #       As a workaround, we add a new property to our simulator and modify that one.

    simulator.add_property(daq.StructProperty('CustomRange', custom_range, True))

    # Updating the struct property and read the new values
    simulator.set_property_value('CustomRange', bldr.build())
    custom_range = simulator.get_property_value('CustomRange')

    print(f'Updated LowValue: {custom_range.LowValue}')
    print(f'Updated HighValue: {custom_range.HighValue}')
    daq_utils.print_struct(custom_range)

    # TODO: BUG where we can not access iterate through the field types of the custom range prop.
    #       Fails on `daq_utils.print_struct(custom_range)`.