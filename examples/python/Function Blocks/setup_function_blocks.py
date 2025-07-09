##
# To use an added function block, users must configure it to match their desired use case. Generally,
# this entails:
#  - Adding any nested FBs when applicable
#  - Connecting signals to the FBs input ports
#  - Connecting the FBs output signals to other input ports (or readers)
#  - Modifying the function block's properties
#
# This example uses the reference function blocks Renderer and Scaling to showcase how signals are
# connected and properties of function blocks modified.
##

# TODO: use `ip.requiresSignal` and `ip.accepts_signal`

import opendaq as daq
import sys
sys.path.append("..")
import daq_utils
import time

if __name__ == "__main__":
    instance = daq.Instance()

    # Setup simulator and store its 1st signal for usage in function blocks
    simulator = daq_utils.setup_simulator()
    device = daq_utils.add_simulator(instance)
    simulator_signal = simulator.signals_recursive[0]

    # Add the Scaling and Renderer function blocks
    renderer_fb = instance.add_function_block('RefFBModuleRenderer')
    scaling_fb = instance.add_function_block('RefFBModuleScaling')

    # Connect the simulator signal to both function blocks
    # We also connect the scaling output to the renderer's 2nd input port
    # The renderer visualizes both connected signals
    print('Connecting simulator signal to scaling input port...')
    scaling_fb.input_ports[0].connect(simulator_signal)

    # The renderer is set up as such that whenever a signal is connected
    # to one of its input ports it opens up another
    print('Connecting simulator signal and scaling signal to renderer input ports 0 and 1...')
    renderer_fb.input_ports[0].connect(simulator_signal)
    renderer_fb.input_ports[1].connect(scaling_fb.signals[0])

    print('Sleeping for 5 seconds...')
    time.sleep(5)

    # Adjusting the "scale" property to 10 scales the input signal by a factor of 10
    # The change in signal can be observed in the renderer
    print('Adjusting the \"scale\" property of the scaling function block to 10...')
    scaling_fb.set_property_value('scale', 10)

    print('Sleeping for 5 seconds...')
    time.sleep(5)