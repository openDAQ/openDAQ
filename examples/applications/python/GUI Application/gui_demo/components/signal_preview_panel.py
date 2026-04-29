import math
import time
import tkinter as tk
from tkinter import ttk
from collections import deque
import numpy as np

import opendaq as daq

from .. import utils


class SignalPreviewPanel(ttk.Frame):
    WINDOW_SECONDS = 5.0
    MAX_BUFFER_SIZE = 100_000
    POLL_MS = 33          # drain the reader this often
    _TARGET_PPS = 2000
    DRAW_MS = 66         # repaint the chart this often
    CANVAS_HEIGHT = 160

    _BG      = '#ffffff'
    _LINE    = '#1a6dcc'
    _GRID    = '#e4e4e4'
    _TEXT    = '#444444'
    _AXIS    = '#999999'
    _FILL    = '#dceafa'

    _AXIS_FONT = ('TkFixedFont', 7)
    _VAL_FONT  = ('TkFixedFont', 10, 'bold')

    def __init__(self, parent, node, context=None, **kwargs):
        super().__init__(parent, **kwargs)
        self.node = node
        self.context = context

        self._collapsed = False
        self._reader = None
        self._selected_signal = None
        self._poll_job = None
        self._draw_job = None

        # Buffer of (elapsed_seconds, value) pairs.
        self._data = deque(maxlen=self.MAX_BUFFER_SIZE)

        self._tick_res_num = None
        self._tick_res_den = None
        self._origin_epoch = None   # datetime | None
        self._first_tick = None

        # name -> ISignal for chartable signals
        self._eligible = {}
        self._unit_str = ''
        self._last_data_time = None
        self._STALE_TIMEOUT = 2.0
        self._needs_redraw = True
        self._chart_ready = False
        self._px_ml = 65

        # Build widgets
        self._build_toggle_bar()
        self._build_content()
        self._content_frame.pack(
            fill=tk.BOTH, expand=True, after=self._toggle_frame)
        self._populate_dropdown()

        self._schedule_poll()
        self._schedule_draw()
        self.bind('<Destroy>', self._on_destroy)

    def _build_toggle_bar(self):
        self._toggle_frame = utils.make_banner(self, '\u25BC Signal Preview')
        self._toggle_frame.configure(cursor='hand2')

        self._arrow_label = self._toggle_frame.winfo_children()[0]
        self._arrow_label.configure(cursor='hand2')

        for w in (self._toggle_frame, self._arrow_label):
            w.bind('<Button-1>', lambda _e: self._toggle())

    def _build_content(self):
        self._content_frame = ttk.Frame(self)
        dd_row = ttk.Frame(self._content_frame)
        dd_row.pack(fill=tk.X, padx=4, pady=(4, 2))

        ttk.Label(dd_row, text='Signal:').pack(side=tk.LEFT, padx=(0, 4))

        self._signal_var = tk.StringVar()
        self._dropdown = ttk.Combobox(
            dd_row, textvariable=self._signal_var, state='readonly')
        self._dropdown.pack(side=tk.LEFT, fill=tk.X, expand=True)
        self._dropdown.bind('<<ComboboxSelected>>', self._on_signal_selected)

        # Duration selector
        self._duration_presets = [1, 2, 5, 10, 30, 60]
        self._duration_var = tk.StringVar(value='5s')
        dur_cb = ttk.Combobox(
            dd_row, textvariable=self._duration_var, state='readonly',
            values=[f'{d}s' for d in self._duration_presets], width=5)
        dur_cb.pack(side=tk.LEFT, padx=(4, 0))
        dur_cb.bind('<<ComboboxSelected>>', self._on_duration_selected)

        self._log_var = tk.BooleanVar(value=False)
        log_cb = ttk.Checkbutton(
            dd_row, text='Log', variable=self._log_var,
            command=self._on_log_toggled)
        log_cb.pack(side=tk.LEFT, padx=(4, 0))

        # Chart canvas
        self._chart = tk.Canvas(
            self._content_frame, bg=self._BG,
            height=self.CANVAS_HEIGHT, highlightthickness=0)
        self._chart.pack(fill=tk.BOTH, expand=True, padx=4, pady=(2, 4))

        self._chart.bind('<Configure>', lambda _e: self._invalidate_chart())
        self._block_mousewheel_recursive(self)

    def _block_mousewheel_recursive(self, widget):
        for event in ('<MouseWheel>', '<Button-4>', '<Button-5>'):
            widget.bind(event, lambda _e=None: 'break')
        for child in widget.winfo_children():
            self._block_mousewheel_recursive(child)

    # MARK: Dropdown
    def _populate_dropdown(self):
        self._eligible.clear()

        if self.node is None:
            return

        signals = []
        search_filter = (
            daq.AnySearchFilter()
            if self.context and self.context.view_hidden_components
            else None)

        if daq.ISignal.can_cast_from(self.node):
            signals = [daq.ISignal.cast_from(self.node)]
        elif daq.IDevice.can_cast_from(self.node):
            device = daq.IDevice.cast_from(self.node)
            signals = device.get_signals(search_filter)
        elif daq.IFunctionBlock.can_cast_from(self.node):
            fb = daq.IFunctionBlock.cast_from(self.node)
            signals = fb.get_signals(search_filter)

        for sig in signals:
            if sig is None or not daq.ISignal.can_cast_from(sig):
                continue
            signal = daq.ISignal.cast_from(sig)
            if not self._is_chartable(signal):
                continue

            name = signal.name if signal.name else signal.global_id

            display = name
            if display in self._eligible:
                local_id = getattr(signal, 'local_id', signal.global_id)
                display = f'{name} ({local_id})'

                first_signal = self._eligible.pop(name)
                if first_signal.local_id is not None:
                    first_local = first_signal.local_id
                else:
                    first_local = first_signal.global_id
                    
                self._eligible[f'{name} ({first_local})'] = first_signal

            self._eligible[display] = signal

        names = list(self._eligible.keys())
        self._dropdown['values'] = names

        if len(names) == 1:
            self._signal_var.set(names[0])
            self._on_signal_selected()

    @staticmethod
    def _is_chartable(signal):
        desc = getattr(signal, 'descriptor', None)
        if desc is None:
            return False

        sample_type = getattr(desc, 'sample_type', None)
        if not sample_type:
            return False

        st_name = getattr(sample_type, 'name', str(sample_type))
        numeric_prefixes = ('Float', 'Int', 'UInt', 'float', 'int', 'uint')
        if not any(st_name.startswith(p) for p in numeric_prefixes):
            return False

        dims = getattr(desc, 'dimensions', None)
        if dims and len(dims) > 0:
            return False

        fields = getattr(desc, 'struct_fields', None)
        if fields and len(fields) > 0:
            return False

        domain_sig = getattr(signal, 'domain_signal', None)
        if domain_sig is None:
            return False

        domain_desc = getattr(domain_sig, 'descriptor', None)
        if domain_desc is None:
            return False

        unit = getattr(domain_desc, 'unit', None)
        symbol = getattr(unit, 'symbol', None)
        if str(symbol) != 's':
            return False

        if getattr(domain_desc, 'tick_resolution', None) is None:
            return False

        return True

    # MARK: Signal selection

    def _on_signal_selected(self, _event=None):
        name = self._signal_var.get()
        signal = self._eligible.get(name)
        if signal is None:
            return

        self._reader = None
        self._selected_signal = signal
        self._data.clear()
        self._first_tick = None
        self._needs_redraw = True

        self._unit_str = ''
        if signal.descriptor.unit is not None:
            unit = signal.descriptor.unit
            if unit is not None and unit.symbol is not None:
                self._unit_str = str(unit.symbol)

        # Cache tick resolution for tick -> seconds conversion.
        domain_desc = signal.domain_signal.descriptor
        res = domain_desc.tick_resolution
        self._tick_res_num = res.numerator
        self._tick_res_den = res.denominator

        origin_str = str(domain_desc.origin) if hasattr(domain_desc, 'origin') else ''
        if origin_str.strip():
            self._origin_epoch = utils.parse_origin(origin_str)
        else:
            self._origin_epoch = None

        self._reader = daq.StreamReader(signal)
        self._last_data_time = time.monotonic()

    # MARK: Polling

    def _on_duration_selected(self, _event=None):
        raw = self._duration_var.get().rstrip('s')
        if float(raw) is not None:
            seconds = float(raw)
        else:
            return
        if seconds <= 0:
            return
        self.WINDOW_SECONDS = seconds
        self._needs_redraw = True

        # Vertical grid lines are created once for the initial WINDOW_SECONDS.
        self._chart_ready = False

    def _recreate_reader(self):
        self._reader = None
        if self._selected_signal is None:
            return
        if not self._is_chartable(self._selected_signal):
            return

        domain_desc = getattr(
            getattr(self._selected_signal, 'domain_signal', None),
            'descriptor', None)
        if domain_desc is None:
            return
        res = getattr(domain_desc, 'tick_resolution', None)
        if res is None:
            return

        self._tick_res_num = res.numerator
        self._tick_res_den = res.denominator
        self._reader = daq.StreamReader(self._selected_signal)
        self._first_tick = None
        self._data.clear()
        self._last_data_time = time.monotonic()
        self._needs_redraw = True

    def _on_log_toggled(self):
        self._needs_redraw = True

    def _schedule_poll(self):
        self._poll_job = self.after(self.POLL_MS, self._poll_tick)

    def _poll_tick(self):
        self._poll_job = None
        if not self.winfo_exists():
            return

        if self._reader is not None:
            try:
                available = self._reader.available_count
                if available > 0:
                    values, domain_ticks = self._reader.read_with_domain(available)
                    self._ingest(values, domain_ticks)
                    self._last_data_time = time.monotonic()
            except RuntimeError as e:
                print(f'[SignalPreview] Reader invalidated, recreating: {e}')
                self._recreate_reader()

        if (self._reader is not None
                and self._last_data_time is not None
                and time.monotonic() - self._last_data_time > self._STALE_TIMEOUT):
            print('[SignalPreview] Reader stale, recreating')
            self._recreate_reader()

        self._poll_job = self.after(self.POLL_MS, self._poll_tick)

    def _ingest(self, values, domain_ticks):
        if values is None or domain_ticks is None:
            return

        n = len(values)
        if n == 0:
            return

        ratio = self._tick_res_num / self._tick_res_den
        if self._first_tick is None:
            self._first_tick = int(domain_ticks[0])

        base = self._first_tick
        buf = self._data

        t_arr = (domain_ticks.astype(np.float64) - base) * ratio
        v_arr = values.astype(np.float64)

        if t_arr is not None:
            target = max(self._TARGET_PPS, int(10_000 / max(self.WINDOW_SECONDS, 0.5)))

            stride = 1
            if n > 2:
                dt = t_arr[-1] - t_arr[0]
                if dt > 0:
                    actual_rate = n / dt
                    if actual_rate > target:
                        stride = max(1, int(actual_rate / target))

            if stride <= 1:
                # Fast path: extend buffer with all points
                for i in range(n):
                    buf.append((t_arr[i], v_arr[i]))
            else:
                usable = (n // stride) * stride
                if usable > 0:
                    t_chunk = t_arr[:usable].reshape(-1, stride)
                    v_chunk = v_arr[:usable].reshape(-1, stride)

                    mn_idx = v_chunk.argmin(axis=1)
                    mx_idx = v_chunk.argmax(axis=1)

                    for ci in range(len(v_chunk)):
                        mni = mn_idx[ci]
                        mxi = mx_idx[ci]
                        if mni == mxi:
                            buf.append((t_chunk[ci, mni], v_chunk[ci, mni]))
                        elif mni < mxi:
                            buf.append((t_chunk[ci, mni], v_chunk[ci, mni]))
                            buf.append((t_chunk[ci, mxi], v_chunk[ci, mxi]))
                        else:
                            buf.append((t_chunk[ci, mxi], v_chunk[ci, mxi]))
                            buf.append((t_chunk[ci, mni], v_chunk[ci, mni]))

                # Handle leftover samples after the last full chunk
                for i in range(usable, n):
                    buf.append((t_arr[i], v_arr[i]))
        else:
            stride = 1
            if n > 2:
                dt = (int(domain_ticks[-1]) - int(domain_ticks[0])) * ratio
                if dt > 0:
                    actual_rate = n / dt
                    target_fb = max(self._TARGET_PPS, int(10_000 / max(self.WINDOW_SECONDS, 0.5)))
                    if actual_rate > target_fb:
                        stride = max(1, int(actual_rate / target_fb))
            for i in range(0, n, stride):
                elapsed = (int(domain_ticks[i]) - base) * ratio
                buf.append((elapsed, float(values[i])))

        self._needs_redraw = True

    # MARK: Drawing

    def _invalidate_chart(self):
        self._chart_ready = False
        self._needs_redraw = True

    def _ensure_chart_items(self):
        c = self._chart
        c.delete('all')

        # Z-order: fill -> grid -> axes -> line -> labels

        self._fill_id = c.create_polygon(
            0, 0, 0, 0, 0, 0, fill=self._FILL, outline='', state='hidden')

        # Horizontal grid
        self._hgrid_ids = []
        self._hgrid_label_ids = []
        for _ in range(5):
            gid = c.create_line(0, 0, 0, 0, fill=self._GRID, dash=(2, 6))
            lid = c.create_text(
                0, 0, text='', fill=self._TEXT, anchor=tk.E, font=self._AXIS_FONT)
            self._hgrid_ids.append(gid)
            self._hgrid_label_ids.append(lid)

        self._vgrid_ids = []
        self._vgrid_label_ids = []
        for _ in range(10):
            vid = c.create_line(0, 0, 0, 0, fill=self._GRID, dash=(2, 6))
            vlid = c.create_text(
                0, 0, text='', fill=self._TEXT, anchor=tk.N, font=self._AXIS_FONT)
            self._vgrid_ids.append(vid)
            self._vgrid_label_ids.append(vlid)

        self._yaxis_id = c.create_line(0, 0, 0, 0, fill=self._AXIS)
        self._xaxis_id = c.create_line(0, 0, 0, 0, fill=self._AXIS)

        # Grid numbers
        self._xlabel_l = c.create_text(
            0, 0, text='', fill=self._TEXT, anchor=tk.SW, font=self._AXIS_FONT)
        self._xlabel_r = c.create_text(
            0, 0, text='', fill=self._TEXT, anchor=tk.SE, font=self._AXIS_FONT)

        self._line_id = c.create_line(
            0, 0, 0, 0, fill=self._LINE, width=1, smooth=False)

        self._badge_bg_id = c.create_rectangle(
            0, 0, 0, 0, fill=self._BG, outline=self._GRID, width=1,
            state='hidden')
        self._badge_id = c.create_text(
            0, 0, text='', fill='#111111', anchor=tk.NE, font=self._VAL_FONT)

        self._nodata_id = c.create_text(
            0, 0, text='', fill=self._TEXT, font=('TkDefaultFont', 10),
            state='hidden')

        self._chart_ready = True

    def _schedule_draw(self):
        self._draw_job = self.after(self.DRAW_MS, self._draw_tick)

    def _draw_tick(self):
        self._draw_job = None
        if not self.winfo_exists():
            return

        if not self._collapsed and self._needs_redraw:
            self._draw_chart()
            self._needs_redraw = False

        self._draw_job = self.after(self.DRAW_MS, self._draw_tick)
        
    def _draw_chart(self):
        if not self._chart_ready:
            self._ensure_chart_items()

        c = self._chart
        w = c.winfo_width()
        h = c.winfo_height()
        if w < 20 or h < 20:
            return

        ml, mr, mt, mb = 65, 12, 10, 22
        pw = w - ml - mr
        ph = h - mt - mb
        if pw < 10 or ph < 10:
            return

        buf = self._data
        has_data = len(buf) > 0

        if not has_data:
            c.coords(self._nodata_id, w // 2, h // 2)
            c.itemconfig(self._nodata_id, text='No data', state='normal')
            c.itemconfig(self._line_id, state='hidden')
            c.itemconfig(self._fill_id, state='hidden')
            c.itemconfig(self._badge_id, state='hidden')
            c.itemconfig(self._badge_bg_id, state='hidden')
            return

        c.itemconfig(self._nodata_id, state='hidden')

        # Visible window
        t_latest = buf[-1][0]
        t_earliest = t_latest - self.WINDOW_SECONDS

        visible = [(t, v) for t, v in buf if t >= t_earliest]
        
        if len(visible) == 0:
            c.coords(self._nodata_id, w // 2, h // 2)
            c.itemconfig(self._nodata_id, text='Waiting...', state='normal')
            c.itemconfig(self._line_id, state='hidden')
            c.itemconfig(self._fill_id, state='hidden')
            c.itemconfig(self._badge_id, state='hidden')
            c.itemconfig(self._badge_bg_id, state='hidden')
            return

        # Y range with breathing room
        vals = [v for _, v in visible]
        v_lo, v_hi = min(vals), max(vals)

        use_log = self._log_var.get()

        if use_log:
            abs_max = max(abs(v) for v in vals)
            symlog_thresh = max(abs_max * 0.01, 1e-10)

            def symlog(v):
                return math.copysign(math.log10(1.0 + abs(v) / symlog_thresh), v)

            symlog_vals = [symlog(v) for v in vals]
            sl_lo = min(symlog_vals)
            sl_hi = max(symlog_vals)
            if sl_lo == sl_hi:
                sl_lo -= 1.0
                sl_hi += 1.0
            else:
                pad = (sl_hi - sl_lo) * 0.05
                sl_lo -= pad
                sl_hi += pad
            sl_span = sl_hi - sl_lo
        else:
            if v_lo == v_hi:
                v_lo -= 1.0
                v_hi += 1.0
            else:
                pad = (v_hi - v_lo) * 0.08
                v_lo -= pad
                v_hi += pad

        t_span = self.WINDOW_SECONDS
        v_span = v_hi - v_lo
        
        def xpx(t):
            return ml + (t - t_earliest) / t_span * pw

        if use_log:
            def ypx(v):
                return mt + (1.0 - (symlog(v) - sl_lo) / sl_span) * ph
        else:
            def ypx(v):
                return mt + (1.0 - (v - v_lo) / v_span) * ph

        # Horizontal grid + Y labels
        unit_suffix = f' {self._unit_str}' if self._unit_str else ''
        for i in range(5):
            gy = mt + ph * i / 4
            c.coords(self._hgrid_ids[i], ml, gy, ml + pw, gy)
            if use_log:
                sl_val = sl_hi - (sl_hi - sl_lo) * i / 4
                gv = math.copysign(
                    symlog_thresh * (10.0 ** abs(sl_val) - 1.0), sl_val)
            else:
                gv = v_hi - v_span * i / 4
            c.coords(self._hgrid_label_ids[i], ml - 4, gy)
            c.itemconfig(self._hgrid_label_ids[i],
                         text=self._fmt(gv) + unit_suffix)

        _nice = [0.1, 0.2, 0.5, 1, 2, 5, 10, 15, 30, 60]
        grid_interval = _nice[-1]
        for ni in _nice:
            if self.WINDOW_SECONDS / ni <= 7:
                grid_interval = ni
                break

        first_mark = math.ceil(t_earliest / grid_interval) * grid_interval
        slot = 0
        t_mark = first_mark
        
        while t_mark < t_latest and slot < len(self._vgrid_ids):
            vx = xpx(t_mark)
            c.coords(self._vgrid_ids[slot], vx, mt, vx, mt + ph)
            secs_ago = t_latest - t_mark
            if grid_interval >= 1:
                label = f'-{secs_ago:.0f}s'
            else:
                label = f'-{secs_ago:.1f}s'
            c.coords(self._vgrid_label_ids[slot], vx, mt + ph + 3)
            c.itemconfig(self._vgrid_label_ids[slot], text=label, state='normal')
            slot += 1
            t_mark += grid_interval

        # Hide unused grid slots
        for i in range(slot, len(self._vgrid_ids)):
            c.itemconfig(self._vgrid_ids[i], state='hidden')
            c.itemconfig(self._vgrid_label_ids[i], state='hidden')

        # Axes
        c.coords(self._yaxis_id, ml, mt, ml, mt + ph)
        c.coords(self._xaxis_id, ml, mt + ph, ml + pw, mt + ph)

        c.coords(self._xlabel_l, ml, h - 2)
        c.itemconfig(self._xlabel_l, text=f'-{self.WINDOW_SECONDS:.0f}s')
        c.coords(self._xlabel_r, ml + pw, h - 2)
        c.itemconfig(self._xlabel_r, text='now')

        # Min/max envelope downsampl
        if len(visible) > pw * 3:
            n_buckets = max(pw, 4)
            bucket_w = t_span / n_buckets
            envelope = []
            bi = 0
            b_edge = t_earliest + bucket_w
            b_min = b_max = None

            for t, v in visible:
                while t >= b_edge and bi < n_buckets - 1:
                    if b_min is not None:
                        if b_min[0] <= b_max[0]:
                            envelope.append(b_min)
                            if b_max != b_min:
                                envelope.append(b_max)
                        else:
                            envelope.append(b_max)
                            if b_min != b_max:
                                envelope.append(b_min)
                    b_min = b_max = None
                    bi += 1
                    b_edge = t_earliest + (bi + 1) * bucket_w

                if b_min is None or v < b_min[1]:
                    b_min = (t, v)
                if b_max is None or v > b_max[1]:
                    b_max = (t, v)

            if b_min is not None:
                if b_min[0] <= b_max[0]:
                    envelope.append(b_min)
                    if b_max != b_min:
                        envelope.append(b_max)
                else:
                    envelope.append(b_max)
                    if b_min != b_max:
                        envelope.append(b_min)
            visible = envelope

        # Filled area under curve
        bottom_y = mt + ph
        if len(visible) >= 2:
            fill_coords = [xpx(visible[0][0]), bottom_y]
            for t, v in visible:
                fill_coords.extend([xpx(t), ypx(v)])
            fill_coords.extend([xpx(visible[-1][0]), bottom_y])
            c.coords(self._fill_id, *fill_coords)
            c.itemconfig(self._fill_id, state='normal')
        else:
            c.itemconfig(self._fill_id, state='hidden')

        # Data line (1 px, no smoothing)
        if len(visible) >= 2:
            line_coords = []
            for t, v in visible:
                line_coords.extend([xpx(t), ypx(v)])
            c.coords(self._line_id, *line_coords)
            c.itemconfig(self._line_id, state='normal')
        elif len(visible) == 1:
            px, py = xpx(visible[0][0]), ypx(visible[0][1])
            c.coords(self._line_id, px, py, px + 1, py)
            c.itemconfig(self._line_id, state='normal')

        badge_text = self._fmt(visible[-1][1])
        if self._unit_str:
            badge_text += f' {self._unit_str}'
        bx = ml + pw - 4
        by = mt + 4
        c.coords(self._badge_id, bx, by)
        c.itemconfig(self._badge_id, text=badge_text, state='normal')
        c.update_idletasks()
        bb = c.bbox(self._badge_id)
        if bb:
            c.coords(self._badge_bg_id, bb[0] - 3, bb[1] - 1, bb[2] + 3, bb[3] + 1)
            c.itemconfig(self._badge_bg_id, state='normal')

    @staticmethod
    def _fmt(v):
        if v != 0 and abs(v) < 0.001:
            return f'{v:.3e}'
        if abs(v) >= 100_000:
            return f'{v:.1f}'
        if v == int(v):
            return str(int(v))
        return f'{v:.4g}'

    def _toggle(self):
        self._collapsed = not self._collapsed
        if self._collapsed:
            self._content_frame.pack_forget()
            self._arrow_label.config(text='\u25B2 Signal Preview')
        else:
            self._content_frame.pack(
                fill=tk.BOTH, expand=True, after=self._toggle_frame)
            self._arrow_label.config(text='\u25BC Signal Preview')
            self._needs_redraw = True
            
    def _on_destroy(self, event):
        if event.widget is not self:
            return

        for job_attr in ('_poll_job', '_draw_job'):
            job_id = getattr(self, job_attr, None)
            if job_id:
                self.after_cancel(job_id)
                setattr(self, job_attr, None)

        if self._chart.winfo_exists():
            self._chart.delete('all')

        self._chart_ready = False
        self._reader = None