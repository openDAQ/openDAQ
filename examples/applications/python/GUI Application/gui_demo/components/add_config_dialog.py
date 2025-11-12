import tkinter as tk
from tkinter import ttk

from ..app_context import AppContext
from .dialog import Dialog
from .generic_properties_treeview import PropertiesTreeview

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

        ttk.Label(self, text=self.device_info.connection_string).pack(
            pady=10, padx=10, anchor=tk.NW, side=tk.TOP)

        button_frame = ttk.Frame(self)
        button_frame.pack(side=tk.BOTTOM, anchor=tk.E, pady=10)

        ttk.Button(button_frame, text='Add', command=self.add).pack(
            padx=10, ipadx=20 * self.context.ui_scaling_factor, side=tk.LEFT)
        ttk.Button(button_frame, text='Cancel', command=self.cancel).pack(
            padx=10, ipadx=10 * self.context.ui_scaling_factor, side=tk.LEFT)

        self.device_config = self.create_add_device_config()

        PropertiesTreeview(self, self.device_config, context)

    def add(self):
        self.destroy()

    def cancel(self):
        self.device_config = None
        self.destroy()

    def close(self):
        # When escape is pressed adding the device is cancelled
        self.cancel()

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
