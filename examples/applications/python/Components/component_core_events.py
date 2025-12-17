##
# Core events are events that are triggered whenever a change to the component structure is done. Ie. whenever
# a component is added/removed, property modified, a component is updated... Each event type has a set of defined
# parameters that can be inspected. Any application can listen and react to said core events.
#
# IMPORTANT: openDAQ configuration servers can be "push" or "pull"-based. "push"-based ones send out information on
# core changes when those happen, whereas "pull"-based ones do not -> as the name implies, the client must request
# information on whether something changed via "getter" methods. As such, push-based protocol (ie. Native) trigger
# core events, whereas pull-based ones (ie. OPC UA) do not.
##

import sys
sys.path.append("..")

import opendaq as daq
import daq_utils
import time

def on_core_event(sender, args):
    core_event_args = daq.ICoreEventArgs.cast_from(args)
    print(f'Core event from {sender} with args name: "{args.event_name}" id: {args.event_id} params: {list(core_event_args.parameters.keys())}')

if __name__ == "__main__":
    simulator = daq_utils.setup_simulator()

    instance = daq.Instance()
    instance.context.on_core_event + daq.EventHandler(on_core_event)

    # Core event will be triggered by the device connection event
    device = daq_utils.add_simulator(instance)

    # Wait for the event
    time.sleep(0.5)

    print(f'Connected to device: {device.name}')