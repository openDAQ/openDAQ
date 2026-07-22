import tkinter as tk
from tkinter import ttk

from .. import utils
from ..app_context import AppContext
from .dialog import Dialog


# startup dialog collecting instance parameters before the openDAQ instance
# is created; closing the dialog (Start, Escape or the window button) applies
# the entered values to the app context
class ConfigureInstanceDialog(Dialog):
    def __init__(self, parent, context: AppContext, **kwargs):
        Dialog.__init__(self, parent, 'Configure instance', context, **kwargs)


        self.geometry(f'{int(620 * context.dpi_factor)}x{int(250 * context.dpi_factor)}')
        form = ttk.Frame(self)
        form.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

        entry_width = int(50 * self.context.ui_scaling_factor)

        ttk.Label(form, text='Connection string:').grid(
            row=0, column=0, sticky=tk.W, pady=2)
        self.connection_string_entry = ttk.Entry(form, width=entry_width)
        self.connection_string_entry.grid(
            row=0, column=1, sticky=tk.EW, pady=2, padx=(5, 0))

        ttk.Label(form, text='Module path:').grid(
            row=1, column=0, sticky=tk.W, pady=2)
        self.module_path_entry = ttk.Entry(form, width=entry_width)
        self.module_path_entry.grid(
            row=1, column=1, sticky=tk.EW, pady=2, padx=(5, 0))

        ttk.Label(form, text='Discovery servers:').grid(
            row=2, column=0, sticky=tk.W, pady=2)
        self.discovery_servers_entry = ttk.Entry(form, width=entry_width)
        self.discovery_servers_entry.grid(
            row=2, column=1, sticky=tk.EW, pady=2, padx=(5, 0))

        self._demo_var = tk.BooleanVar(value=context.include_reference_devices)
        ttk.Checkbutton(form, text='Include reference (demo) devices',
                        variable=self._demo_var).grid(
            row=3, column=0, columnspan=2, sticky=tk.W, pady=2)

        logger_row = ttk.Frame(form)
        logger_row.grid(row=4, column=0, columnspan=2, sticky=tk.EW, pady=(8, 2))
        ttk.Button(logger_row, text='Configure logger…',
                   command=self.handle_configure_logger_clicked).pack(side=tk.LEFT)
        self.logger_summary_label = ttk.Label(logger_row, foreground='gray')
        self.logger_summary_label.pack(side=tk.LEFT, padx=8)

        form.grid_columnconfigure(1, weight=1)

        actions_row = ttk.Frame(self)
        actions_row.pack(fill=tk.X, pady=(8, 2))
        start_button = ttk.Button(actions_row, text='Start',
                                  command=self.handle_start_clicked)
        start_button.pack(side=tk.RIGHT)
        self.bind('<Return>', lambda event: self.handle_start_clicked())
        start_button.focus_set()

        # prefill from the values collected from the command line
        if context.connection_string:
            self.connection_string_entry.insert(0, context.connection_string)
        if context.module_path:
            self.module_path_entry.insert(0, context.module_path)
        self.discovery_servers_entry.insert(
            0, ', '.join(context.discovery_servers))
        self.update_logger_summary()

    def update_logger_summary(self):
        file_part = 'file: {}'.format(
            utils.log_level_name(self.context.file_log_level)
            if self.context.log_to_file else 'off')
        self.logger_summary_label.configure(text='Global: {}, {}'.format(
            utils.log_level_name(self.context.log_level), file_part))

    def handle_configure_logger_clicked(self):
        dialog = ConfigureLoggerDialog(self, self.context)
        dialog.show()
        self.update_logger_summary()

    def handle_start_clicked(self):
        self.close()

    def apply(self):
        self.context.connection_string = self.connection_string_entry.get().strip() or None
        self.context.module_path = self.module_path_entry.get().strip() or None
        self.context.discovery_servers = [
            s.strip() for s in self.discovery_servers_entry.get().split(',') if s.strip()]
        self.context.include_reference_devices = self._demo_var.get()

    def close(self):
        self.apply()
        Dialog.close(self)


# logger settings sub-dialog of ConfigureInstanceDialog; writes directly to
# the app context on close
class ConfigureLoggerDialog(Dialog):
    def __init__(self, parent, context: AppContext, **kwargs):
        Dialog.__init__(self, parent, 'Configure logger', context, **kwargs)

        self.geometry('{}x{}'.format(
            int(460 * self.context.ui_scaling_factor * self.context.dpi_factor),
            int(170 * self.context.ui_scaling_factor * self.context.dpi_factor)))

        form = ttk.Frame(self)
        form.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

        level_names = list(utils.LOG_LEVELS)

        ttk.Label(form, text='Global log level:').grid(
            row=0, column=0, sticky=tk.W, pady=2)
        self.global_level_combo = ttk.Combobox(
            form, state='readonly', values=level_names)
        self.global_level_combo.set(utils.log_level_name(context.log_level))
        self.global_level_combo.grid(
            row=0, column=1, sticky=tk.EW, pady=2, padx=(5, 0))

        self._log_to_file_var = tk.BooleanVar(value=context.log_to_file)
        ttk.Checkbutton(form, text='Write logs to a file (enables the Logs window)',
                        variable=self._log_to_file_var).grid(
            row=1, column=0, columnspan=2, sticky=tk.W, pady=2)

        ttk.Label(form, text='File log level:').grid(
            row=2, column=0, sticky=tk.W, pady=2)
        self.file_level_combo = ttk.Combobox(
            form, state='readonly', values=level_names)
        self.file_level_combo.set(utils.log_level_name(context.file_log_level))
        self.file_level_combo.grid(
            row=2, column=1, sticky=tk.EW, pady=2, padx=(5, 0))

        form.grid_columnconfigure(1, weight=1)

        actions_row = ttk.Frame(self)
        actions_row.pack(fill=tk.X, pady=(8, 2))
        ttk.Button(actions_row, text='OK',
                   command=self.close).pack(side=tk.RIGHT)

    def apply(self):
        self.context.log_level = utils.LOG_LEVELS[self.global_level_combo.get()]
        self.context.log_to_file = self._log_to_file_var.get()
        self.context.file_log_level = utils.LOG_LEVELS[self.file_level_combo.get()]

    def close(self):
        self.apply()
        Dialog.close(self)
