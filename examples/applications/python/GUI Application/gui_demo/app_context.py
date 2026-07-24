import os
import platform
import tempfile

import opendaq as daq

from . import utils
from typing import Callable, Optional


class DeviceInfoLocal:
    def __init__(self, conn_string):
        self.name = conn_string
        self.connection_string = conn_string
        self.serial_number = 'no-serial-number'


class AppContext(object):

    default_folders = {'Dev', 'FB', 'IO', 'IP', 'Sig'}

    # connection-string prefixes of the internal demo/reference devices; these
    # are hidden from the Add device list when include_reference_devices is off
    demo_connection_prefixes = ('daqref://', 'daq.simulator://')

    def __init__(self, params):

        # logic
        self.nodes = {}
        self.custom_component_ids = set()
        self.selected_node = None
        self.include_reference_devices = True
        self.view_hidden_components = False
        self.view_signal_preview = True
        self.metadata_fields = []
        # gui
        self.ui_scaling_factor = 1.0
        self.dpi_factor = self._detect_dpi_factor()
        self.icons = {}
        # daq
        # instance parameters, applied by create_instance() once the
        # configure-instance dialog was confirmed
        self.module_path = params.module_path
        self.discovery_servers = list(getattr(params, 'discovery_servers', []) or [])
        # logger configuration used when the instance is created
        self.log_level = daq.LogLevel.Default
        self.log_to_file = True
        self.file_log_level = daq.LogLevel.Default
        self._log_file_index = 0
        self.log_file_path = os.path.join(
            tempfile.gettempdir(), 'opendaq_gui_{}.log'.format(os.getpid()))

        self.instance = None
        self.connection_string = ''
        self.signals = {}
        self.needs_refresh = False

    # switches to a fresh log file; the previous instance's sink keeps the
    # old file open, so a recreated instance gets its own
    def next_log_file(self):
        self._log_file_index += 1
        self.log_file_path = os.path.join(
            tempfile.gettempdir(), 'opendaq_gui_{}_{}.log'.format(
                os.getpid(), self._log_file_index))

    # True when the connection string belongs to an internal demo/reference
    # device (reference device or simulator)
    def is_demo_device(self, connection_string):
        if not connection_string:
            return False
        conn = str(connection_string)
        return any(conn.startswith(prefix)
                   for prefix in self.demo_connection_prefixes)

    # builds the openDAQ instance from the collected parameters; called after
    # the configure-instance dialog was closed
    def create_instance(self):
        builder = daq.InstanceBuilder()
        builder.scheduler_worker_num = 0
        builder.using_scheduler_main_loop = True

        try:
            daq.OPENDAQ_MODULES_DIR
        except:
            builder.module_path = '.'
        else:
            builder.module_path = daq.OPENDAQ_MODULES_DIR

        if self.module_path:
            builder.add_module_path(self.module_path)

        for protocol in self.discovery_servers:
            builder.add_discovery_server(protocol)

        builder.global_log_level = self.log_level
        # explicit sinks: console output as before, plus a rotating log file
        # the logs window reads from
        builder.add_logger_sink(daq.StdOutLoggerSink())
        if self.log_to_file:
            file_sink = daq.RotatingFileLoggerSink(
                self.log_file_path, 2 * 1024 * 1024, 3)
            file_sink.level = self.file_log_level
            file_sink.pattern = '[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] %v'
            builder.add_logger_sink(file_sink)

        self.instance = daq.InstanceFromBuilder(builder)
        self.instance.context.on_core_event + daq.QueuedEventHandler(self.on_core_event)

    def _detect_dpi_factor(self) -> float:
        """Detect system DPI scaling factor (1.0 = 96 DPI). Used to scale UI elements on high-DPI displays."""
        try:
            if platform.system() == 'Windows':
                from ctypes import windll
                try:
                    # Windows 10 1703+
                    dpi = windll.user32.GetDpiForSystem()
                    if dpi and dpi > 0:
                        return dpi / 96.0
                except Exception:
                    pass
                try:
                    # Fallback: GetDeviceCaps(LOGPIXELSX)
                    hdc = windll.user32.GetDC(0)
                    if hdc:
                        dpi = windll.gdi32.GetDeviceCaps(hdc, 88)  # 88 = LOGPIXELSX
                        windll.user32.ReleaseDC(0, hdc)
                        if dpi and dpi > 0:
                            return dpi / 96.0
                except Exception:
                    pass
        except Exception:
            pass
        return 1.0

    def add_device(self, device_info, parent_device: daq.IDevice, config=None):
        if device_info is None:
            return None
        if parent_device is None:
            return None

        device = parent_device.add_device(
            device_info.connection_string, config)

        return device

    def remove_device(self, device):
        if device is None:
            return
        parent_device = utils.get_nearest_device(device.parent)
        if parent_device is None:
            return
        parent_device.remove_device(device)

    def add_first_available_device(self):
        device_info = DeviceInfoLocal(self.connection_string)
        self.add_device(device_info, self.instance)

    def load_icons(self, directory):
        images = {}
        scale = max(1, int(self.dpi_factor))
        for file in utils.get_files_in_directory(directory):
            # Skip the _x2 variant files — loaded on demand by load_icon()
            if '_x2' in file:
                continue
            image = utils.load_icon(os.path.join(directory, file), scale=scale)
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
            return []
        try:
            return component.all_properties if self.view_hidden_components else component.visible_properties
        except RuntimeError:
            return []

    def on_core_event(self, sender: Optional[daq.IComponent], args: daq.IEventArgs):
        if sender is None or args is None:
            return
        if daq.IDevice.can_cast_from(sender) and args.event_name == "StatusChanged":
            core_event_args: daq.ICoreEventArgs = daq.ICoreEventArgs.cast_from(args)
            if "ConnectionStatus" in core_event_args.parameters.keys():
                self.needs_refresh = True
            return
        if args.event_name in ("ComponentAdded", "ComponentRemoved"):
            self.needs_refresh = True