##
# Function blocks are components that allow can process, generate, or consume signal data. They are bundled in
# openDAQ modules (plugins) where each module can give users access to 0 or more function blocks.
#
# A function block can have the following child component types: Input Ports, Signals, Function Blocks.
# A function block that's a child of another is called a "nested function block". These can be added/removed
# in a manner similar to how this would be done with devices. This example showcases how a trigger function
# block is added and removed from the statistics function block.
##

import opendaq as daq
import sys
sys.path.append("..")
import daq_utils

if __name__ == "__main__":
    instance = daq.Instance()

    # Add the Trigger function block
    print('Adding statistics function block')
    statistics_fb = instance.add_function_block('RefFBModuleStatistics')

    # Add nested function blocks (Trigger)
    available_nested_blocks = statistics_fb.available_function_block_types
    nested_fbs = []
    for fb_type in available_nested_blocks.values():
        daq_utils.add_and_append_fb(statistics_fb, fb_type.Id, nested_fbs)

    # Print names of added nested function blocks and remove them
    print('\nAdded nested blocks')
    for fb in nested_fbs:
        daq_utils.print_indented('- ' + fb.name, 1)
        statistics_fb.remove_function_block(fb)

