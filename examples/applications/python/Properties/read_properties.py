##
# @tags: properties, fundamentals, howto
# @title: How to read all properties
##
# Every component in openDAQ exposes a list of visible properties that
# describe its configuration. Each property has a name, a value, a type,
# and flags like read_only that tell you whether it can be changed.
#
# This example iterates over all visible properties on a component and
# prints their names, values, and read-only status.
#
# Requires a running simulator.
##

import sys
sys.path.append("..")
import opendaq as daq
import Utils.daq_utils as daq_utils

if __name__ == "__main__":
    try:
        simulator = daq_utils.setup_simulator()

        # We use a statistics function block as the target. The same
        # pattern works on any component: devices, channels, signals.
        fb = simulator.add_function_block("RefFBModuleStatistics")
        print(f"Properties on: {fb.name}\n")

        # visible_properties returns only the properties intended for
        # users. Internal or hidden properties are filtered out. Each
        # property object carries metadata (name, value_type, read_only,
        # description, unit, min/max) alongside the current value.
        for prop in fb.visible_properties:
            value = fb.get_property_value(prop.name)
            ro = " (read-only)" if prop.read_only else ""
            print(f"  {prop.name}: {value}{ro}")

    except Exception as e:
        print(f"Example failed: {e}", file=sys.stderr)
        print("exit 1")
        sys.exit(1)

    print("exit 0")
    sys.exit(0)