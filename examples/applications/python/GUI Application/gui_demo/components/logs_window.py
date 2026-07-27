import os
import re
import tkinter as tk
from collections import deque
from tkinter import ttk


# non-modal window tailing the log file written by the instance's rotating
# file sink, with a severity filter over what it shows
class LogsWindow(tk.Toplevel):

    POLL_INTERVAL_MS = 400
    MAX_LINES = 5000

    LINE_TAGS = (
        ('[critical]', 'critical'),
        ('[error]', 'error'),
        ('[warning]', 'warning'),
        ('[debug]', 'quiet'),
        ('[trace]', 'quiet'),
    )

    LEVEL_ORDER = ('trace', 'debug', 'info', 'warning', 'error', 'critical')
    SHOW_ALL = 'All'

    _LEVEL_RE = re.compile(r'^\[[^\]]*\]\s*\[[^\]]*\]\s*\[([a-z]+)\]')

    def __init__(self, parent, context, **kwargs):
        tk.Toplevel.__init__(self, parent, **kwargs)
        self.title('Logs')
        self.context = context

        self.geometry('{}x{}'.format(
            int(1000 * context.ui_scaling_factor * context.dpi_factor),
            int(450 * context.ui_scaling_factor * context.dpi_factor)))

        toolbar = ttk.Frame(self)
        toolbar.pack(fill=tk.X, padx=5, pady=5)

        self._follow_var = tk.BooleanVar(value=True)
        ttk.Checkbutton(toolbar, text='Follow',
                        variable=self._follow_var).pack(side=tk.LEFT)
        ttk.Button(toolbar, text='Clear',
                   command=self.handle_clear_clicked).pack(side=tk.LEFT, padx=5)

        # A view filter, not a logger setting - what the instance emits is
        # fixed when it is built, in the configure-instance dialog.
        ttk.Label(toolbar, text='Show:').pack(side=tk.LEFT, padx=(15, 2))
        self._filter_rank = 0
        self.level_combo = ttk.Combobox(
            toolbar, state='readonly', width=9,
            values=[self.SHOW_ALL] + [name.capitalize()
                                      for name in self.LEVEL_ORDER])
        self.level_combo.set(self.SHOW_ALL)
        self.level_combo.bind('<<ComboboxSelected>>', self.handle_filter_selected)
        self.level_combo.pack(side=tk.LEFT)

        self._path_label = ttk.Label(toolbar, text=context.log_file_path,
                                     foreground='gray')
        self._path_label.pack(side=tk.RIGHT)

        text_frame = ttk.Frame(self)
        text_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=(0, 5))

        text = tk.Text(text_frame, wrap=tk.NONE, state=tk.DISABLED,
                       font='TkFixedFont')
        scroll_y = ttk.Scrollbar(text_frame, orient=tk.VERTICAL,
                                 command=text.yview)
        scroll_x = ttk.Scrollbar(text_frame, orient=tk.HORIZONTAL,
                                 command=text.xview)
        text.configure(yscrollcommand=scroll_y.set, xscrollcommand=scroll_x.set)
        scroll_y.pack(side=tk.RIGHT, fill=tk.Y)
        scroll_x.pack(side=tk.BOTTOM, fill=tk.X)
        text.pack(fill=tk.BOTH, expand=True)

        text.tag_configure('critical', foreground='red')
        text.tag_configure('error', foreground='red')
        text.tag_configure('warning', foreground='#b8860b')
        text.tag_configure('quiet', foreground='gray')
        text.tag_configure('meta', foreground='gray')
        self.text = text

        self._file = None
        self._file_path = None
        self._file_pos = 0
        self._poll_job = None

        # Every line read so far, so changing the filter can re-render without
        # re-reading the file. Bounded by the same cap as the widget.
        self._lines = deque(maxlen=self.MAX_LINES)
        self._last_rank = 0

        self.protocol('WM_DELETE_WINDOW', self.close)

        if not context.log_to_file:
            self.append_meta_line(
                'File logging is disabled. Enable it in the configure-instance '
                'dialog on the next start to see logs here.\n')
        self.poll_log_file()

    def handle_filter_selected(self, event=None):
        selected = self.level_combo.get()
        if selected == self.SHOW_ALL:
            self._filter_rank = 0
        else:
            self._filter_rank = self.LEVEL_ORDER.index(selected.lower())
        self.render_lines()

    # Severity of a line, or None for continuation lines such as the
    # ' - Caused by:' that follow an error and carry no level of their own.
    @classmethod
    def line_rank(cls, line):
        match = cls._LEVEL_RE.match(line)
        if match is None:
            return None
        level = match.group(1)
        if level not in cls.LEVEL_ORDER:
            return None
        return cls.LEVEL_ORDER.index(level)

    def render_lines(self):
        self.text.configure(state=tk.NORMAL)
        self.text.delete('1.0', tk.END)
        rank = 0
        for line in self._lines:
            parsed = self.line_rank(line)
            if parsed is not None:
                rank = parsed
            if rank >= self._filter_rank:
                self.text.insert(tk.END, line, self.tags_for_line(line))
        self.text.configure(state=tk.DISABLED)
        if self._follow_var.get():
            self.text.see(tk.END)

    @classmethod
    def tags_for_line(cls, line):
        for token, tag_name in cls.LINE_TAGS:
            if token in line:
                return (tag_name,)
        return ()

    def handle_clear_clicked(self):
        self._lines.clear()
        self.text.configure(state=tk.NORMAL)
        self.text.delete('1.0', tk.END)
        self.text.configure(state=tk.DISABLED)

    def poll_log_file(self):
        self._poll_job = None
        if not self.winfo_exists():
            return

        path = self.context.log_file_path
        try:
            size = os.path.getsize(path) if os.path.exists(path) else None
        except OSError:
            size = None

        if size is not None:
            try:
                # reopen when the sink rotated the file or the instance was
                # recreated with a new log file
                if self._file is None or path != self._file_path \
                        or size < self._file_pos:
                    if self._file is not None:
                        self._file.close()
                        if path != self._file_path:
                            self.append_meta_line(
                                '--- instance recreated, following new log file ---\n')
                            self._path_label.configure(text=path)
                        else:
                            self.append_meta_line('--- log file rotated ---\n')
                    self._file = open(path, 'r', encoding='utf-8',
                                      errors='replace')
                    self._file_path = path
                    self._file_pos = 0
                self._file.seek(self._file_pos)
                data = self._file.read()
                self._file_pos = self._file.tell()
                if data:
                    self.append_log_data(data)
            except OSError:
                pass

        self._poll_job = self.after(self.POLL_INTERVAL_MS, self.poll_log_file)

    def append_log_data(self, data):
        self.text.configure(state=tk.NORMAL)
        for line in data.splitlines(True):
            self._lines.append(line)
            parsed = self.line_rank(line)
            if parsed is not None:
                self._last_rank = parsed
            if self._last_rank >= self._filter_rank:
                self.text.insert(tk.END, line, self.tags_for_line(line))

        # cap the amount of text kept in the widget
        line_count = int(self.text.index('end-1c').split('.')[0])
        if line_count > self.MAX_LINES:
            self.text.delete('1.0', '{}.0'.format(line_count - self.MAX_LINES))
        self.text.configure(state=tk.DISABLED)

        if self._follow_var.get():
            self.text.see(tk.END)

    def append_meta_line(self, line):
        self.text.configure(state=tk.NORMAL)
        self.text.insert(tk.END, line, ('meta',))
        self.text.configure(state=tk.DISABLED)

    def close(self):
        if self._poll_job is not None:
            self.after_cancel(self._poll_job)
            self._poll_job = None
        if self._file is not None:
            self._file.close()
            self._file = None
        self.destroy()
