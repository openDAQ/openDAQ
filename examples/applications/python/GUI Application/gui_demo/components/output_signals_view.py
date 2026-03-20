import tkinter as tk
from tkinter import ttk

import opendaq as daq

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
        self._title_bg = '#afafaf'
        self._title_fg = 'white'
        self.refresh()
        self._schedule_periodic_refresh()
        self.bind('<Destroy>', self._on_destroy)

    def refresh(self):
        for widget in self.winfo_children():
            widget.pack_forget()
        self._rows = []
        title_bar = tk.Frame(self, bg=self._title_bg, bd=0, highlightthickness=0)
        title_bar.pack(fill=tk.X, pady=(0, 8))
        tk.Label(
            title_bar,
            text='Output signals',
            bg=self._title_bg,
            fg=self._title_fg,
            font=('TkDefaultFont', 10, 'bold')
        ).pack(side=tk.LEFT, padx=6, pady=2)
        self.fill_output_signal(self.node)

    def fill_output_signal(self, node):
        if node is not None and (daq.IFunctionBlock.can_cast_from(node) or daq.IDevice.can_cast_from(node)):
            if daq.IDevice.can_cast_from(node):
                node = daq.IDevice.cast_from(node)
            elif daq.IFunctionBlock.can_cast_from(node):
                node = daq.IFunctionBlock.cast_from(node)

            signals = node.get_signals(daq.AnySearchFilter() if self.context.view_hidden_components else None)
            if len(signals) > 0:
                for output_signal in signals:
                    row = OutputSignalRow(self, output_signal, self.context)
                    row.pack(
                        anchor=tk.NW, fill=tk.X)
                    self._rows.append(row)
                return

        ttk.Label(self, text='No output signals').pack(
            anchor=tk.CENTER, expand=True)

    def _schedule_periodic_refresh(self):
        if self._refresh_job is not None:
            return
        self._refresh_job = self.after(200, self._periodic_refresh_tick)

    def _periodic_refresh_tick(self):
        self._refresh_job = None
        if not self.winfo_exists():
            return
        # Update only when this view is visible to avoid unnecessary work.
        if self.winfo_ismapped():
            for row in list(self._rows):
                try:
                    if row.winfo_exists():
                        row.refresh()
                except Exception:
                    pass
        self._schedule_periodic_refresh()

    def _on_destroy(self, event):
        if event.widget is not self:
            return
        if self._refresh_job is not None:
            try:
                self.after_cancel(self._refresh_job)
            except Exception:
                pass
            self._refresh_job = None
