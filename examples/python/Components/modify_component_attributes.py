import opendaq as daq
import sys
sys.path.append("..")
import daq_utils

if __name__ == "__main__":
    # Component attributes:
    # - Name: The name of the component, equal to Local ID by default
    # - Description: The description of the component
    # - Visible: Visibility of component. Invisible components are not returned by
    #            tree traversal methods by default (eg. folder.items)
    # - Active:

    # Signal attributes:
    # - Public: Public signals are available through openDAQ client-server connections.
    # - DomainSignal: The domain signal of a signal
    # - RelatedSignals: A list of related signals

    simulator = daq_utils.setup_simulator()
    device = simulator.root_device
    signal = simulator.signals_recursive[0]

    # Locked attributes can not be modified
    print('Simulator locked attributes:', device.locked_attributes)
    print('Signal locked attributes:', signal.locked_attributes)

    print('\nSignal Visible flag state:', str(signal.visible))
    print('Trying to set signal to Visible==False')
    signal.visible = False
    print('Signal Visible flag state after set:', str(signal.visible))

    ## ADVANCED USAGE!
    # Attributes can be unlocked on local devices (when not connected to an openDAQ server).
    # Use this with caution, locked attributes are by the module vendor and should generally
    # never be unlocked and changed.

    component_private = daq.IComponentPrivate.cast_from(device)
    component_private.unlock_attributes(['Visible'])
    print('\nSimulator locked attributes:', simulator.locked_attributes)

    print('\nSimulator Visible flag state:', str(device.visible))
    print('Trying to set simulator to Visible==False')
    device.visible = False
    print('Simulator Visible flag state after set:', str(device.visible))

    component_private.lock_all_attributes()
    print('All locked simulator attributes:', simulator.locked_attributes)