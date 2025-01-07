import opendaq as daq


def on_core_event(sender, args):
    print(f'Core event from {sender} with args name: "{
          args.event_name}" id: {args.event_id}')


instance = daq.Instance()
instance.context.on_core_event + daq.EventHandler(on_core_event)

dev_info = instance.available_devices[0]

print(f'Connecting to device: {dev_info.name}: connection string: {
      dev_info.connection_string}')

# on core event will be called from here
dev = instance.add_device(dev_info.connection_string)

if dev:
    print(f'Connected to device: {dev.name}')
else:
    print(f'Failed to connect to device: {dev_info.name}')
