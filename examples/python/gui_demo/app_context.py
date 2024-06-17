import opendaq as daq

from .utils import *


class DeviceInfoLocal:
    def __init__(self, conn_string):
        self.name = conn_string
        self.connection_string = conn_string
        self.serial_number = 'no-serial-number'


class AppContext(object):
    def __init__(self):

        # logic
        self.nodes = {}
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

    def add_device(self, device_info, parent_device: daq.IDevice):
        if device_info is None:
            return None
        if parent_device is None:
            return None
        try:
            device = parent_device.add_device(device_info.connection_string)
            if device:
                device_info.name = device.local_id
                device_info.serial_number = device.info.serial_number
                self.enabled_devices[device_info.connection_string] = {
                    'device_info': device_info, 'device': device}
                return device
        except RuntimeError as e:
            print(f'Error adding device {device_info.connection_string}: {e}')
        return None

    def remove_device(self, device):
        if device is None:
            return
        parent_device = get_nearest_device(device.parent)
        if parent_device is None:
            return

        subdevices = list_all_subdevices(device)
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
        for file in get_files_in_directory(directory):
            image = load_and_resize_image(os.path.join(directory, file))
            images[file.split('.')[0]] = image
        self.icons = images

    def short_id(self, global_id: str):
        parts = global_id.split('/')
        n_parts = len(parts)
        devices = ['']
        if n_parts > 3: # empty + device + Sig + ID
            if parts[-2] == 'Sig':
                signal = parts[-1]
                if signal:
                    for index, part in enumerate(parts[:-2]):
                        if part == 'Dev' and index + 1 < n_parts:
                            devices.append(parts[index + 1])
                    return '/'.join(devices) + '/' + signal
        return ''

    def update_signals_for_device(self, device):
        if device is None:
            return
        self.signals[device.global_id] = {}
        for signal in device.signals_recursive:
            self.signals[device.global_id][self.short_id(
                signal.global_id)] = signal
            
    def signals_for_device(self, device):
        if device is None:
            return {}
        return self.signals.get(device.global_id, {})
