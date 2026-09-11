import os
import tkinter as tk
from tkinter import ttk

from .. import utils
from ..app_context import AppContext
from .dialog import Dialog

POLL_INTERVAL_MS = 500
INITIAL_TAIL_BYTES = 256 * 1024
MAX_LINES = 5000


class LogsDialog(Dialog):

    def __init__(self, parent, context: AppContext, **kwargs):
        super().__init__(parent, 'Logs', context, **kwargs)
        self.geometry('{}x{}'.format(int(900 * context.dpi_factor),
                                     int(520 * context.dpi_factor)))
        self.log_file_path = getattr(context, 'log_file_path', None)
        self.offset = 0
        self.poll_id = None
        self.line_count = 0

        header = ttk.Frame(self)
        self.follow_var = tk.BooleanVar(value=True)
        ttk.Checkbutton(header, text='Follow',
                        variable=self.follow_var).pack(side=tk.LEFT)
        ttk.Button(header, text='Clear',
                   command=self.handle_clear_clicked).pack(side=tk.RIGHT, padx=4)
        header.pack(fill=tk.X, pady=(0, 6))

        footer = ttk.Frame(self)
        self.status_label = ttk.Label(footer, text='', foreground='gray')
        self.status_label.pack(side=tk.LEFT)
        ttk.Button(footer, text='Close',
                   command=self.close).pack(side=tk.RIGHT)
        footer.pack(side=tk.BOTTOM, fill=tk.X, pady=(6, 0))

        text_frame = ttk.Frame(self)
        text = tk.Text(text_frame, wrap=tk.NONE, state=tk.DISABLED,
                       font=('Consolas', 9), bd=1, relief=tk.SUNKEN)
        y_scroll = ttk.Scrollbar(text_frame, orient=tk.VERTICAL,
                                 command=text.yview)
        x_scroll = ttk.Scrollbar(text_frame, orient=tk.HORIZONTAL,
                                 command=text.xview)
        text.configure(yscrollcommand=y_scroll.set, xscrollcommand=x_scroll.set)
        y_scroll.pack(side=tk.RIGHT, fill=tk.Y)
        x_scroll.pack(side=tk.BOTTOM, fill=tk.X)
        text.pack(fill=tk.BOTH, expand=True)
        text_frame.pack(fill=tk.BOTH, expand=True)
        text.tag_configure('warning', foreground=str(utils.StatusColor.WARNING))
        text.tag_configure('error', foreground=str(utils.StatusColor.ERROR))
        text.tag_configure('debug', foreground='gray40')
        self.text = text

    def initial_update(self):
        if not self.log_file_path:
            self.set_status('No log file sink, logs are going to the console only')
            return
        if os.path.exists(self.log_file_path):
            self.offset = max(0, os.path.getsize(self.log_file_path)
                              - INITIAL_TAIL_BYTES)
        self.poll()

    def poll(self):
        self.read_new_lines()
        self.poll_id = self.after(POLL_INTERVAL_MS, self.poll)

    def read_new_lines(self):
        if not self.log_file_path or not os.path.exists(self.log_file_path):
            self.set_status('Waiting for %s' % self.log_file_path)
            return
        try:
            with open(self.log_file_path, 'rb') as handle:
                handle.seek(self.offset)
                chunk = handle.read()
                self.offset = handle.tell()
        except OSError as e:
            self.set_status('Cannot read log file: %s' % e)
            return
        if chunk:
            self.append_lines(chunk.decode('utf-8', errors='replace').splitlines())

    def append_lines(self, lines):
        self.text.configure(state=tk.NORMAL)
        for line in lines:
            self.text.insert(tk.END, line + '\n', self.tag_for_line(line))
            self.line_count += 1
        if self.line_count > MAX_LINES:
            drop = self.line_count - MAX_LINES
            self.text.delete('1.0', '%d.0' % (drop + 1))
            self.line_count -= drop
        self.text.configure(state=tk.DISABLED)
        if self.follow_var.get():
            self.text.see(tk.END)
        self.set_status('%d lines  |  %s' % (self.line_count, self.log_file_path))

    def tag_for_line(self, line):
        lowered = line.lower()
        if '[error]' in lowered or '[critical]' in lowered:
            return 'error'
        if '[warning]' in lowered or '[warn]' in lowered:
            return 'warning'
        if '[debug]' in lowered or '[trace]' in lowered:
            return 'debug'
        return ''

    def set_status(self, text):
        self.status_label.configure(text=text)

    def handle_clear_clicked(self):
        self.text.configure(state=tk.NORMAL)
        self.text.delete('1.0', tk.END)
        self.text.configure(state=tk.DISABLED)
        self.line_count = 0

    def close(self):
        if self.poll_id is not None:
            self.after_cancel(self.poll_id)
            self.poll_id = None
        super().close()
