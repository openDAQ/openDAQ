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

    # How many instances of a nested function block type its parent will take.
    # A block decides this in its C++ impl and the SDK does not expose it, so
    # without an entry here the only way to find the limit is to be refused -
    # and each refusal also leaves the parent sitting in Error status. A type
    # listed here stops being offered once the limit is reached instead.
    # Absent means unlimited, which stays the assumption until a refusal is
    # recorded in nested_fb_full.
    nested_fb_limits = {
        # Statistics takes exactly one: "Only one nested function block is
        # supported".
        'RefFBModuleTrigger': 1,
    }

    def __init__(self, params):

        # logic
        self.nodes = {}
        self.custom_component_ids = set()
        self.selected_node = None
        self.include_reference_devices = True
        self.view_hidden_components = False
        self.view_signal_preview = True
        # global ids whose nested-function-block placeholder rows the user hid.
        # The rows are useful while configuring a block and clutter once it is
        # done, and which blocks are done is per block, not global - so this is
        # a per-row opt-out rather than one switch over the whole tree.
        self.nested_fb_hidden = set()
        # master switch for the nested-function-block placeholder rows and the
        # per-row button that hides them. The per-row opt-out above is for a
        # block that is finished; this is for a user who never wants them.
        self.view_nested_fb = True
        # cleared by ticking "do not ask again" on the removal dialog. Session
        # scoped, like every other view preference here.
        self.confirm_component_removal = True
        # (parent global id, fb type id) the block refused another instance of.
        # How many nested blocks a type accepts is decided in its C++ impl and
        # is not exposed, so it can only be learned by being told no once. The
        # answer is only true for as long as the block stays as it was, so it is
        # forgotten again whenever that changes - see forget_nested_fb_refusals.
        self.nested_fb_full = set()
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

        # global ids the user ran "Begin update" on. Tracked here because the
        # SDK's own `updating` flag never goes true on remote components, so it
        # cannot be used to show that the action was taken.
        self.updating_nodes = set()
        # (component global id, property name) written while inside an update
        # block. Inside one the SDK still reports the old value and fires no
        # event, so nothing else can tell which rows have a queued change.
        self.pending_properties = set()

        self.instance = None
        self.connection_string = ''
        self.signals = {}
        self.needs_refresh = False

    # A log file belongs to the sink that opened it, and the old sink only goes
    # away when the old instance is collected - so a recreated instance cannot be
    # handed the same path, and anything tailing it would sit on a file nothing
    # writes to any more. Numbering the file is what makes the switch visible to
    # the logs window, which watches the path rather than the sink.
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
            # Default is the obvious choice for "no preference" and is the one
            # value that yields an empty file: it is 7 in LogLevel and Off is 6,
            # so as a threshold it sits above every real severity and discards
            # all of them. Mapped here rather than explained in the logs window,
            # because a sink's level cannot be changed once it is built.
            file_sink.level = (daq.LogLevel.Debug
                               if self.file_log_level == daq.LogLevel.Default
                               else self.file_log_level)
            # LogsWindow._LEVEL_RE parses these three bracketed fields to read a
            # line's severity; keep them in step or its filter silently stops
            # matching anything
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

    # The one place that decides whether a component counts as being in an
    # update block: either the user ran Begin update on it, or the SDK says so.
    # Remote components never report the flag, so the first half carries them.
    def is_in_update(self, component):
        global_id = getattr(component, 'global_id', None)
        if global_id is not None and global_id in self.updating_nodes:
            return True
        try:
            return bool(daq.IPropertyObject.cast_from(component).updating)
        except Exception:
            # cast_from can fail in more ways than RuntimeError; this only
            # decides how something is drawn, so it must never propagate.
            return False

    def clear_pending_properties(self, global_id):
        self.pending_properties = {
            entry for entry in self.pending_properties if entry[0] != global_id}

    # A refusal only means "full right now". Removing a nested block frees the
    # slot that caused it, and the refusal may equally have come from a
    # transient error, so anything that changes the block - or an explicit
    # refresh - drops what was learned and lets the type be offered again.
    # global_id None forgets every parent; otherwise that component and
    # everything under it, since removing a block takes its children with it and
    # a rebuilt block can reuse the same global id.
    def forget_nested_fb_refusals(self, global_id=None):
        if global_id is None:
            self.nested_fb_full = set()
            return
        prefix = global_id + '/'
        self.nested_fb_full = {
            entry for entry in self.nested_fb_full
            if entry[0] != global_id and not entry[0].startswith(prefix)}

    # True when the parent already holds as many of this nested type as the
    # policy above allows, so the placeholder has nothing left to add.
    def nested_fb_at_limit(self, parent, fb_type_id):
        limit = self.nested_fb_limits.get(str(fb_type_id))
        if limit is None:
            return False
        return self.count_nested_fbs(parent, fb_type_id) >= limit

    def count_nested_fbs(self, parent, fb_type_id):
        wanted = str(fb_type_id)
        count = 0
        try:
            children = parent.function_blocks
        except RuntimeError:
            return 0
        for child in children:
            try:
                if str(child.function_block_type.id) == wanted:
                    count += 1
            except RuntimeError:
                # A child that will not name its type cannot be counted against
                # the limit; better to keep offering than to hide the row.
                continue
        return count

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