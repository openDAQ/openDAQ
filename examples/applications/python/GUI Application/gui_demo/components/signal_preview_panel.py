import tkinter as tk
from tkinter import ttk
from collections import deque

import opendaq as daq

from .. import utils


class SignalPreviewPanel(ttk.Frame):
    WINDOW_SECONDS = 5.0
    MAX_BUFFER_SIZE = 10_000
    POLL_MS = 33          # drain the reader this often
    DRAW_MS = 66         # repaint the chart this often
    CANVAS_HEIGHT = 160

    _BG      = '#1e1e2e'
    _LINE    = '#89b4fa'
    _GRID    = '#313244'
    _TEXT    = '#cdd6f4'
    _AXIS    = '#585b70'
    _FILL    = '#252942'

    _AXIS_FONT = ('TkFixedFont', 7)
    _VAL_FONT  = ('TkFixedFont', 10, 'bold')

    def __init__(self, parent, node, context=None, **kwargs):
        super().__init__(parent, **kwargs)
        self.node = node
        self.context = context

        self._collapsed = True
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
        self._needs_redraw = True
        self._chart_ready = False

        # Build widgets
        self._build_toggle_bar()
        self._build_content()
        self._populate_dropdown()

        self._schedule_poll()
        self._schedule_draw()
        self.bind('<Destroy>', self._on_destroy)
        self._update_placement()

    def _build_toggle_bar(self):
        self._toggle_frame = utils.make_banner(self, '\u25B2 Signal Preview')
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

        # Chart canvas
        self._chart = tk.Canvas(
            self._content_frame, bg=self._BG,
            height=self.CANVAS_HEIGHT, highlightthickness=0)
        self._chart.pack(fill=tk.BOTH, expand=True, padx=4, pady=(2, 4))

        self._chart.bind('<Configure>', lambda _e: self._invalidate_chart())
        self._block_mousewheel_recursive(self)

    def _block_mousewheel_recursive(self, widget):
        for event in ('<MouseWheel>', '<Button-4>', '<Button-5>'):
            widget.bind(event, lambda _e: 'break')
        for child in widget.winfo_children():
            self._block_mousewheel_recursive(child)

    # MARK: Dropdown
    def _populate_dropdown(self):
        self._eligible.clear()

        if self.node is None:
            return

        signals = []
        if daq.IFunctionBlock.can_cast_from(self.node):
            fb = daq.IFunctionBlock.cast_from(self.node)
            search_filter = (
                daq.AnySearchFilter()
                if self.context and self.context.view_hidden_components
                else None)
            signals = fb.get_signals(search_filter)

        for sig in signals:
            if sig is None or not daq.ISignal.can_cast_from(sig):
                continue
            signal = daq.ISignal.cast_from(sig)
            if not self._is_chartable(signal):
                continue

            name = signal.name if signal.name else signal.global_id
            display = name
            counter = 1
            while display in self._eligible:
                counter += 1
                display = f'{name} ({counter})'
            self._eligible[display] = signal

        names = list(self._eligible.keys())
        self._dropdown['values'] = names

        if len(names) == 1:
            self._signal_var.set(names[0])
            self._on_signal_selected()

    @staticmethod
    def _is_chartable(signal):
        try:
            desc = signal.descriptor
            if desc is None:
                return False

            st_name = desc.sample_type.name if hasattr(desc.sample_type, 'name') else str(desc.sample_type)
            numeric_prefixes = ('Float', 'Int', 'UInt', 'float', 'int', 'uint')
            if not any(st_name.startswith(p) for p in numeric_prefixes):
                return False

            domain_sig = signal.domain_signal
            if domain_sig is None:
                return False
            domain_desc = domain_sig.descriptor
            if domain_desc is None:
                return False

            unit = domain_desc.unit
            if unit is None or unit.quantity is None:
                return False
            if unit.quantity.casefold() != 'time':
                return False

            if domain_desc.tick_resolution is None:
                return False

            return True
        except (RuntimeError, AttributeError):
            return False

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

        try:
            self._reader = daq.StreamReader(signal)
        except RuntimeError as e:
            print(f'[SignalPreview] Failed to create StreamReader: {e}')
            self._reader = None

    # MARK: Polling

    def _on_duration_selected(self, _event=None):
        raw = self._duration_var.get().rstrip('s')
        try:
            seconds = float(raw)
        except ValueError:
            return
        if seconds <= 0:
            return
        self.WINDOW_SECONDS = seconds
        self._needs_redraw = True

        # Vertical grid lines are created once for the initial WINDOW_SECONDS.
        self._chart_ready = False

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
                    # Cap each read so the UI thread isn't blocked for too long
                    # on very high sample-rate signals.
                    count = min(available, 8000)
                    values, domain_ticks = self._reader.read_with_domain(count)
                    self._ingest(values, domain_ticks)
            except RuntimeError as e:
                # The reader can become invalid when the signal is disconnected
                # or its descriptor changes.
                print(f'[SignalPreview] Read error (reader invalidated): {e}')
                self._reader = None

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

        for i in range(n):
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

        # Vertical grid (1-second intervals inside the window)
        n_vsecs = max(int(self.WINDOW_SECONDS) - 1, 0)
        self._vgrid_ids = []
        self._vgrid_label_ids = []
        for _ in range(n_vsecs):
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

        self._badge_id = c.create_text(
            0, 0, text='', fill=self._LINE, anchor=tk.NE, font=self._VAL_FONT)

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

        ml, mr, mt, mb = 55, 12, 10, 22
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
            return

        # Y range with breathing room
        vals = [v for _, v in visible]
        v_lo, v_hi = min(vals), max(vals)
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

        def ypx(v):
            return mt + (1.0 - (v - v_lo) / v_span) * ph

        # Horizontal grid + Y labels
        for i in range(5):
            gy = mt + ph * i / 4
            c.coords(self._hgrid_ids[i], ml, gy, ml + pw, gy)
            gv = v_hi - v_span * i / 4
            c.coords(self._hgrid_label_ids[i], ml - 4, gy)
            c.itemconfig(self._hgrid_label_ids[i], text=self._fmt(gv))

        # Vertical grid at 1-second marks
        for i, vid in enumerate(self._vgrid_ids):
            # i=0 -> -4s, i=1 -> -3s, ... i=3 -> -1s  (for 5s window)
            secs_ago = int(self.WINDOW_SECONDS) - 1 - i
            t_mark = t_latest - secs_ago
            vx = xpx(t_mark)
            c.coords(vid, vx, mt, vx, mt + ph)
            c.coords(self._vgrid_label_ids[i], vx, mt + ph + 3)
            c.itemconfig(self._vgrid_label_ids[i], text=f'-{secs_ago}s')

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

        # Current value badge
        c.coords(self._badge_id, ml + pw - 4, mt + 4)
        c.itemconfig(
            self._badge_id, text=self._fmt(visible[-1][1]), state='normal')

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

        self.update_idletasks()
        self._update_placement()

    def _update_placement(self):
        self.update_idletasks()
        self.place_configure(height=self.winfo_reqheight())

    def _on_destroy(self, event):
        if event.widget is not self:
            return
        for job_attr in ('_poll_job', '_draw_job'):
            job = getattr(self, job_attr, None)
            if job is not None:
                try:
                    self.after_cancel(job)
                except Exception:
                    pass
                setattr(self, job_attr, None)
        self._reader = None