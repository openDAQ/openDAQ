##
# Function blocks are components that allow can process, generate, or consume signal data. They are bundled in
# openDAQ modules (plugins) where each module can give users access to 0 or more function blocks.
#
# This example prints what function blocks are available within the loaded modules and adds all of them.
##

import opendaq as daq
import sys
sys.path.append("..")
import daq_utils

if __name__ == "__main__":
    instance = daq.Instance()

    # List available function block types and store IDs
    print('Available function block types:')
    ids = []
    types = instance.available_function_block_types
    for id_, type_ in types.items():
        print('\n' + id_)
        daq_utils.print_struct(type_, 1)
        ids.append(id_)

    print('\nAdding found function blocks...')
    fbs = []
    for id_ in ids:
        daq_utils.add_and_append_fb(instance, id_, fbs)

    print('Function block properties and child components:')
    for fb in fbs:
        daq_utils.print_component(fb)
        print()