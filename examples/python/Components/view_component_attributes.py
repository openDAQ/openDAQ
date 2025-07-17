##
# Each component type as a set of attributes that can be configured either by a module,
# an application (end-user), or both. These attributes can be locked or unlocked. A module
# should lock all attributes it does not allow users to configure. An application should
# not attempt to unlock attributes locked by the module unless there's good reason to do so.
#
# This example showcases how we can see the current values of all component attributes.
#
# The current list of attributes is as follows:
# Component attributes:
# - Name: The name of the component, equal to Local ID by default.
# - Description: The description of the component.
# - Visible: Visibility of component. Invisible components are not returned by
#   tree traversal methods by default (eg. folder.items)
# - Active: Inactive components do not perform certain actions. Eg. sending a
#   data packet through an inactive signal will not enqueue the packet. Likewise,
#   an inactive input port will drop non-event packets.
#
# Signal attributes:
# - Public: Public signals are available through openDAQ client-server connections.
# - DomainSignal: The domain signal of a signal
# - RelatedSignals: A list of related signals
##

import opendaq as daq
import sys
sys.path.append("..")
import daq_utils

if __name__ == "__main__":
    simulator = daq_utils.setup_simulator()
    print("Simulator:")
    daq_utils.print_indented("Name: \"" + simulator.name + '\"', 1)
    daq_utils.print_indented("Description: \"" + simulator.description + '\"', 1)
    daq_utils.print_indented("Visible: \"" + str(simulator.visible) + '\"', 1)
    daq_utils.print_indented("Active: \"" + str(simulator.active) + '\"', 1)

    signal = simulator.signals_recursive[0]

    print("\nSignal:")
    daq_utils.print_indented("Name: \"" + signal.name + '\"', 1)
    daq_utils.print_indented("Description: \"" + signal.description + '\"', 1)
    daq_utils.print_indented("Visible: \"" +  str(signal.visible) + '\"', 1)
    daq_utils.print_indented("Active: \"" +  str(signal.active) + '\"', 1)
    daq_utils.print_indented("Public: \"" +  str(signal.public) + '\"', 1)
    daq_utils.print_indented("DomainSignal: \"" +  signal.domain_signal.name + '\"', 1)
    daq_utils.print_indented("RelatedSignals: \"" +  str(signal.related_signals) + '\"', 1)

    invisible_signals = simulator.get_signals(daq.RecursiveSearchFilter(daq.NotSearchFilter(daq.VisibleSearchFilter())))
    signal = daq.ISignal.cast_from(invisible_signals[0])

    print("\nInvisible signal:")
    daq_utils.print_indented("Name: \"" + signal.name + '\"', 1)
    daq_utils.print_indented("Description: \"" + signal.description + '\"', 1)
    daq_utils.print_indented("Visible: \"" +  str(signal.visible) + '\"', 1)
    daq_utils.print_indented("Active: \"" +  str(signal.active) + '\"', 1)
    daq_utils.print_indented("Public: \"" +  str(signal.public) + '\"', 1)
    # The time signal has no domain signal
    daq_utils.print_indented("DomainSignal: \"" + str(signal.domain_signal) + '\"', 1)
    daq_utils.print_indented("RelatedSignals: \"" +  str(signal.related_signals) + '\"', 1)
