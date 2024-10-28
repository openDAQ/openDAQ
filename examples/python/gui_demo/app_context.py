import os

import opendaq as daq

from . import utils


class DeviceInfoLocal:
    def __init__(self, conn_string):
        self.name = conn_string
        self.connection_string = conn_string
        self.serial_number = 'no-serial-number'


class AppContext(object):

    default_folders = {'Dev', 'FB', 'IO', 'IP', 'Sig'}

    def __init__(self):

        # logic
        self.nodes = {}
        self.custom_component_ids = set()
        self.selected_node = None
        self.include_reference_devices = False
        self.view_hidden_components = False
        # gui
        self.ui_scaling_factor = 1.0
        self.icons = {}
        # daq
        self.instance = daq.Instance()
        self.enabled_devices = {}
        self.connection_string = ''
        self.signals = {}

    def register_device(self, device_info):
        conn = device_info.connection_string
        # ignore reference devices unless explicitly requested
        if not self.include_reference_devices and 'daqref' in conn:
            return
        # only add device to the list if it isn't there already
        if not conn in self.enabled_devices:
            self.enabled_devices[conn] = {
                'device_info': device_info, 'device': None}

    def add_device(self, device_info, parent_device: daq.IDevice, config=None):
        if device_info is None:
            return None
        if parent_device is None:
            return None
        try:
            device = parent_device.add_device(
                device_info.connection_string, config)
            if device:
                device_info.name = device.local_id
                device_info.serial_number = device.info.serial_number
                self.enabled_devices[device.info.connection_string] = {
                    'device_info': device_info, 'device': device}
                return device
        except RuntimeError as e:
            print(f'Error adding device {device_info.connection_string}: {e}')
        return None

    def remove_device(self, device):
        if device is None:
            return
        parent_device = utils.get_nearest_device(device.parent)
        if parent_device is None:
            return

        subdevices = utils.list_all_subdevices(device)
        for subdevice in subdevices:
            del self.enabled_devices[subdevice.info.connection_string]

        conn_string = device.info.connection_string
        parent_device.remove_device(device)
        del self.enabled_devices[conn_string]

    def scan_devices(self, parent_device=None):
        parent_device = parent_device or self.instance

        for device_info in parent_device.available_devices:
            self.register_device(device_info)

    def add_first_available_device(self):
        device_info = DeviceInfoLocal(self.connection_string)
        self.add_device(device_info, self.instance)

    def load_icons(self, directory):
        images = {}
        for file in utils.get_files_in_directory(directory):
            image = utils.load_and_resize_image(os.path.join(directory, file))
            images[file.split('.')[0]] = image
        self.icons = images

    def is_server(self, device_id):
        if not device_id:
            return False

        component = self.instance.find_component(device_id)
        if component is None or not daq.IDevice.can_cast_from(component):
            return False

        device = daq.IDevice.cast_from(component)
        return len(device.info.server_capabilities) > 0

    def short_id(self, global_id):
        if not global_id or not isinstance(global_id, str):
            return ''

        # split to '', root_device, etc...
        parts = global_id.split('/')
        n_parts = len(parts)

        # fallback root device
        server_device_index = 2  # skip root device id
        # find the nearest server device pass
        for index, part in reversed(list(enumerate(parts))):
            # found subdevice
            if part == 'Dev' and index + 2 <= n_parts:
                # recreate device id
                device_id = '/'.join(parts[:index + 2])
                # found server device
                if self.is_server(device_id):
                    server_device_index = index + 1
                    break

        # filter realitive to device id
        filtered_parts = []
        for index, part in reversed(
                list(enumerate(parts[server_device_index:]))):
            if part not in ('IO', 'FB', 'Sig', 'Dev'):
                filtered_parts.append(part)

        return '/'.join(reversed(filtered_parts))

    def update_signals_for_device(self, device: daq.IDevice):
        if device is None:
            return

        self.signals[device.global_id] = {}
        for signal in device.get_signals_recursive(daq.AnySearchFilter() if self.view_hidden_components else None):
            short_id = self.short_id(signal.global_id)

            if short_id not in self.signals[device.global_id]:
                self.signals[device.global_id][short_id] = signal
            else:  # handle collision
                # replace short entry to long one
                collided_signal = self.signals[device.global_id][short_id]
                del self.signals[device.global_id][short_id]
                self.signals[device.global_id][collided_signal.global_id] = collided_signal
                # insert second collision entry
                self.signals[device.global_id][signal.global_id] = signal

    def signals_for_device(self, device):
        if device is None:
            return {}
        return self.signals.get(device.global_id, {})

    def properties_of_component(self, component: daq.IComponent):
        if component is None:
            return daq.IList()

        return component.all_properties if self.view_hidden_components else component.visible_properties
