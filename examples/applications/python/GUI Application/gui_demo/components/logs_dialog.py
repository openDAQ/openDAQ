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
