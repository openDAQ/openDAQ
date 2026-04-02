import tkinter as tk
from tkinter import ttk
from datetime import datetime, timezone

import opendaq as daq

from ..app_context import AppContext


class DomainView(ttk.Frame):
    def __init__(self, parent, device, context: AppContext = None, **kwargs):
        ttk.Frame.__init__(self, parent, **kwargs)
        self.device = device
        self.context = context
        self.configure(padding=(10, 5))

        ttk.Label(self, text='Domain', font=("TkDefaultFont", 10, "bold")).pack(
            fill=tk.X, pady=5)

        self.info_frame = ttk.Frame(self)
        self.info_frame.pack(fill=tk.BOTH, expand=True)

        self.time_label = None
        self._populate()
        self._update_time()

    def _populate(self):
        domain = self.device.domain
        if domain is None:
            ttk.Label(self.info_frame, text="No domain available",
                      foreground="gray").pack(anchor=tk.W)
            self.time_label = ttk.Label(self.info_frame, text="")
            return

        fields = []
        fields.append(("Origin", domain.origin))
        fields.append(("Tick Resolution", str(domain.tick_resolution)))

        unit = domain.unit
        if unit:
            fields.append(("Unit", f"{unit.name} ({unit.symbol})"))
            if unit.quantity:
                fields.append(("Quantity", unit.quantity))

        ref = domain.reference_domain_info
        if ref:
            fields.append(("Ref Domain ID", ref.reference_domain_id))
            fields.append(("Ref Offset", str(ref.reference_domain_offset)))

        row = ttk.Frame(self.info_frame)
        row.pack(fill=tk.X, pady=2)

        for label, value in fields:
            ttk.Label(row, text=f"{label}:", foreground="gray").pack(side=tk.LEFT, padx=(0, 2))
            ttk.Label(row, text=str(value)).pack(side=tk.LEFT, padx=(0, 10))

        time_row = ttk.Frame(self.info_frame)
        time_row.pack(fill=tk.X, pady=2)
        ttk.Label(time_row, text="Current Time:", foreground="gray").pack(side=tk.LEFT, padx=(0, 2))
        self.time_label = ttk.Label(time_row, text="")
        self.time_label.pack(side=tk.LEFT)

    def _update_time(self):
        if self.time_label is None:
            return

        try:
            domain = self.device.domain
            if domain is None:
                self.time_label.config(text="N/A")
                self.after(1000, self._update_time)
                return

            ticks = self.device.ticks_since_origin
            resolution = domain.tick_resolution

            res_str = str(resolution)
            if '/' in res_str:
                num, den = res_str.split('/')
                seconds = ticks * int(num) / int(den)
            else:
                seconds = ticks * float(res_str)

            origin_str = domain.origin
            try:
                origin = datetime.fromisoformat(origin_str.replace('Z', '+00:00'))
                current = datetime.fromtimestamp(
                    origin.timestamp() + seconds, tz=timezone.utc)
                self.time_label.config(
                    text=current.strftime('%Y-%m-%d %H:%M:%S.%f')[:-3] + ' UTC')
            except Exception:
                self.time_label.config(text=f"{ticks} ticks")
        except Exception as e:
            self.time_label.config(text=f"N/A ({e})")

        self.after(1000, self._update_time)