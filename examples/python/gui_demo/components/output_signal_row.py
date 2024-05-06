import tkinter as tk
import opendaq as daq
from tkinter import ttk

from ..utils import *
from .attributes_dialog import AttributesDialog


class OutputSignalRow(tk.Frame):
    def __init__(self, parent, output_signal, context=None, **kwargs):
        tk.Frame.__init__(self, parent, **kwargs)
        self.parent = parent
        self.output_signal = output_signal
        self.selection = ''
        self.context = context

        self.configure(padx=10, pady=5)

        ttk.Label(self, text=output_signal.name).pack(side=tk.LEFT, padx=5)
        ttk.Label(self, text=str(output_signal.last_value)
                  ).pack(side=tk.LEFT, padx=5, expand=True)

        self.edit_icon = context.icons['settings'] if context and context.icons and 'settings' in context.icons else None
        self.edit_button = tk.Button(
            self, text='Edit', image=self.edit_icon, borderwidth=0, command=self.handle_edit_clicked)
        self.edit_button.pack(side=tk.RIGHT)

    def refresh(self):
        pass

    def handle_edit_clicked(self):
        if self.output_signal is not None:
            AttributesDialog(self, 'Attributes', self.output_signal).show()
