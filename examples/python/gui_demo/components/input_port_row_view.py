import tkinter as tk
import opendaq as daq
from tkinter import ttk
from ..utils import *
from .attributes_dialog import AttributesDialog


class InputPortRowView(tk.Frame):
    def __init__(self, parent, input_port: daq.IInputPort, context=None, **kwargs):
        tk.Frame.__init__(self, parent, **kwargs)
        self.parent = parent
        self.input_port = input_port
        self.selection = ''
        self.context = context

        self.configure(padx=10, pady=5, border=1, relief=tk.GROOVE)

        ttk.Label(self, text=input_port.name).pack(side=tk.LEFT, padx=5)
        self.input_var = tk.StringVar()
        self.dropdown = ttk.Combobox(self, textvariable=self.input_var)
        self.dropdown.pack(side=tk.LEFT, fill=tk.X, padx=5, expand=True)

        self.dropdown.bind('<<ComboboxSelected>>', self.handle_dropdown_select)
        self.dropdown.configure(state='readonly')

        self.edit_icon = context.icons['settings'] if context and context.icons and 'settings' in context.icons else None
        self.connect_icon = context.icons['link'] if context and context.icons and 'link' in context.icons else None
        self.disconnect_icon = context.icons['unlink'] if context and context.icons and 'unlink' in context.icons else None

        self.edit_button = ttk.Button(
            self, text='Edit', image=self.edit_icon, command=self.handle_edit_clicked)
        self.edit_button.pack(side=tk.RIGHT)

        self.connect_button = ttk.Button(
            self, text='Connect', image=self.connect_icon, command=self.handle_connect_clicked)
        self.connect_button.pack(side=tk.RIGHT, padx=5)

        device = root_device(input_port)
        device = device if device is not None and daq.IDevice.can_cast_from(
            device) else None

        if device is not None:
            self.device = daq.IDevice.cast_from(device)
            self.refresh()

    def refresh(self):
        if self.device is not None:
            self.fill_dropdown()
        if self.input_port is not None and self.input_port.connection is not None:
            self.input_var.set(self.input_port.connection.signal.global_id)
            self.dropdown.set(self.input_port.connection.signal.global_id)

    def fill_dropdown(self):
        signals = ['none']

        signals += [signal.global_id for signal in self.device.signals_recursive]
        self.dropdown['values'] = signals
        self.selection = ''

    def handle_dropdown_select(self, event):
        self.selection = self.input_var.get()
        print(self.selection)
        if self.selection == 'none':
            self.connect_button.configure(
                text='Disconnect', image=self.disconnect_icon)
        else:
            self.connect_button.configure(
                text='Connect', image=self.connect_icon)

    def handle_edit_clicked(self):
        print('Edit clicked')
        if self.input_port is not None:
            AttributesDialog(self, 'Attributes', self.input_port).show()

    def handle_connect_clicked(self):
        print('Connect clicked')
        if (self.selection == 'none'):
            self.input_port.disconnect()
        elif self.selection != '':
            for signal in self.device.signals_recursive:
                if signal.global_id == self.selection:
                    self.input_port.connect(signal)
                    break
