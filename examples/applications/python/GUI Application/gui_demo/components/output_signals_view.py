import tkinter as tk
from tkinter import ttk

import opendaq as daq
from .. import utils

from .output_signal_row import OutputSignalRow


class OutputSignalsView(ttk.Frame):
    def __init__(self, parent, node=None, context=None, **kwargs):
        ttk.Frame.__init__(self, parent, **kwargs)
        self.parent = parent
        self.node = node
        self.context = context
        self._rows = []
        self._refresh_job = None

        self.configure(padding=(0, 5), borderwidth=0, relief=tk.FLAT)
        self.refresh()
        utils.poll_signal_rows(self, self._rows, interval_ms=200, _job_attr='_refresh_job')
        self.bind('<Destroy>', self._on_destroy)

    def refresh(self):
        for widget in self.winfo_children():
            widget.pack_forget()
        self._rows = []

        if daq.ISignal.can_cast_from(self.node):
            node = daq.ISignal.cast_from(self.node)
            self.make_output_signal_section([node], 'Signal info')
            self.make_output_signal_section([node.domain_signal], 'Domain signal')
            self.make_output_signal_section(node.related_signals, 'Related signals')
        elif daq.IDevice.can_cast_from(self.node):
            node = daq.IDevice.cast_from(self.node)
            signals = node.get_signals(daq.AnySearchFilter() if self.context.view_hidden_components else None)
            self.make_output_signal_section(signals, 'Output signals')
        elif daq.IFunctionBlock.can_cast_from(self.node):
            node = daq.IFunctionBlock.cast_from(self.node)
            signals = node.get_signals(daq.AnySearchFilter() if self.context.view_hidden_components else None)
            self.make_output_signal_section(signals, 'Output signals')

        return  
    
    def make_output_signal_section(self, signals, banner_text):
        utils.make_banner(self, banner_text)
        if len(signals) > 0:
            for output_signal in signals:
                if output_signal is None:
                    continue
                
                row = OutputSignalRow(self, output_signal, self.context)
                row.pack(
                    anchor=tk.NW, fill=tk.X)
                self._rows.append(row)
            return

        ttk.Label(self, text='None').pack(
            anchor=tk.W, expand=True, padx=(10,0), pady=(5))
        
    def _on_destroy(self, event):
        if event.widget is not self:
            return
        if self._refresh_job is not None:
            try:
                self.after_cancel(self._refresh_job)
            except Exception:
                pass
            self._refresh_job = None
