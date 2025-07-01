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
