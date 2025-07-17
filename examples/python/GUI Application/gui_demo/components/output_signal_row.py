import tkinter as tk
from tkinter import ttk

import opendaq as daq

from .. import utils
from .attributes_dialog import AttributesDialog


class OutputSignalRow(ttk.Frame):
    def __init__(self, parent, output_signal, context=None, **kwargs):
        ttk.Frame.__init__(self, parent, **kwargs)
        self.parent = parent
        self.output_signal = output_signal
        self.selection = ''
        self.context = context

        self.configure(padding=(10, 5))

        last_value = utils.get_last_value_for_signal(output_signal)
        ttk.Label(self, text=output_signal.name, anchor=tk.W).grid(
            row=0, column=0, sticky=tk.W)
        ttk.Label(self, text=str(last_value), anchor=tk.W).grid(
            row=0, column=1, sticky=tk.E)

        self.edit_icon = context.icons['settings'] if context and context.icons and 'settings' in context.icons else None
        self.edit_button = tk.Button(
            self, text='Edit', image=self.edit_icon, borderwidth=0, command=self.handle_edit_clicked)
        self.edit_button.grid(row=0, column=2, sticky=tk.E)

        self.grid_columnconfigure(0, weight=10)
        self.grid_columnconfigure(1, weight=10)
        self.grid_columnconfigure(2, weight=1, minsize=40)
        self.grid_columnconfigure((0, 1, 2), uniform='uniform')

    def refresh(self):
        pass

    def handle_edit_clicked(self):
        if self.output_signal is not None:
            AttributesDialog(self, 'Attributes', self.output_signal, self.context).show()
