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
        self.all_devices = {}
        self.connected_devices = {}
        self.connection_string = ''
        self.all_devices[self.instance.global_id] = dict()
        self.connected_devices[self.instance.global_id] = dict()

    def scan_devices(self, parent_device=None):
        print(f'Scanning devices for {parent_device.info.name}')
        parent_device = parent_device if parent_device else self.instance

        def add_device(device_info):
            conn = device_info.connection_string
            # ignore reference devices unless explicitly requested
            if not self.include_reference_devices and 'daqref' in device_info.connection_string:
                return
            # only add device to the list if it isn't there already
            if not conn in self.all_devices[parent_device.global_id]:
                self.all_devices[parent_device.global_id][conn] = {
                    'device_info': device_info, 'enabled': False, 'device': None}

        if daq.IDevice.can_cast_from(parent_device):
            parent_device = daq.IDevice.cast_from(parent_device)
            if parent_device.global_id not in self.all_devices:
                self.all_devices[parent_device.global_id] = dict()

        for device_info in parent_device.available_devices:
            add_device(device_info)

    def add_first_available_device(self):
        device_info = DeviceInfoLocal(self.connection_string)

        device_state = {'device_info': device_info,
                        'enabled': False, 'device': None}

        self.all_devices[self.instance.global_id][device_info.connection_string] = device_state

        try:
            device = self.instance.add_device(
                device_info.connection_string)
            if device:
                device_info.name = device.local_id
                device_info.serial_number = device.info.serial_number
                device_state['device'] = device
                device_state['enabled'] = True

                # multiple keys for the same device's state
                self.all_devices[self.instance.global_id][device_info.connection_string] = device_state
                self.all_devices[self.instance.global_id][device.global_id] = device_state
                self.connected_devices[self.instance.global_id][device_info.connection_string] = device_state
        except RuntimeError as e:
            print(f'Error adding device {device_info.connection_string}: {e}')

    def load_icons(self, directory):
        images = {}
        for file in get_files_in_directory(directory):
            image = load_and_resize_image(os.path.join(directory, file))
            images[file.split('.')[0]] = image
        self.icons = images
