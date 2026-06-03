##
# @tags: function-blocks, howto, fundamental
# @title: How to add a function block
##
# Function blocks can process, generate, or consume signal data. They're
# bundled in openDAQ modules where each module provides 0 or more types.
#
# This example lists available function block types, adds one to the
# instance, and prints its properties and child components.
#
# No device connection required -- function blocks are available as long
# as the modules providing them are loaded.
##

import sys
import opendaq as daq
import Utils.daq_utils as daq_utils

if __name__ == "__main__":
    try:
        instance = daq.Instance()

        # available_function_block_types returns a dictionary of type ID
        # to IFunctionBlockType. Each type has a name, description, and
        # optionally a default config you can customize before adding.
        types = instance.available_function_block_types
        if not types:
            print("No function block types available", file=sys.stderr)
            print("exit 1")
            sys.exit(1)

        print("Available function block types:")
        ids = []
        for id_, type_ in types.items():
            print("\n" + id_)
            daq_utils.print_struct(type_, 1)
            ids.append(id_)

        # add_function_block takes a type ID and returns the new FB.
        # Some function blocks need specific setup (scheduler, GUI) to
        # work. The statistics FB is a safe general-purpose choice.
        type_id = "RefFBModuleStatistics"
        if type_id not in types:
            type_id = ids[0]

        print(f"\nAdding function block: {type_id}")
        fb = instance.add_function_block(type_id)

        # Every function block is a component with its own properties,
        # input ports, output signals, and optionally nested FBs.
        print("\nFunction block properties and child components:")
        daq_utils.print_component(fb)

    except Exception as e:
        print(f"Example failed: {e}", file=sys.stderr)
        print("exit 1")
        sys.exit(1)

    print("exit 0")
    sys.exit(0)