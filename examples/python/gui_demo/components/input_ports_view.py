import tkinter as tk
from tkinter import ttk

import opendaq as daq

from .input_port_row_view import InputPortRowView


class InputPortsView(ttk.Frame):
    def __init__(self, parent, node, context=None, **kwargs):
        ttk.Frame.__init__(self, parent, **kwargs)
        self.parent = parent
        self.node = node
        self.context = context

        self.configure(padding=(10, 5), border=1, relief=tk.GROOVE)

        self.refresh()

    def refresh(self):
        for widget in self.children.values():
            widget.pack_forget()

        ttk.Label(self, text='Input ports').pack(
            side=tk.TOP, fill=tk.X, pady=5)
        self.fill_input_ports(self.node)

    def fill_input_ports(self, node):
        if node is not None:
            if daq.IFunctionBlock.can_cast_from(node):
                node = daq.IFunctionBlock.cast_from(node)
                if len(node.input_ports) != 0:
                    for input_port in node.input_ports:
                        input_port_row = InputPortRowView(
                            self, input_port, self.context)
                        input_port_row.pack(anchor=tk.NW, fill=tk.X)
                    return
        ttk.Label(self, text='No input ports').pack(
            anchor=tk.CENTER, expand=True)
