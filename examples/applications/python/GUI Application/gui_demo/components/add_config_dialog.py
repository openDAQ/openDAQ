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

def remove_properties(section, properties):
    for p in properties:
        if section.has_property(p):
            section.remove_property(p)
        else:
            print(f"Property {p=} does not exist")

class AddConfigDialog(Dialog):
    def __init__(self, parent, context: AppContext, device_info, parent_device):
        super().__init__(parent, 'Add with config', context)
        self.context = context
        self.device_info = device_info
        self.parent_device = parent_device
        self.geometry('{}x{}'.format(
            1200 * self.context.ui_scaling_factor, 600 * self.context.ui_scaling_factor))

        self.protocol('WM_DELETE_WINDOW', self.cancel)

        # MARK: Layout creation
        self.rowconfigure(0, weight=1)
        self.columnconfigure(0, weight=1, uniform='column')
        self.columnconfigure(1, weight=3, uniform='column')

        left_frame = ttk.Frame(self)

        self.device_combobox = self.add_combobox(left_frame, "Device configuration:")
        self.device_combobox.bind("<<ComboboxSelected>>", self.handle_device_select)
        self.streaming_checklist = self.add_checkbox_list(left_frame, "Streaming configurations:")
        self.streaming_checklist.register_callback(self.handle_checklist_changed)

        left_frame.grid(row=0, column=0)
        left_frame.grid_configure(sticky=tk.NSEW)

        right_frame = ttk.Frame(self)

        self.tabs = ttk.Notebook(right_frame)
        self.tabs.bind('<<NotebookTabChanged>>', self.on_tab_change)
        self.tabs.pack(fill=tk.BOTH, expand=True)

        ttk.Button(right_frame, text='Add device',
                   command=self.handle_add_button_clicked).pack(side=tk.BOTTOM, anchor=tk.E, padx=10, pady=10, ipadx=20)

        right_frame.grid(row=0, column=1)
        right_frame.grid_configure(sticky=tk.NSEW)

        # Filter and save config, initialize left panel
        self.config = self.create_add_device_config()
        self.update_selection_widgets()

        # Initialize members related to editor tabs
        self.selected_device_config = self.device_combobox.get()
        self.selected_streaming_configs = []

        self.device_tab = self.create_device_tab()
        self.general_tab = self.create_general_tab()
        self.streaming_tabs = []

        self.update_general_tab()
        self.update_device_tab()
        self.update_streaming_tabs()

    def handle_add_button_clicked(self):
        self.sync_device_and_streaming_configs()
        self.finalize_general_section()
        self.destroy()

    def cancel(self):
        self.config = None
        self.destroy()

    def close(self):
        # When escape is pressed adding the device is cancelled
        print("Close")
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
    def handle_device_select(self, event=None):
        value = self.device_combobox.get()
        if not value or len(value) == 0:
            self.selected_device_config = None
            return

        self.selected_device_config = value
        self.update_device_tab()

    def handle_checklist_changed(self, checked_labels: list):
        self.selected_streaming_configs = checked_labels
        self.update_streaming_tabs()

        general_section = self.config.get_property_value(_GENERAL)
        _ASP = "AllowedStreamingProtocols"
        if general_section.has_property(_ASP):
            allowed_protocols = general_section.get_property(_ASP)
            data = allowed_protocols.value
            data.clear()
            for label in self.selected_streaming_configs:
                data.append(utils.value_to_coretype(
                    label, allowed_protocols.item_type))
            allowed_protocols.value = data
            self.general_tab.editor.refresh()

    def on_tab_change(self, e):
        pass

    # MARK: Update notebook tabs
    def create_device_tab(self):
        device_tab = AddDeviceConfigView(self.tabs)
        device_tab.pack(fill=tk.BOTH, expand=True)

        self.tabs.add(device_tab, text=_DEVICE)
        self.tabs.insert(0, device_tab)
        self.tabs.hide(device_tab)
        return device_tab

    def update_device_tab(self):
        if len(self.selected_device_config) == 0 or self.selected_device_config == _NO_CONFIG_STR:
            if self.device_tab:
                self.device_tab.clear()
                self.tabs.hide(self.device_tab)
            return

        device_section = self.config.get_property_value(_DEVICE)
        self.device_tab.edit(device_section.get_property(self.selected_device_config), self.context)
        self.tabs.add(self.device_tab) # unhide
        self.tabs.select(self.device_tab)

    def update_streaming_tabs(self):
        for tab in self.streaming_tabs:
            self.tabs.forget(tab)
            tab.destroy()

        streaming_section = self.config.get_property_value(_STREAMING)

        self.streaming_tabs = []
        for property_name in self.selected_streaming_configs:
            new_tab = AddDeviceConfigView(self)
            new_tab.edit(streaming_section.get_property(property_name), self.context)

            tab_title = property_name.strip("OpenDAQ").replace("Streaming", " Streaming")
            self.tabs.add(new_tab, text=tab_title)
            self.tabs.insert(tk.END, new_tab)
            self.streaming_tabs.append(new_tab)

        # TODO: Would be great to somehow open the last one that was checked - but this would require the checkbox
        # widget to tell us whether the event was to check or uncheck and which entry
        if len(self.streaming_tabs) > 0:
            self.tabs.select(self.streaming_tabs[0])

    def create_general_tab(self):
        general_tab = AddDeviceConfigView(self.tabs)
        general_tab.pack(fill=tk.BOTH, expand=True)

        self.tabs.add(general_tab, text=_GENERAL)
        self.tabs.insert(1, general_tab)
        self.tabs.hide(general_tab)
        return general_tab

    def update_general_tab(self):
        self.general_tab.edit(self.config.get_property(_GENERAL), self.context)
        self.tabs.add(self.general_tab)
        self.tabs.select(self.general_tab)

    def update_selection_widgets(self):
        self.update_combobox()
        self.update_checklist(self.config.get_property_value(_STREAMING))

    def update_combobox(self):
        # Lookup streaming options
        streaming_section = self.config.get_property_value(_STREAMING)
        streaming_options = []
        for property in self.context.properties_of_component(streaming_section):
            streaming_options.append(property.name)

        device_section = self.config.get_property_value(_DEVICE)
        device_options = [_NO_CONFIG_STR]

        for property in self.context.properties_of_component(device_section):
            # Avoid duplication in device combobox and streaming checklist
            if property.name not in streaming_options:
                device_options.append(property.name)
        self.device_combobox["values"] = device_options

        default_idx = min(1, len(device_options) - 1)
        self.device_combobox.set(device_options[default_idx])

    def update_checklist(self, streaming_section):
        for property in self.context.properties_of_component(streaming_section):
            self.streaming_checklist.insert(property.name)

    def create_add_device_config(self):
        supported_protocols = self.compute_supported_protocols()

        # Default add device config assumes all kinds of server capabilities
        config = self.context.instance.create_default_add_device_config()

        self.filter_device_section(config, supported_protocols)
        self.filter_streaming_section(config, supported_protocols)
        self.filter_general_section(config, supported_protocols)

        return config

    def compute_supported_protocols(self):
        server_capabilities = self.device_info.server_capabilities

        # Remote devices have protocol IDs defined in server capabilities
        if len(server_capabilities) > 0:
            supported_protocols = list()
            for c in server_capabilities:
                supported_protocols.append(c.protocol_id)
            return supported_protocols

        # For local devices
        supported_prefix = self.device_info.connection_string.split("://")[0]

        available_device_types = self.parent_device.available_device_types

        # Find protocol ID matching the connection string prefix
        for protocol_id, device_type in available_device_types.items():
            dt_dict = device_type.as_dictionary

            if dt_dict["Prefix"] == supported_prefix:
                return [protocol_id]
        return []

    def filter_device_section(self, config, supported_protocols):
        device_section = config.get_property_value("Device")
        for property in self.context.properties_of_component(device_section):
            if property.name not in supported_protocols:
                device_section.remove_property(property.name)

    def filter_streaming_section(self, config, supported_protocols):
        streaming_section = config.get_property_value("Streaming")
        for property in self.context.properties_of_component(streaming_section):
            if property.name not in supported_protocols:
                streaming_section.remove_property(property.name)

    def filter_general_section(self, config, supported_protocols):
        general_section = config.get_property_value("General")

        if "OpenDAQNativeConfiguration" not in supported_protocols:
            remove_properties(general_section, ["Username", "Password", "ClientType", "ExclusiveControlDropOthers"])

        # Find TCP/IP and streaming capabilities
        has_tcp_ip = False
        has_streaming = False
        for c in self.device_info.server_capabilities:
            if c.get_property_value("ConnectionType") == "TCP/IP":
                has_tcp_ip = True
            if c.get_property_value("ProtocolType") in ["Streaming", "ConfigurationAndStreaming"]:
                has_streaming = True

        if not has_tcp_ip:
            remove_properties(general_section, ["PrimaryAddressType"])

        if not has_streaming:
            remove_properties(
                general_section,
                ["StreamingConnectionHeuristic",
                 "PrioritizedStreamingProtocols",
                 "AllowedStreamingProtocols",
                 "AutomaticallyConnectStreaming"])

    def sync_device_and_streaming_configs(self):
        device_section = self.config.get_property_value(_DEVICE)
        streaming_section = self.config.get_property_value(_STREAMING)

        # Copy all the streaming configs from streaming to device, as they could only have been modified
        # in the streaming section
        for streaming in self.context.properties_of_component(streaming_section):
            for device in self.context.properties_of_component(device_section):
                if device.name == streaming.name:
                    print(f"\n\nSync {device.name}")
                    # Recursively update
                    utils.update_properties(device.value, streaming.value)
                    print("\n\n")
                    break

    def finalize_general_section(self):
        general_section = self.config.get_property_value(_GENERAL)

        # TODO: Extend general_config
        # general_section.set_property("SelectedDeviceConfig", self.selected_device_config)
