import tkinter as tk
import opendaq as daq
from tkinter import ttk
from gui_demo.utils import *
from gui_demo.components.attributes_dialog import AttributesDialog

class OutputSignalRow(tk.Frame):
    def __init__(self, parent, output_signal: daq.ISignal, **kwargs):
        tk.Frame.__init__(self, parent, **kwargs)
        self.parent = parent
        self.output_signal = output_signal
        self.selection = ''

        self.configure(padx=10, pady=5, border=1, relief=tk.GROOVE)

        ttk.Label(self, text=output_signal.name).pack(side=tk.LEFT, padx=5)
        ttk.Label(self, text=str(output_signal.last_value)).pack(side=tk.LEFT, padx=5)

        self.edit_button = ttk.Button(
            self, text='Edit', command=self.handle_edit_clicked)
        self.edit_button.pack(side=tk.RIGHT)

    def refresh(self):
        pass

    def handle_edit_clicked(self):
        print('Edit clicked')
        if self.output_signal is not None:
            AttributesDialog(self, 'Attributes', self.output_signal).show()