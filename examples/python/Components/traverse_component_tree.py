##
# openDAQ features a tree structure of components. The tree always starts with a device at its root.
# Each object is either a folder (has children components) or a component (can not have child components -
# is a leaf node of the tree). This example showcases how we can traverse the openDAQ component tree.
#
# Each folder type in openDAQ follows a set of modelling rules enforced by default subfolders present below
# every component. In example, a Function block, by default, has 3 folders below it at all times: "FB", "IP",
# "Sig". The "FB" folder can only have other function blocks, "IP" input ports, and "Sig" signals.
#
# The other modelling rules are as follows (enforced by the default-created folders):
# Device (Dev) folders: "Sig", "Dev", "FB", "IO"
# Function Block (FB) folder: "Sig", "FB", "IP"
# Inputs-Outputs (IO) folder: "IO", "Ch"
# Channel (Ch) folders: "Sig", "FB", "IP"
#
# Signals and Input Ports are leaf components.
##

import opendaq as daq
import sys
sys.path.append("..")
import daq_utils

def traverse(component, depth = 0):
    print_str = '- ' + component.local_id + ' : ' + daq_utils.get_component_id_str(component)
    daq_utils.print_indented(print_str, depth)

    # Folders can have child components. We check whether a component is a folder
    # and recursively traverse its children.
    if daq.IFolder.can_cast_from(component):
        folder = daq.IFolder.cast_from(component)
        for item in folder.items:
            traverse(item, depth + 1)

if __name__ == "__main__":
    instance = daq.Instance()

    # Setup and connect to simulator
    simulator = daq_utils.setup_simulator()
    daq_utils.add_simulator(instance)

    traverse(instance.root_device)
