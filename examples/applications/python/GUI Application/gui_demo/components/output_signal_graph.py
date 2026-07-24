import math
import re
import tkinter as tk
from tkinter import ttk
from collections import deque
import numpy as np
from tkinter import font as tkfont

import opendaq as daq

from .. import utils

class OutputSignalGraph(ttk.Frame):
    DEFAULT_WINDOW_SECONDS = 0.2
    TARGET_POINTS_PER_FRAME = 50_000
    MAX_BUFFER_SIZE = int(TARGET_POINTS_PER_FRAME * 1.5)
    POLL_MS = 33          # drain the reader this often
    _TARGET_PPS = 2000
    DRAW_MS = 66         # repaint the chart this often
    CANVAS_HEIGHT = 180

    _BG      = '#ffffff'
    _LINE    = '#1a6dcc'
    _GRID    = '#e4e4e4'
    _TEXT    = '#444444'
    _AXIS    = '#999999'

    _AXIS_FONT = ('TkFixedFont', 7)
    _VAL_FONT  = ('TkFixedFont', 10, 'bold')
    _DURATION_RE = re.compile(r'^\s*(\d+(?:\.\d+)?|\.\d+)\s*s?\s*$', re.IGNORECASE)

    def __init__(self, parent, node, context=None, duration_var=None, **kwargs):
        super().__init__(parent, **kwargs)
        self.node = node
        self.context = context
        self._external_duration_var = duration_var

        self._reader = None
        self._selected_signal = None
        self._selected_descriptor = None
        self._selected_domain_descriptor = None

        self._poll_job = None
        self._draw_job = None

        # Buffer of (elapsed_seconds, value) pairs.
        self._data = deque(maxlen=self.MAX_BUFFER_SIZE)
        self._window_seconds = self.DEFAULT_WINDOW_SECONDS
        self._window_size_in_samples = None

        self._tick_res_num = None
        self._tick_res_den = None
        self._dt = None
        self._sr = None
        self._first_tick = None
        # running sample index: time is computed from this and the sample rate
        # (read() returns values only; the domain signal is not read)
        self._sample_pos = 0

        # name -> ISignal for chartable signals
        self._eligible = {}
        self._unit_str = ''
        self._needs_redraw = True
        self._chart_ready = False
        self._px_ml = 65

        # Build widgets
        self._build_content()
        self._content_frame.pack(fill=tk.BOTH, expand=True)
        self._populate_dropdown()
        self._schedule_poll()
        self._schedule_draw()
        self.bind('<Destroy>', self._on_destroy)

    def _build_content(self):
        self._content_frame = ttk.Frame(self)

        self._duration_presets = [0.01, 0.05, 0.1, 0.2, 0.5, 1]
        if self._external_duration_var is not None:
            self._duration_var = self._external_duration_var
        else:
            self._duration_var = tk.StringVar(value=f'{self._window_seconds:g}s')
        self._dur_edit_entry = None

        self._scale_var = tk.StringVar(value='Linear')

        controls = ttk.Frame(self._content_frame)
        controls.pack(fill=tk.X, padx=(2, 12), pady=(2, 0))

        self._scale_header = ttk.Label(controls, text='Scale')
        self._scale_header.pack(side=tk.LEFT)
        self._scale_cb = ttk.Combobox(
            controls, textvariable=self._scale_var, state='readonly',
            values=['Linear', 'Log'], width=7)
        self._scale_cb.pack(side=tk.LEFT)
        self._scale_cb.bind('<<ComboboxSelected>>', self._on_scale_changed)

        # Hidden until explicit signals are handled
        self._set_scale_visible(False)

        self._signal_var = tk.StringVar()
        self._dropdown = None

        # Chart canvas
        self._chart = tk.Canvas(
            self._content_frame, bg=self._BG,
            height=140, highlightthickness=0)
        self._chart.pack(fill=tk.BOTH, expand=True, padx=(2, 0), pady=(4, 6))

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
            domain_signal = signal.domain_signal
            if domain_signal is None:
                continue

            if not self._is_chartable(signal.descriptor, domain_signal.descriptor):   
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
        names.insert(0, 'None')

        if self._dropdown is not None:
            self._dropdown['values'] = names

        if len(names) > 1:
            self._signal_var.set(names[1])
            self._on_signal_selected()
        else:
            self._deselect_signal()
        
    def _deselect_signal(self):
        self._signal_var.set('None')
        self._on_signal_selected()

    @staticmethod
    def _is_chartable(desc, domain_desc):
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

        if domain_desc is None:
            return False
        
        unit = getattr(domain_desc, 'unit', None)
        symbol = getattr(unit, 'symbol', None)
        if str(symbol) != 's':
            return False

        if getattr(domain_desc, 'tick_resolution', None) is None:
            return False
        
        data_rule = getattr(domain_desc, 'rule', None)
        if data_rule is None or data_rule.type != daq.DataRuleType.Linear:
            return False
    
        return True

    # MARK: Signal selection

    def _on_signal_selected(self, _event=None):
        self._reader = None
        self._selected_signal = None
        self._selected_descriptor = None
        self._selected_domain_descriptor = None

        self._data.clear()
        self._first_tick = None
        self._sample_pos = 0
        self._needs_redraw = True

        self._unit_str = ''
        self._tick_res_num = None
        self._tick_res_den = None
        self._window_size_in_samples = None
        self._dt = None

        name = self._signal_var.get()
        signal = self._eligible.get(name)
        if signal is None:
            self._set_scale_visible(False)
            return

        self._selected_signal = signal
        # Linear/Log scale toggle is relevant once a signal is charted.
        self._set_scale_visible(True)
        self._reader = daq.StreamReader(signal, skip_events=False)

    # MARK: Polling

    def _on_duration_committed(self, event=None):
        text = self._duration_var.get()
        match = self._DURATION_RE.match(text)
        if match is None:
            self._duration_var.set(f'{self.DEFAULT_WINDOW_SECONDS:g}s')
            return

        seconds = float(match.group(1))
        seconds = max(0.01, min(10.0, seconds))
        self._duration_var.set(f'{seconds:g}s')

        if seconds == self._window_seconds:
            return

        self._window_seconds = seconds
        self._needs_redraw = True
        self._chart_ready = False

    def _begin_duration_edit(self, event=None):
        if self._dur_edit_entry is not None:
            return

        canvas = self._chart
        bb = canvas.bbox(self._xlabel_l)
        if bb is None:
            return

        # Position the entry right over the label
        x, y = bb[0], bb[1]
        w = max(bb[2] - bb[0] + 16, 50)
        h = bb[3] - bb[1] + 4

        entry = tk.Entry(canvas, font=self._AXIS_FONT, width=8,
                         justify=tk.LEFT)
        # Pre-fill with the raw number (no dash, no 's')
        entry.insert(0, f'{self._window_seconds:g}')
        entry.select_range(0, tk.END)
        entry.place(x=x, y=y, width=w, height=h)
        entry.focus_set()
        self._dur_edit_entry = entry

        entry.bind('<Return>', self._commit_duration_edit)
        entry.bind('<FocusOut>', self._commit_duration_edit)
        entry.bind('<Escape>', self._cancel_duration_edit)
        
    def _on_dur_hover_enter(self, event=None):
        self._chart.config(cursor='xterm')
        self._chart.itemconfig(
            self._xlabel_l,
            font=(*self._AXIS_FONT, 'underline'),
            fill='#1a6dcc')

    def _on_dur_hover_leave(self, event=None):
        self._chart.config(cursor='')
        self._chart.itemconfig(
            self._xlabel_l,
            font=self._AXIS_FONT,
            fill=self._TEXT)

    def _commit_duration_edit(self, event=None):
        entry = self._dur_edit_entry
        if entry is None:
            return
        raw = entry.get().strip()
        entry.destroy()
        self._dur_edit_entry = None

        self._duration_var.set(raw)
        self._on_duration_committed()

    def _cancel_duration_edit(self, event=None):
        if self._dur_edit_entry is not None:
            self._dur_edit_entry.destroy()
            self._dur_edit_entry = None

    def _recreate_reader(self):
        self._reader = None
        if self._selected_signal is None:
            return
        
        domain_signal = self._selected_signal.domain_signal
        if domain_signal is None:
            return

        self._reader = daq.StreamReader(
            self._selected_signal, skip_events=False)
        
        self._unit_str = ''
        self._tick_res_num = None
        self._tick_res_den = None
        self._window_size_in_samples = None
        self._dt = None

        self._first_tick = None
        self._sample_pos = 0
        self._data.clear()
        self._needs_redraw = True

    def _set_scale_visible(self, visible):
        if visible:
            self._scale_header.pack(side=tk.LEFT)
            self._scale_cb.pack(side=tk.LEFT, padx=(4, 0))
        else:
            self._scale_header.pack_forget()
            self._scale_cb.pack_forget()

    @staticmethod
    def _is_2d_vector_signal(signal):
        desc = getattr(signal, 'descriptor', None)
        dims = getattr(desc, 'dimensions', None) if desc else None
        if not dims or len(dims) != 1:
            return False
        size = getattr(dims[0], 'size', None)
        return size == 2

    def _on_scale_changed(self, _event=None):
        self._needs_redraw = True
        self._chart.focus_set()
        
    def _schedule_poll(self):
        self._poll_job = self.after(self.POLL_MS, self._poll_tick)

    def _poll_tick(self):
        self._poll_job = None
        if not self.winfo_exists():
            return

        if self._reader is not None:
            self._drain_reader()

        self._poll_job = self.after(self.POLL_MS, self._poll_tick)

    def _drain_reader(self):
        while True:
            try:
                available = self._reader.available_count
            except RuntimeError as e:
                print(f'[SignalPreview] Reader query failed: {e}')
                self._recreate_reader()
                return

            read_count = min(available, self.MAX_BUFFER_SIZE)

            # Explicit signals: read values only and compute the domain time
            # from the sample rate, instead of reading the domain ticks.
            try:
                values, status = self._reader.read(
                    read_count, return_status=True)
            except RuntimeError as e:
                print(f'[SignalPreview] Read failed: {e}')
                self._recreate_reader()
                return

            if len(values) > 0:
                self._ingest(values)

            if status.read_status == daq.ReadStatus.Event:
                self._handle_event_packet(status.event_packet)
                continue

            if status.read_status == daq.ReadStatus.Ok and len(values) == 0:
                return
            
    @staticmethod
    def _is_null_descriptor(desc):
        sample_type = getattr(desc, 'sample_type', None)
        if sample_type is None:
            return True
        name = getattr(sample_type, 'name', str(sample_type))
        return name == 'Null'

    def _apply_descriptors(self, data_descriptor, domain_descriptor):
        if data_descriptor is not None:
            self._selected_descriptor = daq.IDataDescriptor.cast_from(data_descriptor)
            if self._is_null_descriptor(self._selected_descriptor):
                self._unit_str = ''
            else:
                unit = getattr(self._selected_descriptor, 'unit', None)
                symbol = (getattr(unit, 'symbol', None)
                          if unit is not None else None)
                self._unit_str = str(symbol) if symbol is not None else ''

        if domain_descriptor is not None:
            self._selected_domain_descriptor = daq.IDataDescriptor.cast_from(domain_descriptor)
            
            if self._is_null_descriptor(self._selected_domain_descriptor):
                self._tick_res_num = None
                self._tick_res_den = None
                self._window_size_in_samples = None
                self.dt = None
            else:
                res = self._selected_domain_descriptor.tick_resolution
                self._tick_res_num = res.numerator
                self._tick_res_den = res.denominator

                # Calculate buffer size based on tick resolution + delta and desired window duration
                rule = self._selected_domain_descriptor.rule
                self._dt = rule.parameters['delta']
                self._sr = self._tick_res_den / self._tick_res_num / self._dt

    def _handle_event_packet(self, event_packet):
        event_id = event_packet.event_id

        self._needs_redraw = True
        self._data.clear()
        self._first_tick = None
        self._sample_pos = 0
        self._chart_ready = False

        if event_id != 'DATA_DESCRIPTOR_CHANGED':
            return
        
        params = event_packet.parameters
        data_desc = params['DataDescriptor']
        domain_desc = params['DomainDataDescriptor']

        self._apply_descriptors(data_desc, domain_desc)
        if not self._is_chartable(self._selected_descriptor, self._selected_domain_descriptor):
            self._deselect_signal()

    def _ingest(self, values):
        if values is None:
            return

        n = len(values)
        if n == 0:
            return

        if (self._tick_res_num is None or self._tick_res_den is None
                or self._dt is None or self._sr is None):
            return

        # Seconds per sample from the (linear) domain: tick_resolution * delta.
        # The domain is not read; time comes from the running sample index.
        dt_seconds = (self._tick_res_num / self._tick_res_den) * self._dt

        idx = self._sample_pos + np.arange(n, dtype=np.float64)
        t_arr = idx * dt_seconds
        v_arr = values.astype(np.float64)
        self._sample_pos += n

        buf = self._data

        actual_points_per_frame = int(self._window_seconds * self._sr)

        stride = 1
        if actual_points_per_frame > self.TARGET_POINTS_PER_FRAME:
            stride = math.ceil(actual_points_per_frame / self.TARGET_POINTS_PER_FRAME)

        for i in range(0, n, stride):
            buf.append((t_arr[i], v_arr[i]))

        self._needs_redraw = True

    # MARK: Drawing

    def _invalidate_chart(self):
        self._chart_ready = False
        self._needs_redraw = True

    def _ensure_chart_items(self):
        canvas = self._chart
        canvas.delete('all')

        # Z-order: grid -> axes -> line -> labels

        # Horizontal grid
        self._hgrid_ids = []
        self._hgrid_label_ids = []
        for _ in range(5):
            gid = canvas.create_line(0, 0, 0, 0, fill=self._GRID, dash=(2, 6))
            lid = canvas.create_text(
                0, 0, text='', fill=self._TEXT, anchor=tk.E, font=self._AXIS_FONT)
            self._hgrid_ids.append(gid)
            self._hgrid_label_ids.append(lid)

        self._vgrid_ids = []
        self._vgrid_label_ids = []
        for _ in range(10):
            vid = canvas.create_line(0, 0, 0, 0, fill=self._GRID, dash=(2, 6))
            vlid = canvas.create_text(
                0, 0, text='', fill=self._TEXT, anchor=tk.N, font=self._AXIS_FONT)
            self._vgrid_ids.append(vid)
            self._vgrid_label_ids.append(vlid)

        self._yaxis_id = canvas.create_line(0, 0, 0, 0, fill=self._AXIS)
        self._xaxis_id = canvas.create_line(0, 0, 0, 0, fill=self._AXIS)

        # Grid numbers
        self._xlabel_l = canvas.create_text(
            0, 0, text='', fill=self._TEXT, anchor=tk.NW, font=self._AXIS_FONT,
            tags=('dur_label',))
        canvas.tag_bind('dur_label', '<Double-1>', self._begin_duration_edit)
        canvas.tag_bind('dur_label', '<Enter>', self._on_dur_hover_enter)
        canvas.tag_bind('dur_label', '<Leave>', self._on_dur_hover_leave)
        self._xlabel_r = canvas.create_text(
            0, 0, text='', fill=self._TEXT, anchor=tk.NE, font=self._AXIS_FONT)

        self._line_id = canvas.create_line(
            0, 0, 0, 0, fill=self._LINE, width=1, smooth=False)

        self._nodata_id = canvas.create_text(
            0, 0, text='', fill=self._TEXT, font=('TkDefaultFont', 10),
            state='hidden')

        self._chart_ready = True

    def _schedule_draw(self):
        draw_interval = max(16, min(self.DRAW_MS, int(self.DEFAULT_WINDOW_SECONDS * 500)))
        self._draw_job = self.after(draw_interval, self._draw_tick)

    def _draw_tick(self):
        self._draw_job = None
        if not self.winfo_exists():
            return

        if self._needs_redraw:
            self._draw_chart()
            self._needs_redraw = False

        self._draw_job = self.after(self.DRAW_MS, self._draw_tick)
        
    def _draw_chart(self):
        if not self._chart_ready:
            self._ensure_chart_items()

        canvas = self._chart
        canvas_width = canvas.winfo_width()
        canvas_height = canvas.winfo_height()
        if canvas_width < 20 or canvas_height < 20:
            return

        margin_right = 12
        margin_top = 10
        margin_bottom = 22
        plot_height = canvas_height - margin_top - margin_bottom
        if plot_height < 10:
            return

        buf = self._data
        has_data = len(buf) > 0

        if not has_data:
            canvas.coords(self._nodata_id, canvas_width // 2, canvas_height // 2)
            canvas.itemconfig(self._nodata_id, text='None', state='normal')
            canvas.itemconfig(self._line_id, state='hidden')
            return

        canvas.itemconfig(self._nodata_id, state='hidden')

        # Visible window
        t_latest = buf[-1][0]
        t_earliest = t_latest - self._window_seconds

        visible = [(t, v) for t, v in buf if t >= t_earliest]

        if len(visible) == 0:
            canvas.coords(self._nodata_id, canvas_width // 2, canvas_height // 2)
            canvas.itemconfig(self._nodata_id, text='Waiting...', state='normal')
            canvas.itemconfig(self._line_id, state='hidden')
            return

        # Y range from a stabilized window so a brief spike does not
        # immediately rescale the whole chart.
        range_earliest = t_latest - max(self._window_seconds, 0.5)
        range_vals = [v for t, v in buf if t >= range_earliest]
        v_lo, v_hi = min(range_vals), max(range_vals)

        use_log = self._scale_var.get() == 'Log'

        if use_log:
            abs_max = max(abs(v) for v in range_vals)
            symlog_thresh = max(abs_max * 0.01, 1e-10)

            def symlog(v):
                return math.copysign(math.log10(1.0 + abs(v) / symlog_thresh), v)

            symlog_vals = [symlog(v) for v in range_vals]
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

        t_span = self._window_seconds
        v_span = v_hi - v_lo

        unit_suffix = f' {self._unit_str}' if self._unit_str else ''
        y_labels = []
        for i in range(5):
            if use_log:
                sl_val = sl_hi - (sl_hi - sl_lo) * i / 4
                gv = math.copysign(
                    symlog_thresh * (10.0 ** abs(sl_val) - 1.0), sl_val)
            else:
                gv = v_hi - v_span * i / 4
            y_labels.append(self._fmt(gv) + unit_suffix)

        label_font = tkfont.Font(font=self._AXIS_FONT)
        max_label_w = max(label_font.measure(t) for t in y_labels)
        # Round left margin up to not nudge
        margin_left = -(-(max_label_w + 8) // 4) * 4

        plot_width = canvas_width - margin_left - margin_right
        if plot_width < 10:
            return

        def xpx(t):
            return margin_left + (t - t_earliest) / t_span * plot_width

        if use_log:
            def ypx(v):
                return margin_top + (1.0 - (symlog(v) - sl_lo) / sl_span) * plot_height
        else:
            def ypx(v):
                return margin_top + (1.0 - (v - v_lo) / v_span) * plot_height

        # Horizontal grid + Y labels
        for i in range(5):
            gy = margin_top + plot_height * i / 4
            canvas.coords(self._hgrid_ids[i],
                        margin_left, gy, margin_left + plot_width, gy)
            canvas.coords(self._hgrid_label_ids[i], margin_left - 4, gy)
            canvas.itemconfig(self._hgrid_label_ids[i], text=y_labels[i])

        # Axes
        canvas.coords(self._yaxis_id,
                    margin_left, margin_top,
                    margin_left, margin_top + plot_height)
        canvas.coords(self._xaxis_id,
                    margin_left, margin_top + plot_height,
                    margin_left + plot_width, margin_top + plot_height)

        _nice = [0.001, 0.002, 0.005, 0.01, 0.02, 0.05, 0.1, 0.2, 0.5,
                1, 2, 5, 10, 15, 30, 60]
        grid_interval = _nice[-1]
        for ni in _nice:
            if self._window_seconds / ni <= 7:
                grid_interval = ni
                break

        if grid_interval >= 1:
            label_fmt = '-{:.0f}s'
        elif grid_interval >= 0.1:
            label_fmt = '-{:.1f}s'
        elif grid_interval >= 0.01:
            label_fmt = '-{:.2f}s'
        else:
            label_fmt = '-{:.3f}s'

        # Single y for every bottom label so the row reads as one axis.
        label_y = margin_top + plot_height + 3

        slot = 0
        k = 1

        while (k * grid_interval) < self._window_seconds - 1e-9 \
                and slot < len(self._vgrid_ids):
            secs_ago = k * grid_interval
            vx = margin_left + plot_width - (secs_ago / self._window_seconds) * plot_width
            canvas.coords(self._vgrid_ids[slot], vx, margin_top, vx, margin_top + plot_height)
            canvas.itemconfig(self._vgrid_ids[slot], state='normal')
            
            canvas.coords(self._vgrid_label_ids[slot], vx, label_y)
            canvas.itemconfig(self._vgrid_label_ids[slot],
                         text=label_fmt.format(secs_ago), state='normal')
            slot += 1
            k += 1

        # Hide unused grid slots
        for i in range(slot, len(self._vgrid_ids)):
            canvas.itemconfig(self._vgrid_ids[i], state='hidden')
            canvas.itemconfig(self._vgrid_label_ids[i], state='hidden')

        # Bring your missing edge labels back on screen using label_y
        canvas.coords(self._xlabel_l, margin_left, label_y)
        canvas.itemconfig(self._xlabel_l,
                     text=label_fmt.format(self._window_seconds))
                     
        canvas.coords(self._xlabel_r, margin_left + plot_width, label_y)
        canvas.itemconfig(self._xlabel_r, text='0s')

        if len(visible) > plot_width * 3:
            n_buckets = max(plot_width, 4)
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

        # Data line (1 px, no smoothing)
        if len(visible) >= 2:
            line_coords = []
            for t, v in visible:
                line_coords.extend([xpx(t), ypx(v)])
            canvas.coords(self._line_id, *line_coords)
            canvas.itemconfig(self._line_id, state='normal')
        elif len(visible) == 1:
            px, py = xpx(visible[0][0]), ypx(visible[0][1])
            canvas.coords(self._line_id, px, py, px + 1, py)
            canvas.itemconfig(self._line_id, state='normal')

    @staticmethod
    def _fmt(v):
        if v != 0 and abs(v) < 0.001:
            return f'{v:.3e}'
        if abs(v) >= 100_000:
            return f'{v:.3e}'
        if v == int(v):
            return str(int(v))
        return f'{v:.4g}'
            
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