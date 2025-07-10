##
# openDAQ component statuses represent the current state of any component. They are set up by the module that
# implements each component and aim to provide information to the user of the component state and instructions
# on how to resolve any misconfigurations.
#
# This example uses teh scaling function block's statuses as an example to showcase the system.
##

import opendaq as daq
import sys
sys.path.append("..")
import daq_utils
import time

def print_statuses(component):
    status_container = component.status_container
    print('Statuses: ')
    for status in status_container.statuses.values():
        enum_type = status.enumeration_type
        daq_utils.print_indented(enum_type.name + ' : ' + status.name, 1)

if __name__ == "__main__":
    instance = daq.Instance()
    simulator = daq_utils.setup_simulator()
    device = daq_utils.add_simulator(instance)

    # Add scaling function block and print initial statuses
    scaling = instance.add_function_block('RefFBModuleScaling')
    print_statuses(scaling)

    # Connect invalid signal (simulator time signal)
    time_sig = device.get_signals()[0]
    print ('Connecting signal', time_sig.name, 'to scaling input port...')
    scaling.input_ports[0].connect(time_sig)

    time.sleep(0.5)
    print_statuses(scaling)

    # Connect valid signal (simulator sine wave signal)
    sig = device.signals_recursive[0]
    print ('Connecting signal', sig.name, 'to scaling input port...')
    scaling.input_ports[0].connect(sig)

    time.sleep(0.5)
    print_statuses(scaling)

