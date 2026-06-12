import tkinter as tk
from tkinter import ttk

import opendaq as daq
from opendaq import IStruct

from .. import utils
from .attributes_dialog import AttributesDialog
from .dialog import Dialog
from .output_signal_graph import OutputSignalGraph


class OutputSignalRow(ttk.Frame):
    def __init__(self, parent, output_signal, context=None, **kwargs):
        ttk.Frame.__init__(self, parent, **kwargs)
        self.parent = parent
        self.output_signal = output_signal
        self.selection = ''
        self.context = context

        self._expanded = False
        self._preview = None
        self._show_bottom_sep = True

        self.configure(padding=(10, 3))

        last_value, raw_value = self._read_values()
        self._view_value = last_value

        is_struct_or_string = False
        if isinstance(raw_value, daq.IBaseObject) and IStruct.can_cast_from(raw_value) or isinstance(raw_value, str):
            is_struct_or_string = True

        # Check whether this signal can be charted so we know whether
        # to show the expand arrow.  Gated on the global toggle too.
        self._chartable = self._check_chartable()
        show_expand = (
            self._chartable
            and context is not None
            and getattr(context, 'view_signal_preview', False))

        # Bottom separator between rows.
        self._bottom_sep = tk.Frame(self, height=1, bg='#cccccc')
        self._bottom_sep.grid(
            row=3, column=0, columnspan=3, sticky=tk.EW, pady=(6, 0))

        # Column 0: name label
        self._name_frame = ttk.Frame(self)
        self._name_frame.grid(row=1, column=0, sticky=tk.W)
        
        self._arrow_label = None
        if show_expand:
            self._arrow_right = (
                context.icons.get('right')
                if context and context.icons else None)
            self._arrow_down = (
                context.icons.get('down')
                if context and context.icons else None)
            self._arrow_label = ttk.Label(
                self._name_frame, image=self._arrow_right, cursor='hand2')
            self._arrow_label.pack(side=tk.LEFT, padx=(0, 2))
            self._arrow_label.bind('<Button-1>', lambda _e: self._toggle_expand())

        ttk.Label(self._name_frame, text=output_signal.name, anchor=tk.W).pack(
            side=tk.LEFT)

        # Duration control
        self._duration_presets = [0.01, 0.05, 0.1, 0.2, 0.5, 1]
        self._duration_var = tk.StringVar(value='0.2s')
        self._dur_label = ttk.Label(self._name_frame, text='|     Display duration:')
        self._dur_cb = ttk.Combobox(
            self._name_frame, textvariable=self._duration_var,
            values=[f'{d:g}s' for d in self._duration_presets], width=12)

        # Value column with optional View button
        value_frame = ttk.Frame(self)
        value_frame.grid(row=1, column=1, sticky=tk.EW)

        self.view_button = None
        
        if is_struct_or_string:
            self.view_button = ttk.Button(
                value_frame, text='View', command=lambda: self.handle_view_clicked(self._view_value)
            )
            self.view_button.pack(side=tk.RIGHT, padx=(4, 0))
            
            display_value = str(last_value)
            self.value_label = ttk.Label(value_frame, text=display_value, anchor=tk.E, justify=tk.RIGHT)
            if len(display_value) < 25:
                self.value_label.pack(side=tk.RIGHT, fill=tk.X, expand=True)
                
        else:
            display_value = self._truncate_for_display(last_value, is_struct_or_string)
            self.value_label = ttk.Label(value_frame, text=display_value, anchor=tk.E, justify=tk.RIGHT)
            self.value_label.pack(side=tk.RIGHT, fill=tk.X, expand=True)

        self.edit_icon = context.icons['settings'] if context and context.icons and 'settings' in context.icons else None
        self.edit_button = tk.Button(
            self, text='Edit', image=self.edit_icon, borderwidth=0, command=self.handle_edit_clicked)
        self.edit_button.grid(row=1, column=2, sticky=tk.E)

        self.grid_columnconfigure(0, weight=10)
        self.grid_columnconfigure(1, weight=10)
        self.grid_columnconfigure(2, weight=1, minsize=40)
        self.grid_columnconfigure((0, 1, 2), uniform='uniform')

    def _check_chartable(self):
        if not daq.ISignal.can_cast_from(self.output_signal):
            return False
        signal = daq.ISignal.cast_from(self.output_signal)
        domain = signal.domain_signal
        if domain is None:
            return False
        return OutputSignalGraph._is_chartable(
            signal.descriptor, domain.descriptor)

    def _toggle_expand(self):
        if self._expanded:
            self._collapse()
        else:
            self._expand()

    def _expand(self):
        self._expanded = True
        if self._arrow_label is not None:
            self._arrow_label.config(image=self._arrow_down)

        # Give column 0 room for the duration controls
        self.grid_columnconfigure(0, weight=10, uniform='')
        self.grid_columnconfigure(1, weight=1, uniform='')
        self.grid_columnconfigure(2, weight=0, uniform='')

        self._dur_label.pack(side=tk.LEFT, padx=(12, 0))
        self._dur_cb.pack(side=tk.LEFT, padx=(4, 0))

        self._preview = OutputSignalGraph(
            self, self.output_signal, self.context,
            duration_var=self._duration_var)
        self._preview.grid(
            row=2, column=0, columnspan=3, sticky=tk.NSEW, pady=(4, 0))
        
        self._dur_cb.bind('<<ComboboxSelected>>', self._preview._on_duration_committed)
        self._dur_cb.bind('<Return>', self._preview._on_duration_committed)
        self._dur_cb.bind('<FocusOut>', self._preview._on_duration_committed)

    def _collapse(self):
        self._expanded = False
        if self._arrow_label is not None:
            self._arrow_label.config(image=self._arrow_right)

        self._dur_label.pack_forget()
        self._dur_cb.pack_forget()

        # Restore the even column layout
        self.grid_columnconfigure(0, weight=10, uniform='uniform')
        self.grid_columnconfigure(1, weight=10, uniform='uniform')
        self.grid_columnconfigure(2, weight=1, uniform='uniform')

        if self._preview is not None:
            self._preview.destroy()
            self._preview = None
            
    def hide_bottom_border(self):
        self._show_bottom_sep = False
        self._bottom_sep.grid_remove()

    def refresh(self):
        last_value, raw_value = self._read_values()
        is_viewable = (isinstance(raw_value, daq.IBaseObject) and IStruct.can_cast_from(raw_value)) or isinstance(raw_value, str)
        display_value = self._truncate_for_display(last_value, is_viewable)
        self.value_label.config(text=display_value)
        self._view_value = raw_value if (isinstance(raw_value, daq.IBaseObject) and IStruct.can_cast_from(raw_value)) else last_value
        if self.view_button is not None:
            self.view_button.configure(command=lambda: self.handle_view_clicked(self._view_value))
            
    @staticmethod
    def _truncate_for_display(value, is_viewable):
        """For values that have a View button, truncate the inline label
        to prevent layout overflow. Numeric/timestamp values pass through unchanged."""
        if not is_viewable:
            return str(value)
        text = str(value)
        max_len = 32
        if len(text) > max_len:
            return text[:max_len] + '...'
        return text

    def _read_values(self):
        # Both reads can raise RuntimeError when the device can not handle 200ms pings for last value
        try:
            last_value = utils.get_last_value_for_signal(self.output_signal)
        except RuntimeError:
            last_value = None

        raw_value = None
        try:
            if self.output_signal is not None and daq.ISignal.can_cast_from(self.output_signal):
                raw_value = daq.ISignal.cast_from(self.output_signal).last_value
        except RuntimeError:
            raw_value = None
        return last_value, raw_value

    def handle_edit_clicked(self):
        if self.output_signal is not None:
            AttributesDialog(self, 'Attributes', self.output_signal, self.context).show()

    def handle_view_clicked(self, value):
        if self.context is None:
            return
        
        if isinstance(value, str):
            # For strings, show in a simple text dialog
            dialog = Dialog(self, 'View Value', self.context)
            dialog.geometry('600x400')
            text_widget = tk.Text(dialog, wrap=tk.WORD)
            text_widget.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
            text_widget.insert('1.0', value)
            
            # Make read-only but allow selection and copying
            def prevent_edit(event):
                # Allow standard copy shortcuts (Ctrl+C, Ctrl+Insert) and navigation
                if event.state & 0x4 and event.keysym.lower() in ('c', 'insert'):  # Ctrl+C or Ctrl+Insert
                    return None
                if event.keysym in ('Left', 'Right', 'Up', 'Down', 'Home', 'End', 'Prior', 'Next'):
                    return None
                if event.keysym in ('Control_L', 'Control_R', 'Shift_L', 'Shift_R'):
                    return None
                return "break"
            
            text_widget.bind('<Key>', prevent_edit)
            text_widget.bind('<Button-1>', lambda e: text_widget.focus_set())
            
            # Add copy functionality via context menu
            def handle_copy():
                try:
                    selected_text = text_widget.selection_get()
                    text_widget.clipboard_clear()
                    text_widget.clipboard_append(selected_text)
                except tk.TclError:
                    # No selection, copy all
                    text_widget.clipboard_clear()
                    text_widget.clipboard_append(value)
            
            def copy_all():
                text_widget.clipboard_clear()
                text_widget.clipboard_append(value)
            
            def show_menu(event):
                menu = tk.Menu(text_widget, tearoff=0)
                menu.add_command(label='Copy', command=handle_copy)
                menu.add_command(label='Copy All', command=copy_all)
                menu.tk_popup(event.x_root, event.y_root)
            
            text_widget.bind('<Button-3>', show_menu)
            
            dialog.show()
        elif isinstance(value, daq.IBaseObject) and IStruct.can_cast_from(value):
            # Structs
            struct = IStruct.cast_from(value)
            dialog = Dialog(self, 'View Struct', self.context)
            dialog.geometry('600x800')
            tree_frame = ttk.Frame(dialog)
            tree_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
            tree = ttk.Treeview(tree_frame, columns=('value',), show='tree headings')
            tree.heading('#0', text='Field')
            tree.heading('value', text='Value')
            tree.column('#0', anchor=tk.W, minwidth=200)
            tree.column('value', anchor=tk.W, minwidth=200)

            scroll_bar = ttk.Scrollbar(tree_frame, orient=tk.VERTICAL, command=tree.yview)
            tree.configure(yscrollcommand=scroll_bar.set)
            scroll_bar.pack(side=tk.RIGHT, fill=tk.Y)

            if hasattr(struct, 'as_dictionary'):
                for key, val in struct.as_dictionary.items():
                    tree.insert('', tk.END, text=key, values=(str(val),))

            tree.pack(fill=tk.BOTH, expand=True)
            
            # Add copy functionality
            def handle_copy():
                selected_item = utils.treeview_get_first_selection(tree)
                if selected_item is None:
                    return
                item = tree.item(selected_item)
                tree.clipboard_clear()
                value = '' if len(item['values']) == 0 else item['values'][0]
                tree.clipboard_append(str(value).strip())
            
            def show_menu(event):
                utils.treeview_select_item(tree, event)
                menu = tk.Menu(tree, tearoff=0)
                menu.add_command(label='Copy', command=handle_copy)
                menu.tk_popup(event.x_root, event.y_root)
            
            tree.bind('<Button-3>', show_menu)
            
            dialog.show()
