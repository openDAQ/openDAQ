import tkinter as tk
from tkinter import ttk

import opendaq as daq

from .. import utils
from ..event_port import EventPort
from ..app_context import AppContext
from .attributes_dialog import AttributesDialog


class InputPortRowView(ttk.Frame):
    def __init__(self, parent, input_port, context=None, **kwargs):
        ttk.Frame.__init__(self, parent, **kwargs)
        self.parent = parent
        self.input_port = input_port
        self.selection = ''
        self.context = context
        self.event_port = EventPort(self)

        self.configure(padding=(10, 5))

        ttk.Label(self, text=input_port.name).grid(
            row=0, column=0, sticky=tk.NSEW)
        self.input_var = tk.StringVar()
        self.dropdown = ttk.Combobox(self, textvariable=self.input_var)
        self.dropdown.grid(row=0, column=1, sticky=tk.NSEW)
        self.dropdown.bind('<<ComboboxSelected>>', self.handle_dropdown_select)
        self.dropdown.configure(state='readonly')

        self.edit_icon = context.icons['settings'] if context and context.icons and 'settings' in context.icons else None
        self.connect_icon = context.icons['link'] if context and context.icons and 'link' in context.icons else None
        self.disconnect_icon = context.icons['unlink'] if context and context.icons and 'unlink' in context.icons else None

        self.connect_button = tk.Button(
            self, text='Connect', image=self.connect_icon, borderwidth=0, command=self.handle_connect_clicked)
        self.connect_button.grid(row=0, column=2, sticky=tk.NSEW)

        self.edit_button = tk.Button(
            self, text='Edit', image=self.edit_icon, borderwidth=0, command=self.handle_edit_clicked)
        self.edit_button.grid(row=0, column=3, sticky=tk.NSEW)

        self.grid_columnconfigure(0, weight=3)
        self.grid_columnconfigure(1, weight=16)
        self.grid_columnconfigure(2, weight=1, minsize=30)
        self.grid_columnconfigure(3, weight=1, minsize=30)
        self.grid_columnconfigure((0, 1, 2, 3), uniform='uniform')

        device = utils.root_device(input_port)
        device = device if device is not None and daq.IDevice.can_cast_from(
            device) else None

        if device is not None:
            self.device = daq.IDevice.cast_from(device)
            self.refresh()

    def refresh(self):
        if self.device is not None:
            self.context.update_signals_for_device(self.device)
            self.fill_dropdown()
        if self.input_port is not None and self.input_port.signal is not None:
            short_id = self.context.short_id(self.input_port.signal.global_id)
            self.input_var.set(short_id)
            self.dropdown.set(short_id)

    def fill_dropdown(self):
        signals = [signal_id for signal_id in self.context.signals_for_device(
            self.device).keys()]
        if not self.context.view_hidden_components:
            signals = list(filter(lambda signal_id: self.context.signals_for_device(
                self.device)[signal_id].visible, signals))
        signals = ['none'] + signals

        self.dropdown['values'] = signals
        self.selection = ''

    def handle_dropdown_select(self, event):
        self.selection = self.input_var.get()
        if self.selection == 'none':
            self.connect_button.configure(
                text='Disconnect', image=self.disconnect_icon)
        else:
            self.connect_button.configure(
                text='Connect', image=self.connect_icon)

    def handle_edit_clicked(self):
        if self.input_port is not None:
            AttributesDialog(self, 'Attributes', self.input_port, self.context).show()

    def handle_connect_clicked(self):
        if self.selection == 'none':
            self.input_port.disconnect()
            self.event_port.emit()
        elif self.selection != '':
            selected_signal = self.context.signals_for_device(self.device)[
                self.selection]
            self.input_port.connect(selected_signal)
            self.event_port.emit()
