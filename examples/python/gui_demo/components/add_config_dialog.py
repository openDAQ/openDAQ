import tkinter as tk
from tkinter import ttk

from ..app_context import AppContext
from .dialog import Dialog
from .generic_properties_treeview import PropertiesTreeview

class AddConfigDialog(Dialog):
    def __init__(self, parent, context: AppContext, connection_string=None):
        super().__init__(parent, 'Add with config', context)
        self.connection_string = connection_string
        self.geometry('{}x{}'.format(
            1200 * self.context.ui_scaling_factor, 600 * self.context.ui_scaling_factor))

        self.protocol('WM_DELETE_WINDOW', self.cancel)

        ttk.Label(self, text=self.connection_string).pack(
            pady=10, padx=10, anchor=tk.NW, side=tk.TOP)

        button_frame = ttk.Frame(self)
        button_frame.pack(side=tk.BOTTOM, anchor=tk.E, pady=10)

        ttk.Button(button_frame, text='Add', command=self.add).pack(
            padx=10, ipadx=20 * self.context.ui_scaling_factor, side=tk.LEFT)
        ttk.Button(button_frame, text='Cancel', command=self.cancel).pack(
            padx=10, ipadx=10 * self.context.ui_scaling_factor, side=tk.LEFT)

        self.device_config = context.instance.create_default_add_device_config()

        PropertiesTreeview(self, self.device_config, context)

    def add(self):
        self.destroy()

    def cancel(self):
        self.device_config = None
        self.destroy()
