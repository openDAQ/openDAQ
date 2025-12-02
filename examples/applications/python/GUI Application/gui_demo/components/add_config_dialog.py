import tkinter as tk
from tkinter import ttk

import opendaq as daq

from .. import utils
from ..app_context import AppContext
from .dialog import Dialog
from .checkbox_list import CheckboxList
from .add_device_configuration_view import AddDeviceConfigView

_NO_CONFIG_STR = "-- Streaming only --"
_DEVICE = "Device"
_STREAMING = "Streaming"
_GENERAL = "General"
_ASP = "AllowedStreamingProtocols"
_ACS = "AutomaticallyConnectStreaming"

class AddConfigDialog(Dialog):
    @classmethod
    def for_discoverable(cls, parent, context: AppContext, parent_device, device_info):
        """Populate server capabilities and tcp/ip and streaming from device_info."""
        discoverable = True
        server_capabilities = {}

        # Find TCP/IP and streaming capabilities
        has_tcp_ip = False
        has_streaming = False
        for c in device_info.server_capabilities:
            server_capabilities[c.protocol_id] = c.connection_string
            if c.get_property_value("ConnectionType") == "TCP/IP":
                has_tcp_ip = True
            if c.get_property_value("ProtocolType") in ["Streaming", "ConfigurationAndStreaming"]:
                has_streaming = True

        daq_connection_string = device_info.connection_string
        return cls(
            parent,
            context,
            parent_device,
            discoverable,
            server_capabilities,
            daq_connection_string,
            has_streaming,
            has_tcp_ip)

    @classmethod
    def from_connection_string(cls, parent, context: AppContext, parent_device, connection_string, implied_protocol):
        """Show user sensible settings based on entered connection string with minimal assumptions."""
        discoverable = False
        server_capabilities = {}
        server_capabilities[implied_protocol] = connection_string

        streaming_protocols = []
        module_manager: daq.IModuleManager = parent_device.context.module_manager
        for _module in module_manager.modules:
            for streaming in _module.available_streaming_types:
                streaming_protocols.append(streaming)

        if implied_protocol not in streaming_protocols:
            # When connection string implies non-streaming protocol, the user can select streaming protocols
            for streaming in streaming_protocols:
                server_capabilities[streaming] = ""
        # else: the only server capability is the streaming protocol implied by the connection string

        # We can't know, must assume true
        has_streaming = True
        has_tcp_ip = True
        return cls(
            parent,
            context,
            parent_device,
            discoverable,
            server_capabilities,
            connection_string,
            has_streaming,
            has_tcp_ip)

    def __init__(
            self,
            parent,
            context: AppContext,
            parent_device,
            discoverable,
            server_capabilities,
            connection_string,
            has_streaming,
            has_tcp_ip):
        super().__init__(parent, 'Add with config', context)
        self.context = context
        self.parent_device = parent_device
        # self.device_info = device_info

        self.discoverable = discoverable
        self.server_capabilities = server_capabilities
        self.default_connection_string = connection_string
        self.has_streaming = has_streaming
        self.has_tcp_ip = has_tcp_ip

        self.geometry('{}x{}'.format(
            1200 * self.context.ui_scaling_factor, 600 * self.context.ui_scaling_factor))

        self.protocol('WM_DELETE_WINDOW', self.cancel)

        # MARK: Layout creation
        self.rowconfigure(0, weight=1)
        self.columnconfigure(0, weight=1, uniform='column')
        self.columnconfigure(1, weight=3, uniform='column')

        left_frame = ttk.Frame(self)

        self.device_combobox = self.add_combobox(left_frame, "Configuration protocols:")
        self.device_combobox.bind("<<ComboboxSelected>>", self.handle_device_protocol_select)
        self.streaming_checklist = self.add_checkbox_list(left_frame, "Streaming protocols:")
        self.streaming_checklist.register_callback(self.handle_checklist_changed)

        left_frame.grid(row=0, column=0)
        left_frame.grid_configure(sticky=tk.NSEW)

        right_frame = ttk.Frame(self)

        self.tabs = ttk.Notebook(right_frame)
        self.tabs.pack(fill=tk.BOTH, expand=True)

        bottom_frame = ttk.Frame(right_frame)
        bottom_frame.pack(side=tk.BOTTOM, fill=tk.X)

        self.status_label = ttk.Label(bottom_frame, text="")
        self.status_label.pack(side=tk.LEFT, anchor=tk.W, fill=tk.X, expand=True)

        self.add_device_button = ttk.Button(bottom_frame, text='Add device', command=self.handle_add_button_clicked)
        self.add_device_button.pack(side=tk.RIGHT, anchor=tk.E, padx=10, pady=10, ipadx=20)

        right_frame.grid(row=0, column=1)
        right_frame.grid_configure(sticky=tk.NSEW)

        # Filter and save config, initialize left panel
        self.connection_string = None
        self.config = self.create_add_device_config()
        self.update_selection_widgets()

        # Initialize members related to editor tabs
        self.selected_device_config = self.device_combobox.get()
        self.selected_streaming_configs = []

        self.general_tab = self.create_general_tab()
        self.device_tab = self.create_device_tab()
        self.streaming_tabs = []

        self.update_connection_string()

        self.update_general_tab()
        self.update_device_tab()
        self.update_streaming_tabs()

        # Update streaming tabs and general section as all available streamings are checked
        self.handle_checklist_changed(self.streaming_checklist.get_checked())
        self.tabs.select(self.general_tab)

    def handle_add_button_clicked(self):
        self.sync_device_and_streaming_configs()
        self.destroy()

    def cancel(self):
        self.config = None
        self.destroy()

    def close(self):
        # When escape is pressed adding the device is cancelled
        self.cancel()

    # MARK: Add widgets
    def add_combobox(self, frame, title):
        sub_frame = ttk.Frame(frame)
        sub_frame.pack(fill=tk.BOTH, pady=(0, 5))
        ttk.Label(sub_frame, text=title, font=("Arial", 10, "bold")).pack(side=tk.TOP, fill=tk.X, anchor=tk.W, pady=5)
        var = tk.StringVar()
        cbox = ttk.Combobox(sub_frame, state="readonly", textvariable=var)
        cbox.pack(side=tk.TOP, fill=tk.X, anchor=tk.W, padx=(0, 25))

        return cbox

    def add_checkbox_list(self, frame, title):
        sub_frame = ttk.Frame(frame)
        sub_frame.pack(fill=tk.BOTH, expand=True, pady=(5, 10))
        ttk.Label(sub_frame, text=title, font=("Arial", 10, "bold")).pack(side=tk.TOP, fill=tk.X, anchor=tk.W)

        checkbox_list = CheckboxList(sub_frame, items=None)
        checkbox_list.pack(fill=tk.BOTH, expand=True)

        return checkbox_list

    # MARK: Handle widget events
    def handle_device_protocol_select(self, event=None):
        value = self.device_combobox.get()
        if not value or len(value) == 0:
            self.selected_device_config = None
            return

        self.selected_device_config = value

        self.update_connection_string()
        self.update_device_tab()

    def handle_checklist_changed(self, checked_labels: list):
        self.selected_streaming_configs = checked_labels
        self.update_connection_string()
        self.update_streaming_tabs()

        general_section = self.config.get_property_value(_GENERAL)
        if general_section.has_property(_ASP):
            allowed_protocols = general_section.get_property(_ASP)
            allowed = daq.List()
            for label in self.selected_streaming_configs:
                allowed.append(utils.value_to_coretype(
                    label, allowed_protocols.item_type))
            allowed_protocols.value = allowed
            self.general_tab.editor.refresh()

    # MARK: Update notebook tabs
    def create_device_tab(self):
        device_tab = AddDeviceConfigView(self.tabs)
        device_tab.pack(fill=tk.BOTH, expand=True)

        self.tabs.add(device_tab, text=_DEVICE)
        self.tabs.insert(1, device_tab)
        self.tabs.hide(device_tab)
        return device_tab

    def update_device_tab(self):
        if len(self.selected_device_config) == 0 or self.selected_device_config == _NO_CONFIG_STR:
            if self.device_tab:
                self.device_tab.clear()
                self.tabs.hide(self.device_tab)
            return

        tab_title = self.selected_device_config.replace("OpenDAQ", "").replace("Configuration", " Configuration")
        self.tabs.tab(self.device_tab, text=tab_title)

        device_section = self.config.get_property_value(_DEVICE)
        self.device_tab.edit(device_section.get_property(self.selected_device_config), self.context)
        self.tabs.add(self.device_tab) # unhide

    def update_streaming_tabs(self):
        for tab in self.streaming_tabs:
            self.tabs.forget(tab)
            tab.destroy()

        streaming_section = self.config.get_property_value(_STREAMING)

        self.streaming_tabs = []
        for property_name in self.selected_streaming_configs:
            new_tab = AddDeviceConfigView(self)
            new_tab.edit(streaming_section.get_property(property_name), self.context)

            tab_title = property_name.replace("OpenDAQ", "").replace("Streaming", " Streaming")
            self.tabs.add(new_tab, text=tab_title)
            self.tabs.insert(tk.END, new_tab)
            self.streaming_tabs.append(new_tab)

    def create_general_tab(self):
        general_tab = AddDeviceConfigView(self.tabs)
        general_tab.pack(fill=tk.BOTH, expand=True)

        self.tabs.add(general_tab, text=_GENERAL)
        self.tabs.insert(0, general_tab)
        self.tabs.hide(general_tab)
        return general_tab

    def update_general_tab(self):
        self.general_tab.edit(self.config.get_property(_GENERAL), self.context, self.hidden_general_entries)
        self.tabs.add(self.general_tab)

    def update_selection_widgets(self):
        self.update_combobox()
        self.update_checklist(self.config.get_property_value(_STREAMING), True)

    def update_combobox(self):
        # Lookup streaming options
        streaming_section = self.config.get_property_value(_STREAMING)
        streaming_options = []
        for property in self.context.properties_of_component(streaming_section):
            streaming_options.append(property.name)

        device_section = self.config.get_property_value(_DEVICE)
        device_options = []

        for property in self.context.properties_of_component(device_section):
            # Avoid duplication in device combobox and streaming checklist
            if property.name not in streaming_options:
                device_options.append(property.name)

        # -- Streaming only -- option when streaming is available and for streaming connections to non-discoverable devices
        if len(streaming_options) > 0 and (self.discoverable or len(device_options) == 0):
            device_options.insert(0, _NO_CONFIG_STR)

        self.device_combobox["values"] = device_options

        default_idx = min(1, len(device_options) - 1)
        self.device_combobox.set(device_options[default_idx])

    def update_checklist(self, streaming_section, select_all):
        for property in self.context.properties_of_component(streaming_section):
            self.streaming_checklist.insert(property.name, select_all)

    def update_connection_string(self):
        """Update connection string used to add device. This function also
        performs a quick validation of the UI state and sets the status message."""

        self.add_device_button.config(state="!disabled")
        # A configuration protocol is selected
        if self.selected_device_config != _NO_CONFIG_STR:
            if len(self.server_capabilities) == 0:
                self.connection_string = self.default_connection_string
                self.status_label.configure(
                    text=f"[OK] Connecting to local device: {self.connection_string}", style="StatusOk.TLabel")
                return

            # Remote devices have protocol IDs defined in server capabilities
            for protocol_id, conn_str in self.server_capabilities.items():
                if protocol_id == self.selected_device_config:
                    self.connection_string = conn_str
                    self.status_label.configure(
                        text=f"[OK] Configuration connection to: {self.connection_string}", style="StatusOk.TLabel")
                    return

        # No config and 0 or >= 2 streaming connections selected, unclear how to connect
        if len(self.selected_streaming_configs) != 1:
            self.connection_string = None
            self.add_device_button.config(state="disabled")
            self.status_label.configure(
                text="[ERROR] Select a configuration protocol or exactly one streaming protocol.",
                style="StatusError.TLabel")
            return

        streaming_protocol = self.selected_streaming_configs[0]
        for protocol_id, conn_str in self.server_capabilities.items():
            if protocol_id == streaming_protocol:
                self.connection_string = conn_str
                self.status_label.configure(
                    text=f"[OK] Streaming connection to: {self.connection_string}", style="StatusOk.TLabel")
                break

    def create_add_device_config(self):
        supported_protocols = self.compute_supported_protocols()

        # Default add device config assumes all kinds of server capabilities
        config = self.context.instance.create_default_add_device_config()

        self.filter_device_section(config, supported_protocols)
        self.filter_streaming_section(config, supported_protocols)
        self.hidden_general_entries = self.filter_general_section(config, supported_protocols)

        return config

    def compute_supported_protocols(self):
        # Remote devices have protocol IDs defined in server capabilities
        if len(self.server_capabilities) > 0:
            supported_protocols = list()
            for protocol_id in self.server_capabilities:
                supported_protocols.append(protocol_id)
            return supported_protocols

        # For local devices
        supported_prefix = self.default_connection_string.split("://")[0]

        available_device_types = self.parent_device.available_device_types

        # Find protocol ID matching the connection string prefix
        for protocol_id, device_type in available_device_types.items():
            dt_dict = device_type.as_dictionary

            if dt_dict["Prefix"] == supported_prefix:
                return [protocol_id]
        return []

    def filter_device_section(self, config, supported_protocols):
        device_section = config.get_property_value(_DEVICE)
        for property in self.context.properties_of_component(device_section):
            if property.name not in supported_protocols:
                device_section.remove_property(property.name)

    def filter_streaming_section(self, config, supported_protocols):
        streaming_section = config.get_property_value(_STREAMING)
        for property in self.context.properties_of_component(streaming_section):
            if property.name not in supported_protocols:
                streaming_section.remove_property(property.name)

    def filter_general_section(self, config, supported_protocols):
        general_section = config.get_property_value(_GENERAL)
        hidden_property_paths = []

        def add_to_hidden(section, properties):
            for p in properties:
                if section.has_property(p):
                    hidden_property_paths.append(p)

        # Hide options that are covered with the UI interaction
        # The user edits allowed streaming protocols through checklist and
        # automatically connect streaming will connect if any allowed
        add_to_hidden(general_section, [_ASP, _ACS])

        if "OpenDAQNativeConfiguration" not in supported_protocols:
            add_to_hidden(general_section, ["Username", "Password", "ClientType", "ExclusiveControlDropOthers"])

        if not self.has_tcp_ip:
            add_to_hidden(general_section, ["PrimaryAddressType"])

        if not self.has_streaming:
            add_to_hidden(
                general_section,
                ["StreamingConnectionHeuristic",
                 "PrioritizedStreamingProtocols"])

        return hidden_property_paths

    def sync_device_and_streaming_configs(self):
        device_section = self.config.get_property_value(_DEVICE)
        streaming_section = self.config.get_property_value(_STREAMING)

        # Copy all the streaming configs from streaming to device, as they could only have been modified
        # in the streaming section
        for streaming in self.context.properties_of_component(streaming_section):
            for device in self.context.properties_of_component(device_section):
                if device.name == streaming.name:
                    # Recursively update
                    utils.update_properties(device.value, streaming.value)
                    break
