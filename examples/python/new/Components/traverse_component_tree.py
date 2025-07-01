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
