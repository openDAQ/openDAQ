import tkinter as tk
from .component import Component
from gui_demo.components.input_port_row_view import InputPortRowView
from tkinter import ttk, simpledialog
import opendaq as daq


class InputPortsView(tk.Frame, Component):
    def __init__(self, parent, node: daq.IFunctionBlock, **kwargs):
        tk.Frame.__init__(self, parent, **kwargs)
        Component.__init__(self)
        self.parent = parent
        self.node = node

        self.configure(padx=10, pady=5, border=1, relief=tk.GROOVE)

        self.refresh()

    def refresh(self):
        for widget in self.children.values():
            widget.pack_forget()

        ttk.Label(self, text='Input ports').pack(side=tk.TOP, fill=tk.X, pady=5)
        self.fill_input_ports(self.node)

    def fill_input_ports(self, node: daq.IFunctionBlock):
        if node is not None:
            if daq.IFunctionBlock.can_cast_from(node):
                node = daq.IFunctionBlock.cast_from(node)
                if len(node.input_ports) != 0:
                    for input_port in node.input_ports:
                        input_port_row = InputPortRowView(self, input_port)
                        input_port_row.pack(anchor=tk.NW, fill=tk.X)
                    return
        ttk.Label(self, text='No input ports').pack(anchor=tk.W, fill=tk.X)
