import math
import re
import tkinter as tk
from tkinter import ttk
from collections import deque, namedtuple
import numpy as np
from tkinter import font as tkfont

import opendaq as daq

# Maps a value onto 0..1 (0 at the bottom of the plot) and lists the values that
# get a gridline. `engineering` picks how they are labelled: a log axis spans
# decades so every tick carries its own metric prefix, while a linear one shares
# one prefix across the whole axis.
_YAxis = namedtuple('_YAxis', ('norm', 'ticks', 'engineering'))

# Same for the horizontal axis, plus the text to put under each gridline.
_XAxis = namedtuple('_XAxis', ('norm', 'ticks', 'label'))

# Everything one repaint needs, whichever way the signal is being charted.
_Frame = namedtuple('_Frame', ('points', 'y_range', 'x_axis', 'edge_labels'))


class OutputSignalGraph(ttk.Frame):
    DEFAULT_WINDOW_SECONDS = 0.2
    TARGET_POINTS_PER_FRAME = 50_000
    MAX_BUFFER_SIZE = int(TARGET_POINTS_PER_FRAME * 1.5)
    POLL_MS = 33          # drain the reader this often
    DRAW_MS = 66         # repaint the chart this often

    _NUMERIC_SAMPLE_TYPES = frozenset((
        daq.SampleType.Float32, daq.SampleType.Float64,
        daq.SampleType.Int8, daq.SampleType.Int16,
        daq.SampleType.Int32, daq.SampleType.Int64,
        daq.SampleType.UInt8, daq.SampleType.UInt16,
        daq.SampleType.UInt32, daq.SampleType.UInt64,
    ))

    MAX_HGRID = 9         # horizontal gridline slots; a log axis needs one per decade
    LOG_MAX_DECADES = 6   # widest log span, so one near-zero sample cannot stretch it
    STEP_MAX_POINTS = 25  # sample-and-hold fallback when the rate is unknown

    # Sampled slower than this and the signal is a monitored value rather than
    # a waveform, so it holds between samples instead of ramping.
    SPARSE_SAMPLE_SECONDS = 0.05

    DURATION_PRESETS = (0.01, 0.05, 0.1, 0.2, 0.5, 1, 2, 5, 10, 30, 60)
    MAX_WINDOW_SECONDS = 60.0
    AUTO_WINDOW_SAMPLES = 10  # samples the opening window should be able to show

    _TIME_GRID_INTERVALS = (0.001, 0.002, 0.005, 0.01, 0.02, 0.05, 0.1, 0.2,
                            0.5, 1, 2, 5, 10, 15, 30, 60)

    # Metric prefixes by power of ten, so ticks read '742.4 MB' instead of
    # '7.424e+05 kB'. The unit may already carry one, so they compose.
    _SI_PREFIXES = {-15: 'f', -12: 'p', -9: 'n', -6: 'µ', -3: 'm', 0: '',
                    3: 'k', 6: 'M', 9: 'G', 12: 'T', 15: 'P'}
    _PREFIX_EXPONENTS = {'f': -15, 'p': -12, 'n': -9, 'µ': -6, 'u': -6,
                         'm': -3, 'k': 3, 'M': 6, 'G': 9, 'T': 12, 'P': 15}
    # Two-letter units opening with a prefix letter that are whole units, so
    # 'Pa' does not read as peta-a. They can still take a prefix of their own.
    _UNSPLITTABLE_UNITS = frozenset(('Pa', 'cd', 'ft', 'mi', 'nt'))
    # Non-metric units, which take no prefix at all - there is no kilominute.
    _UNPREFIXABLE_UNITS = frozenset(('min', 'h', 'd', 'ft', 'in', 'mi', 'yd'))

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
        # A window picked for the signal is only a starting point; once the
        # user names a duration it is theirs and nothing overrides it.
        self._duration_user_set = False
        self._auto_window_applied = False

        # Domain timing. _dt is the linear rule's tick delta and stays None for
        # explicit and constant domains; _sr is then estimated from the ticks.
        self._tick_res_num = None
        self._tick_res_den = None
        self._tick_delta = None
        self._sample_rate = None
        self._first_tick = None

        # Set only for vector signals, which are charted as a spectrum: the
        # latest sample against the positions its dimension spans.
        self._bin_positions = None
        self._bin_unit = ''
        self._spectrum = None

        # name -> ISignal for chartable signals
        self._eligible = {}
        self._unit_str = ''
        self._needs_redraw = True
        self._chart_ready = False
        self._log_unavailable = False

        # Build widgets
        self._build_content()
        self._content_frame.pack(fill=tk.BOTH, expand=True)
        self._populate_dropdown()
        self._schedule_poll()
        self._schedule_draw()
        self.bind('<Destroy>', self._on_destroy)

    def _build_content(self):
        self._content_frame = ttk.Frame(self)

        if self._external_duration_var is not None:
            self._duration_var = self._external_duration_var
        else:
            self._duration_var = tk.StringVar(value=f'{self._window_seconds:g}s')
        self._dur_edit_entry = None

        self._scale_var = tk.StringVar(value='Linear')

        self._signal_var = tk.StringVar()
        self._dropdown = None

        # Chart canvas
        self._chart = tk.Canvas(
            self._content_frame, bg=self._BG,
            height=140, highlightthickness=0)
        self._chart.pack(fill=tk.BOTH, expand=True, padx=(2, 0), pady=(4, 6))

        # Built here but packed only while it applies, so it reads as belonging
        # to the axis it scales rather than as a strip above the plot.
        self._scale_controls = ttk.Frame(self._content_frame)
        ttk.Label(self._scale_controls, text='Scale').pack(side=tk.LEFT)
        self._scale_cb = ttk.Combobox(
            self._scale_controls, textvariable=self._scale_var,
            state='readonly', values=['Linear', 'Log'], width=7)
        self._scale_cb.pack(side=tk.LEFT, padx=(4, 0))
        self._scale_cb.bind('<<ComboboxSelected>>', self._on_scale_changed)
        self._refresh_scale_control()

        self._chart.bind('<Configure>', lambda _e: self._invalidate_chart())
        self._redirect_mousewheel(self)

    # Tk sends the wheel only to the widget under the pointer and never bubbles
    # it, so the chart would eat the scroll of the page it sits on. Pass it up
    # instead, then break so the Scale box does not change value under it.
    def _redirect_mousewheel(self, widget):
        for event in ('<MouseWheel>', '<Button-4>', '<Button-5>'):
            widget.bind(event, lambda e, seq=event: self._forward_wheel(e, seq))
        for child in widget.winfo_children():
            self._redirect_mousewheel(child)

    def _forward_wheel(self, event, sequence):
        ancestor = self.master
        while ancestor is not None:
            if ancestor.bind(sequence):
                if sequence == '<MouseWheel>':
                    ancestor.event_generate(sequence, delta=event.delta)
                else:
                    ancestor.event_generate(sequence)
                break
            ancestor = getattr(ancestor, 'master', None)
        return 'break'

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

    @classmethod
    def _is_chartable(cls, desc, domain_desc):
        if desc is None:
            return False

        if getattr(desc, 'sample_type', None) not in cls._NUMERIC_SAMPLE_TYPES:
            return False

        # One dimension means each sample is a vector along that axis, drawn as
        # a spectrum. More than one has no layout here.
        dims = getattr(desc, 'dimensions', None)
        if dims and (len(dims) > 1 or cls._vector_dimension(desc) is None):
            return False

        fields = getattr(desc, 'struct_fields', None)
        if fields and len(fields) > 0:
            return False

        if domain_desc is None:
            return False

        unit = getattr(domain_desc, 'unit', None)
        if str(getattr(unit, 'symbol', None)) != 's':
            return False

        if getattr(domain_desc, 'tick_resolution', None) is None:
            return False

        # The rule only says whether the spacing is known up front; what
        # matters here is that a reader can turn it into ticks at all.
        rule = getattr(domain_desc, 'rule', None)
        if rule is None or rule.type not in (daq.DataRuleType.Linear,
                                             daq.DataRuleType.Explicit,
                                             daq.DataRuleType.Constant):
            return False

        return True

    # The axis a vector sample spans, or None when the signal is scalar.
    @staticmethod
    def _vector_dimension(desc):
        dims = getattr(desc, 'dimensions', None)
        if not dims or len(dims) != 1:
            return None

        dimension = daq.IDimension.cast_from(dims[0])
        if not dimension.size or dimension.size < 2:
            return None

        return dimension

    # MARK: Signal selection

    def _on_signal_selected(self, _event=None):
        self._reader = None
        self._selected_signal = None
        self._selected_descriptor = None
        self._selected_domain_descriptor = None

        self._data.clear()
        self._first_tick = None
        self._needs_redraw = True

        self._unit_str = ''
        self._forget_domain_timing()
        self._forget_dimension()
        self._refresh_scale_control()
        self._auto_window_applied = False

        name = self._signal_var.get()
        signal = self._eligible.get(name)
        if signal is None:
            return

        self._selected_signal = signal
        self._reader = daq.StreamReader(signal, skip_events=False)

    # MARK: Polling

    def _on_duration_committed(self, event=None):
        text = self._duration_var.get()
        match = self._DURATION_RE.match(text)
        if match is None:
            self._duration_var.set(f'{self.DEFAULT_WINDOW_SECONDS:g}s')
            return

        seconds = float(match.group(1))
        seconds = max(0.01, min(self.MAX_WINDOW_SECONDS, seconds))
        self._duration_var.set(f'{seconds:g}s')

        if seconds == self._window_seconds:
            return

        # Naming a duration hands the window to the user for good. A focus-out
        # that changes nothing falls out above, so it does not count.
        self._duration_user_set = True
        self._window_seconds = seconds
        self._needs_redraw = True
        self._chart_ready = False

    # Open on a window wide enough to actually show a trace. The 0.2s
    # default leaves a 1 Hz channel with a single point and no line.
    def _auto_window(self):
        if self._duration_user_set or self._auto_window_applied:
            return
        if self._sample_rate is None or self._sample_rate <= 0:
            return

        self._auto_window_applied = True

        # Only ever widen - a fast signal is served fine by the default.
        wanted = max(self.DEFAULT_WINDOW_SECONDS,
                     self.AUTO_WINDOW_SAMPLES / self._sample_rate)
        window = next((p for p in self.DURATION_PRESETS if p >= wanted),
                      self.DURATION_PRESETS[-1])
        if window == self._window_seconds:
            return

        self._window_seconds = window
        self._duration_var.set(f'{window:g}s')
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
        self._forget_domain_timing()
        self._forget_dimension()
        # deliberately no _refresh_scale_control() here: the same signal is
        # still selected and the reader is rebuilt with skip_events=False, so
        # its descriptor event restores the dimension a moment later. Resetting
        # would throw away a Log choice on every transient read failure.

        self._first_tick = None
        self._data.clear()
        self._needs_redraw = True

    def _forget_domain_timing(self):
        self._tick_res_num = None
        self._tick_res_den = None
        self._tick_delta = None
        self._sample_rate = None

    def _forget_dimension(self):
        self._bin_positions = None
        self._bin_unit = ''
        self._spectrum = None

    @property
    def _is_spectrum(self):
        return self._bin_positions is not None

    # A decade axis only says something about a spectrum; a scalar trace against
    # time has nothing to gain from it. The descriptor can turn one into the
    # other at any time, so the control follows it instead of the build.
    def _refresh_scale_control(self):
        if self._is_spectrum:
            self._scale_controls.pack(fill=tk.X, padx=(2, 12), pady=(0, 2))
            return

        # Log picked while the signal was a spectrum would otherwise keep the
        # axis logarithmic with nothing on screen left to undo it.
        if self._scale_var.get() != 'Linear':
            self._scale_var.set('Linear')
            self._needs_redraw = True

        self._scale_controls.pack_forget()

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

            try:
                values, domain_ticks, status = self._reader.read_with_domain(
                    read_count, return_status=True)
            except RuntimeError as e:
                print(f'[SignalPreview] Read failed: {e}')
                self._recreate_reader()
                return

            if len(values) > 0:
                self._ingest(values, domain_ticks)

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
            self._forget_dimension()
            if self._is_null_descriptor(self._selected_descriptor):
                self._unit_str = ''
            else:
                unit = getattr(self._selected_descriptor, 'unit', None)
                symbol = (getattr(unit, 'symbol', None)
                          if unit is not None else None)
                self._unit_str = str(symbol) if symbol is not None else ''
                self._apply_dimension(self._selected_descriptor)
            # The value descriptor is what decides whether samples are vectors,
            # so this is the one place the scale control can start to apply.
            self._refresh_scale_control()

        # An event that only changes the value descriptor leaves the domain out
        # entirely - the timing we already have stays valid.
        if domain_descriptor is None:
            return

        self._selected_domain_descriptor = daq.IDataDescriptor.cast_from(domain_descriptor)
        self._forget_domain_timing()

        if self._is_null_descriptor(self._selected_domain_descriptor):
            return

        res = self._selected_domain_descriptor.tick_resolution
        if res is None:
            return

        self._tick_res_num = res.numerator
        self._tick_res_den = res.denominator

        # A linear domain states its sample spacing up front, which is what
        # sizes the decimation. Explicit and constant domains only reveal it
        # once ticks arrive, so _ingest estimates it there instead.
        rule = self._selected_domain_descriptor.rule
        if rule is not None and rule.type == daq.DataRuleType.Linear:
            self._tick_delta = rule.parameters['delta']
            self._sample_rate = self._tick_res_den / self._tick_res_num / self._tick_delta
            self._auto_window()

    # Work out where each element of a vector sample sits on the x axis.
    # Scalar signals leave the spectrum state untouched and stay on time.
    def _apply_dimension(self, descriptor):
        dimension = self._vector_dimension(descriptor)
        if dimension is None:
            return

        unit = dimension.unit
        symbol = getattr(unit, 'symbol', None) if unit is not None else None
        self._bin_unit = str(symbol) if symbol is not None else ''

        rule = dimension.rule
        if rule is not None and rule.type == daq.DimensionRuleType.Linear:
            params = rule.parameters
            start = float(params['start'])
            delta = float(params['delta'])
            self._bin_positions = start + delta * np.arange(
                dimension.size, dtype=np.float64)
            return

        # Any other rule still enumerates its positions as labels.
        labels = dimension.labels
        self._bin_positions = np.asarray(
            [float(labels[i]) for i in range(len(labels))], dtype=np.float64)

    def _handle_event_packet(self, event_packet):
        event_id = event_packet.event_id
        self._needs_redraw = True

        # IMPLICIT_DOMAIN_GAP_DETECTED only says samples went missing. What is
        # already buffered stays true, so keep it and keep the time base.
        if event_id != 'DATA_DESCRIPTOR_CHANGED':
            return

        # A new descriptor can change the unit, the rule or the sample shape,
        # so nothing gathered against the old one is still comparable.
        self._data.clear()
        self._spectrum = None
        self._first_tick = None
        self._chart_ready = False
        
        params = event_packet.parameters
        data_desc = params['DataDescriptor']
        domain_desc = params['DomainDataDescriptor']

        self._apply_descriptors(data_desc, domain_desc)
        if not self._is_chartable(self._selected_descriptor, self._selected_domain_descriptor):
            self._deselect_signal()

    def _ingest(self, values, domain_ticks):
        if values is None:
            return

        n = len(values)
        if n == 0:
            return

        if self._is_spectrum:
            self._ingest_spectrum(values)
            return

        if domain_ticks is None:
            return

        if self._tick_res_num is None or self._tick_res_den is None:
            return

        ratio = self._tick_res_num / self._tick_res_den
        if self._first_tick is None:
            self._first_tick = int(domain_ticks[0])

        base = self._first_tick
        buf = self._data

        t_arr = (domain_ticks.astype(np.float64) - base) * ratio
        v_arr = values.astype(np.float64)

        self._update_sample_rate(t_arr)

        stride = self._decimation_stride()
        for i in range(0, n, stride):
            buf.append((t_arr[i], v_arr[i]))

        self._needs_redraw = True

    # Vector samples arrive flattened, one block of bins after another.
    # Only the newest block is on screen, so keep just that.
    def _ingest_spectrum(self, values):
        count = len(self._bin_positions)
        if len(values) < count:
            return

        self._spectrum = values[-count:].astype(np.float64)
        self._needs_redraw = True

    # A linear domain already told us its rate. For explicit and constant
    # domains, take it from the spacing of the ticks that just arrived.
    def _update_sample_rate(self, t_arr):
        if self._tick_delta is not None:
            return

        gaps = np.diff(t_arr)
        if len(gaps) == 0 and self._data:
            # Slow signals arrive a sample at a time, so the only spacing on
            # offer runs back to the previous read. These are domain ticks, so
            # delivery jitter does not distort it.
            gaps = np.array([t_arr[0] - self._data[-1][0]])

        gaps = gaps[gaps > 0]
        if len(gaps) == 0:
            return

        self._sample_rate = 1.0 / float(np.median(gaps))
        self._auto_window()

    # Thin out the buffer so one window never holds more points than the
    # canvas can use. Unknown rate means the signal is sparse anyway.
    def _decimation_stride(self):
        if self._sample_rate is None:
            return 1

        points_per_window = self._window_seconds * self._sample_rate
        if points_per_window <= self.TARGET_POINTS_PER_FRAME:
            return 1

        return math.ceil(points_per_window / self.TARGET_POINTS_PER_FRAME)

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
        for _ in range(self.MAX_HGRID):
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
        # Only the time axis has a duration to edit; a frequency axis has none.
        self._xlabel_l = canvas.create_text(
            0, 0, text='', fill=self._TEXT, anchor=tk.NW, font=self._AXIS_FONT,
            tags=() if self._is_spectrum else ('dur_label',))
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

        # Says why a Log selection is showing a linear axis.
        self._scale_note_id = canvas.create_text(
            0, 0, text='', fill=self._TEXT, anchor=tk.NW,
            font=self._AXIS_FONT, state='hidden')

        self._chart_ready = True

    def _schedule_draw(self):
        self._draw_job = self.after(self.DRAW_MS, self._draw_tick)

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

        frame = (self._spectrum_frame() if self._is_spectrum
                 else self._time_series_frame())

        if frame is None:
            canvas.coords(self._nodata_id, canvas_width // 2, canvas_height // 2)
            canvas.itemconfig(self._nodata_id, text='None', state='normal')
            canvas.itemconfig(self._line_id, state='hidden')
            canvas.itemconfig(self._scale_note_id, state='hidden')
            return

        canvas.itemconfig(self._nodata_id, state='hidden')

        y_axis = self._make_y_axis(*frame.y_range)
        x_axis = frame.x_axis

        y_labels = self._y_axis_labels(y_axis)

        label_font = tkfont.Font(font=self._AXIS_FONT)
        max_label_w = max(label_font.measure(t) for t in y_labels)
        # Round left margin up to not nudge
        margin_left = -(-(max_label_w + 8) // 4) * 4

        plot_width = canvas_width - margin_left - margin_right
        if plot_width < 10:
            return

        def xpx(x):
            return margin_left + x_axis.norm(x) * plot_width

        def ypx(v):
            return margin_top + (1.0 - y_axis.norm(v)) * plot_height

        # Horizontal grid + Y labels
        for i, tick in enumerate(y_axis.ticks):
            grid_y = ypx(tick)
            canvas.coords(self._hgrid_ids[i],
                        margin_left, grid_y, margin_left + plot_width, grid_y)
            canvas.itemconfig(self._hgrid_ids[i], state='normal')
            canvas.coords(self._hgrid_label_ids[i], margin_left - 4, grid_y)
            canvas.itemconfig(self._hgrid_label_ids[i], text=y_labels[i],
                              state='normal')

        for i in range(len(y_axis.ticks), self.MAX_HGRID):
            canvas.itemconfig(self._hgrid_ids[i], state='hidden')
            canvas.itemconfig(self._hgrid_label_ids[i], state='hidden')

        if self._log_unavailable:
            canvas.coords(self._scale_note_id, margin_left + 4, margin_top + 1)
            canvas.itemconfig(self._scale_note_id, state='normal',
                              text='log needs positive values')
        else:
            canvas.itemconfig(self._scale_note_id, state='hidden')

        # Axes
        canvas.coords(self._yaxis_id,
                    margin_left, margin_top,
                    margin_left, margin_top + plot_height)
        canvas.coords(self._xaxis_id,
                    margin_left, margin_top + plot_height,
                    margin_left + plot_width, margin_top + plot_height)

        # Single y for every bottom label so the row reads as one axis.
        label_y = margin_top + plot_height + 3

        slot = 0
        for tick in x_axis.ticks:
            if slot >= len(self._vgrid_ids):
                break
            grid_x = xpx(tick)
            canvas.coords(self._vgrid_ids[slot],
                          grid_x, margin_top, grid_x, margin_top + plot_height)
            canvas.itemconfig(self._vgrid_ids[slot], state='normal')

            canvas.coords(self._vgrid_label_ids[slot], grid_x, label_y)
            canvas.itemconfig(self._vgrid_label_ids[slot],
                              text=x_axis.label(tick), state='normal')
            slot += 1

        # Hide unused grid slots
        for i in range(slot, len(self._vgrid_ids)):
            canvas.itemconfig(self._vgrid_ids[i], state='hidden')
            canvas.itemconfig(self._vgrid_label_ids[i], state='hidden')

        left_label, right_label = frame.edge_labels
        canvas.coords(self._xlabel_l, margin_left, label_y)
        canvas.itemconfig(self._xlabel_l, text=left_label)

        canvas.coords(self._xlabel_r, margin_left + plot_width, label_y)
        canvas.itemconfig(self._xlabel_r, text=right_label)

        points = frame.points
        if len(points) > plot_width * 3:
            points = self._envelope(points, x_axis.norm, max(int(plot_width), 4))

        # Data line (1 px, no smoothing)
        if len(points) >= 2:
            if self._use_step_draw(points):
                points = self._step_points(points)
            line_coords = []
            for x, v in points:
                line_coords.extend([xpx(x), ypx(v)])
            canvas.coords(self._line_id, *line_coords)
            canvas.itemconfig(self._line_id, state='normal')
        elif len(points) == 1:
            px, py = xpx(points[0][0]), ypx(points[0][1])
            canvas.coords(self._line_id, px, py, px + 1, py)
            canvas.itemconfig(self._line_id, state='normal')

    # MARK: Frames

    # The rolling window: elapsed seconds across, newest sample pinned to
    # the right edge.
    def _time_series_frame(self):
        buf = self._data
        if not buf:
            return None

        window = self._window_seconds
        newest_time = buf[-1][0]
        window_start = newest_time - window

        visible = [(t, v) for t, v in buf if t >= window_start]
        if not visible:
            return None

        # Y range from a stabilized window so a brief spike does not
        # immediately rescale the whole chart.
        range_start = newest_time - max(window, 0.5)
        range_values = [v for t, v in buf if t >= range_start]
        positive = [v for v in range_values if v > 0]

        interval = next((ni for ni in self._TIME_GRID_INTERVALS
                         if window / ni <= 7), self._TIME_GRID_INTERVALS[-1])
        if interval >= 1:
            label_fmt = '-{:.0f}s'
        elif interval >= 0.1:
            label_fmt = '-{:.1f}s'
        elif interval >= 0.01:
            label_fmt = '-{:.2f}s'
        else:
            label_fmt = '-{:.3f}s'

        ticks = []
        k = 1
        while k * interval < window - 1e-9:
            ticks.append(newest_time - k * interval)
            k += 1

        x_axis = _XAxis(norm=lambda t: (t - window_start) / window,
                        ticks=ticks,
                        label=lambda t: label_fmt.format(newest_time - t))

        return _Frame(points=visible,
                      y_range=(min(range_values), max(range_values),
                               min(positive) if positive else 0.0),
                      x_axis=x_axis,
                      edge_labels=(label_fmt.format(window), '0s'))

    # A vector sample laid out across the axis its dimension spans. For an
    # FFT that is amplitude against frequency.
    def _spectrum_frame(self):
        row = self._spectrum
        if row is None or len(row) == 0:
            return None

        positions = self._bin_positions
        first_position = float(positions[0])
        last_position = float(positions[-1])
        span = last_position - first_position
        if span <= 0:
            return None

        interval = self._nice_interval(span, 6)
        ticks = []
        tick = first_position + interval
        while tick < last_position - interval * 0.25:
            ticks.append(tick)
            tick += interval

        scale, symbol = self._scale_to_unit((first_position, last_position),
                                            self._bin_unit)
        unit = f' {symbol}' if symbol else ''

        x_axis = _XAxis(norm=lambda x: (x - first_position) / span,
                        ticks=ticks,
                        label=lambda x: self._fmt(x / scale) + unit)

        # Amplitudes sit above zero, so pinning the floor there keeps the axis
        # from jumping on every block.
        positive = row[row > 0]
        return _Frame(points=list(zip(positions.tolist(), row.tolist())),
                      y_range=(min(0.0, float(row.min())), float(row.max()),
                               float(positive.min()) if positive.size else 0.0),
                      x_axis=x_axis,
                      edge_labels=(self._fmt(first_position / scale) + unit,
                                   self._fmt(last_position / scale) + unit))

    # MARK: Axes

    def _make_y_axis(self, value_min, value_max, min_positive):
        self._log_unavailable = False
        if self._scale_var.get() == 'Log':
            axis = self._log_y_axis(value_min, value_max, min_positive)
            if axis is not None:
                return axis
            # Falling back without saying so reads as the Log setting breaking.
            self._log_unavailable = True

        return self._linear_y_axis(value_min, value_max)

    @staticmethod
    def _linear_y_axis(value_min, value_max):
        if value_min == value_max:
            value_min -= 1.0
            value_max += 1.0
        else:
            pad = (value_max - value_min) * 0.08
            value_min -= pad
            value_max += pad

        span = value_max - value_min
        # Subtracting the span back off leaves float residue that would
        # otherwise print a zero crossing as something like 8.882e-16.
        ticks = [0.0 if abs(t) < span * 1e-12 else t
                 for t in (value_max - span * i / 4 for i in range(5))]
        return _YAxis(lambda v: (v - value_min) / span, ticks, False)

    # Log10 axis bounded by the data rather than by whole decades, because
    # snapping a 4..6 V signal out to 1..10 leaves it a squiggle using a sixth
    # of the height. Returns None when a log axis cannot mean anything, which
    # leaves the caller on a linear one.
    @classmethod
    def _log_y_axis(cls, value_min, value_max, min_positive):
        if value_max <= 0:
            return None

        # Half of a signal that swings through zero has no logarithm, and
        # clamping it would draw that half as a solid line along the floor.
        if value_min < 0:
            return None

        # Zeros are legitimate in a spectrum, so the floor comes from the
        # smallest real reading, capped so one tiny bin cannot stretch the axis.
        floor = min_positive if min_positive > 0 else 0.0
        cap = value_max / 10.0 ** cls.LOG_MAX_DECADES
        floor = max(floor, cap)

        low, high, ticks = cls._log_ticks(floor, value_max)
        if high <= low:
            return None

        span = math.log10(high / low)

        def norm(v):
            if v <= 0:
                return 0.0
            return min(1.0, max(0.0, math.log10(v / low) / span))

        return _YAxis(norm, list(reversed(ticks)), True)

    # Gridlines at mantissa x 10^k, subdividing further as the span narrows so
    # the trace fills the plot instead of hugging one decade line.
    @classmethod
    def _log_ticks(cls, floor, ceiling):
        decades = math.log10(ceiling / floor) if floor > 0 else cls.LOG_MAX_DECADES
        if decades >= 2:
            # Wide spans get one line per decade. Subdividing them produces a
            # wall of labels, and every third one lands off a round prefix.
            mantissas = (1,)
        elif decades >= 1:
            mantissas = (1, 2, 5)
        else:
            mantissas = (1, 2, 3, 4, 5, 6, 7, 8, 9)

        candidates = []
        for exponent in range(math.floor(math.log10(floor)),
                              math.ceil(math.log10(ceiling)) + 1):
            for mantissa in mantissas:
                candidates.append(mantissa * 10.0 ** exponent)
        candidates.sort()

        low = max((c for c in candidates if c <= floor), default=candidates[0])
        high = min((c for c in candidates if c >= ceiling), default=candidates[-1])
        ticks = [c for c in candidates if low <= c <= high]

        # Keep both ends and sample evenly between them.
        if len(ticks) > cls.MAX_HGRID:
            last = len(ticks) - 1
            picked = {ticks[round(i * last / (cls.MAX_HGRID - 1))]
                      for i in range(cls.MAX_HGRID)}
            ticks = sorted(picked)

        return low, high, ticks


    # Separate a metric prefix from the unit it scales: 'kB' -> (3, 'B'). Only
    # two-letter symbols split, so 'min' and 'mol' stay whole.
    @classmethod
    def _split_unit_prefix(cls, symbol):
        if len(symbol) != 2 or symbol in cls._UNSPLITTABLE_UNITS:
            return 0, symbol

        exponent = cls._PREFIX_EXPONENTS.get(symbol[0])
        if exponent is None or not symbol[1].isalpha():
            return 0, symbol

        return exponent, symbol[1]

    # Divisor and restated unit that keep the ticks readable as plain numbers:
    # 742400 'kB' -> (1000.0, 'MB'). Units that are not letters ('%', 'm/s')
    # and non-metric ones come back unchanged with a divisor of 1.
    @classmethod
    def _scale_to_unit(cls, values, symbol):
        if symbol in cls._UNPREFIXABLE_UNITS:
            return 1.0, symbol

        peak = max((abs(v) for v in values), default=0.0)
        if peak == 0.0 or not math.isfinite(peak):
            return 1.0, symbol

        shift = int(math.floor(math.log10(peak) / 3)) * 3
        if shift == 0:
            return 1.0, symbol

        base_exponent, base = cls._split_unit_prefix(symbol)
        if not base.isalpha():
            return 1.0, symbol

        prefix = cls._SI_PREFIXES.get(base_exponent + shift)
        if prefix is None:
            return 1.0, symbol

        return 10.0 ** shift, prefix + base

    def _y_axis_labels(self, y_axis):
        if y_axis.engineering:
            return [self._engineering_label(v) for v in y_axis.ticks]

        scale, symbol = self._scale_to_unit(y_axis.ticks, self._unit_str)
        suffix = f' {symbol}' if symbol else ''
        return [self._fmt(v / scale) + suffix for v in y_axis.ticks]

    # '2e-07 W' is unreadable next to '5e-08 W'. Give the tick its own prefix so
    # it reads '200 nW' against '50 nW'.
    def _engineering_label(self, value):
        scale, symbol = self._scale_to_unit((value,), self._unit_str)
        text = self._fmt(value / scale)
        return f'{text} {symbol}' if symbol else text

    # Largest 1/2/5 x 10^k step that keeps the tick count within max.
    @staticmethod
    def _nice_interval(span, max_ticks):
        raw = span / max_ticks
        magnitude = 10.0 ** math.floor(math.log10(raw))
        for factor in (1.0, 2.0, 5.0):
            if factor * magnitude >= raw:
                return factor * magnitude
        return 10.0 * magnitude

    # Slowly sampled channels - status flags, CPU load, navigation fixes -
    # hold their value between samples, so a diagonal between two of them
    # would invent readings. This is about how slow the signal is, not
    # which rule it uses: a waveform stays a plain line however far you
    # zoom in.
    def _use_step_draw(self, points):
        if self._is_spectrum:
            return False
        if self._sample_rate is not None:
            return self._sample_rate * self.SPARSE_SAMPLE_SECONDS <= 1.0
        return len(points) <= self.STEP_MAX_POINTS

    @staticmethod
    def _step_points(points):
        stepped = [points[0]]
        for (_, held), (x, v) in zip(points, points[1:]):
            stepped.append((x, held))
            stepped.append((x, v))
        return stepped

    # Collapse dense data down to a min/max pair per pixel column, keeping
    # left-to-right order so the trace still reads as one line.
    @staticmethod
    def _envelope(points, norm, buckets):
        out = []
        current = -1
        lowest = highest = None

        for x, v in points:
            index = min(int(norm(x) * buckets), buckets - 1)
            if index != current and lowest is not None:
                out.extend((lowest,) if lowest == highest else sorted((lowest, highest)))
                lowest = highest = None
            current = index

            if lowest is None or v < lowest[1]:
                lowest = (x, v)
            if highest is None or v > highest[1]:
                highest = (x, v)

        if lowest is not None:
            out.extend((lowest,) if lowest == highest else sorted((lowest, highest)))

        return out

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