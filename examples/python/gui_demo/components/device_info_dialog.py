import tkinter as tk
from tkinter import ttk

import opendaq as daq

from ..app_context import AppContext
from .dialog import Dialog
from .metadata_fields_selector_dialog import MetadataFieldsSelectorDialog
from .generic_properties_treeview import PropertiesTreeview


class DeviceInfoDialog(Dialog):

    def __init__(self, parent, node: daq.IDeviceInfo, context: AppContext, title='', **kwargs):
        super().__init__(parent, f'Device {node.name} info', context, **kwargs)
        self.context = context
        self.geometry(f'{600}x{800}')

        header_frame = ttk.Frame(self)

        ttk.Label(header_frame, text='Properties').pack(side=tk.LEFT, pady=5)
        tk.Button(header_frame, text='Edit', image=self.context.icons['settings'], borderwidth=0,
                  command=lambda: MetadataFieldsSelectorDialog(
                      self, self.context).show()
                  ).pack(side=tk.RIGHT, anchor=tk.E)

        header_frame.pack(fill=tk.X)

        PropertiesTreeview(self, node, context)