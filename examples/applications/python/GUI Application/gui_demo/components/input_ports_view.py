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

        self.configure(padding=(0, 5), borderwidth=0, relief=tk.FLAT)
        self._title_bg = '#afafaf'
        self._title_fg = 'white'

        self.refresh()

    def refresh(self):
        for widget in self.children.values():
            widget.pack_forget()

        title_bar = tk.Frame(self, bg=self._title_bg, bd=0, highlightthickness=0)
        title_bar.pack(side=tk.TOP, fill=tk.X, pady=(0, 8))
        tk.Label(
            title_bar,
            text='Input ports',
            bg=self._title_bg,
            fg=self._title_fg,
            font=('TkDefaultFont', 10, 'bold')
        ).pack(side=tk.LEFT, padx=6, pady=2)
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
            anchor=tk.W, expand=True, padx=(10,0), pady=(5,0))
