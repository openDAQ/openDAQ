import tkinter as tk
from tkinter import ttk

from ..app_context import AppContext
from .dialog import Dialog
from .generic_properties_treeview import PropertiesTreeview

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

    def create_add_device_config(self):
        server_capabilities = self.device_info.server_capabilities

        # Device is either remote with 1 (or more) server capabilities or local with a single supported prefix
        supported_prefixes = list()
        if len(server_capabilities) > 0:
            supported_prefixes = [c.prefix for c in server_capabilities]
        else:
            supported_prefixes = [self.device_info.connection_string.split("://")[0]]

        available_device_types = self.parent_device.available_device_types

        # Protocols supported on the device
        supported_protocols = list()
        for protocol_id, device_type in available_device_types.items():
            dt_dict = device_type.as_dictionary

            if dt_dict["Prefix"] in supported_prefixes:
                supported_protocols.append(protocol_id)

        # Default add device config assumes all kinds of server capabilities
        config = self.context.instance.create_default_add_device_config()

        device_section = config.get_property("Device").value
        for property in self.context.properties_of_component(device_section):
            if property.name not in supported_protocols:
                device_section.remove_property(property.name)

        streaming_section = config.get_property("Streaming").value
        for property in self.context.properties_of_component(streaming_section):
            if property.name not in supported_protocols:
                streaming_section.remove_property(property.name)

        return config
