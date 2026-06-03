##
# @tags: properties, howto, fundamental
# @title: How to change property value
##
# Reads a property, changes it, and confirms the update.
#
# Every component in openDAQ (devices, function blocks, channels, signals)
# carries a set of properties that control its behavior. Properties have
# names, types, and optional constraints like min/max values or read-only
# flags. The get/set pattern shown here is the same regardless of which
# component you're working with.
##

import sys
import opendaq as daq
import Utils.daq_utils as daq_utils

if __name__ == "__main__":
    try:
        simulator = daq_utils.setup_simulator()

        # We need a component with writable properties to demonstrate on.
        # A statistics function block is a good choice because it's always
        # available in the reference modules and has several configurable
        # properties like BlockSize and DomainSignalType.
        fb = simulator.add_function_block("RefFBModuleStatistics")
        print(f"Component: {fb.name}")

        # get_property_value returns the current value by name. The type
        # you get back matches the property's CoreType: int for ctInt,
        # str for ctString, bool for ctBool, etc.
        prop_name = "BlockSize"
        print(f"{prop_name} before: {fb.get_property_value(prop_name)}")

        # set_property_value writes the new value. If the property has
        # constraints (min/max, coercion, read-only), the SDK enforces
        # them and raises an error if the value is rejected.
        fb.set_property_value(prop_name, 20)

        print(f"{prop_name} after:  {fb.get_property_value(prop_name)}")

    except Exception as e:
        print(f"Example failed: {e}", file=sys.stderr)
        print("exit 1")
        sys.exit(1)

    print("exit 0")
    sys.exit(0)