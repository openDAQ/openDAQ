import tkinter as tk
from tkinter import ttk

import opendaq as daq
from .. import utils
from datetime import timedelta

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
            related = node.related_signals
            self.make_output_signal_section(
                related if related is not None else [], 'Related signals')
        elif daq.IDevice.can_cast_from(self.node):
            node = daq.IDevice.cast_from(self.node)
            signals = node.get_signals(daq.AnySearchFilter() if self.context.view_hidden_components else None)
            self.make_output_signal_section(signals, 'Output signals')
            self.make_device_domain_section(node)
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


    def make_device_domain_section(self, device_node):
        if not hasattr(device_node, 'domain') or device_node.domain is None:
            return

        device_domain = device_node.domain
        utils.make_banner(self, 'Device domain')

        row = ttk.Frame(self, padding=(10, 5))
        row.pack(fill=tk.X)
        
        unit = device_domain.unit
        if unit and unit.quantity:
            label_text = unit.quantity.capitalize()
        else:
            label_text = f"{device_node.name} Domain"
            
        ttk.Label(row, text=label_text).pack(side=tk.LEFT)
        
        value_label = ttk.Label(row, text='N/A')
        value_label.pack(side=tk.RIGHT, padx=(4, 0))

        res = device_domain.tick_resolution
        origin_dt = None

        try:
            origin = device_domain.origin
            origin_str = str(origin) if origin is not None else ''
        except RuntimeError:
            origin_str = ''

        if res is not None and origin_str.strip():
            origin_dt = utils.parse_origin(origin_str)

        res_num = res.numerator if res is not None else None
        res_den = res.denominator if res is not None else None

        def _refresh():
            if not hasattr(device_node, 'ticks_since_origin'):
                return
            ticks = device_node.ticks_since_origin
            if ticks is None:
                return
            if origin_dt is not None and res_num is not None and res_den is not None:
                seconds = ticks * res_num / res_den
                ts = origin_dt + timedelta(seconds=seconds)
                value_label.config(
                    text=ts.strftime('%Y-%m-%d %H:%M:%S.%f')
                    .rstrip('0').rstrip('.'))
            else:
                value_label.config(text=f'Ticks: {ticks}')

        row.refresh = _refresh
        self._rows.append(row)