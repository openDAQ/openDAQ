import tkinter as tk
from .component import Component
from .output_signal_row import OutputSignalRow
from tkinter import ttk, simpledialog
import opendaq as daq


class OutputSignalsView(tk.Frame, Component):
    def __init__(self, parent, node=None, context=None, **kwargs):
        tk.Frame.__init__(self, parent, **kwargs)
        Component.__init__(self)
        self.parent = parent
        self.node = node
        self.context = context

        self.configure(padx=10, pady=5, border=1, relief=tk.GROOVE)
        self.refresh()

    def refresh(self):
        for widget in self.winfo_children():
            widget.pack_forget()
        ttk.Label(self, text='Output signals').pack(anchor=tk.W, pady=5)
        self.fill_output_signal(self.node)

    def fill_output_signal(self, node: daq.IDevice):
        if node is not None and (daq.IFunctionBlock.can_cast_from(node) or daq.IDevice.can_cast_from(node)):
            if daq.IDevice.can_cast_from(node):
                node = daq.IDevice.cast_from(node)
            elif daq.IFunctionBlock.can_cast_from(node):
                node = daq.IFunctionBlock.cast_from(node)

            if any(output_signal.domain_signal is None for output_signal in node.signals):
                # Code to execute if there are signals with domain_signal is not None
                for output_signal in node.signals:
                    if output_signal.domain_signal == None:
                        OutputSignalRow(self, output_signal, self.context).pack(
                            anchor=tk.NW, fill=tk.X)
                return

        ttk.Label(self, text='No output signals').pack(
            anchor=tk.CENTER, expand=True)
