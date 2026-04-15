import tkinter as tk
from tkinter import ttk

import opendaq as daq
from .. import utils

from .input_port_row_view import InputPortRowView


class InputPortsView(ttk.Frame):
    def __init__(self, parent, node, context=None, **kwargs):
        ttk.Frame.__init__(self, parent, **kwargs)
        self.parent = parent
        self.node = node
        self.context = context

        self.configure(padding=(0, 5), borderwidth=0, relief=tk.FLAT)
        self.refresh()

    def refresh(self):
        for widget in self.children.values():
            widget.pack_forget()

        utils.make_banner(self, 'Input ports')
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
        ttk.Label(self, text='None').pack(
            anchor=tk.W, expand=True, padx=(10,0), pady=(5))
