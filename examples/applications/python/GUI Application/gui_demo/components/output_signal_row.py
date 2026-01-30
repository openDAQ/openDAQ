import tkinter as tk
from tkinter import ttk

import opendaq as daq
from opendaq import IStruct

from .. import utils
from .attributes_dialog import AttributesDialog
from .dialog import Dialog


class OutputSignalRow(ttk.Frame):
    def __init__(self, parent, output_signal, context=None, **kwargs):
        ttk.Frame.__init__(self, parent, **kwargs)
        self.parent = parent
        self.output_signal = output_signal
        self.selection = ''
        self.context = context

        self.configure(padding=(10, 5))

        last_value = utils.get_last_value_for_signal(output_signal)
        ttk.Label(self, text=output_signal.name, anchor=tk.W).grid(
            row=0, column=0, sticky=tk.W)

        # Value column with optional View button
        value_frame = ttk.Frame(self)
        value_frame.grid(row=0, column=1, sticky=tk.EW)
        value_label = ttk.Label(value_frame, text=str(last_value), anchor=tk.W)
        value_label.pack(side=tk.LEFT, fill=tk.X, expand=True)

        is_struct_or_string = False
        if isinstance(last_value, daq.IBaseObject) and IStruct.can_cast_from(last_value) or isinstance(last_value, str):
            is_struct_or_string = True

        if is_struct_or_string:
            view_button = ttk.Button(
                self, text='View', command=lambda: self.handle_view_clicked(last_value))
            view_button.grid(row=0, column=1, sticky=tk.E)

        self.edit_icon = context.icons['settings'] if context and context.icons and 'settings' in context.icons else None
        self.edit_button = tk.Button(
            self, text='Edit', image=self.edit_icon, borderwidth=0, command=self.handle_edit_clicked)
        self.edit_button.grid(row=0, column=2, sticky=tk.E)

        self.grid_columnconfigure(0, weight=10)
        self.grid_columnconfigure(1, weight=10)
        self.grid_columnconfigure(2, weight=1, minsize=40)
        self.grid_columnconfigure((0, 1, 2), uniform='uniform')

    def refresh(self):
        pass

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
