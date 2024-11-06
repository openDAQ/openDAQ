import tkinter as tk
from tkinter import ttk
from tkinter import simpledialog

import opendaq as daq

from .. import utils
from .dialog import Dialog
from ..app_context import AppContext
from ..event_port import EventPort


class EditContainerPropertyDialog(Dialog):
    def __init__(self, parent, property: daq.IProperty, context: AppContext, **kwargs):
        super().__init__(parent, 'Edit: ' + property.name, context, **kwargs)
        self.property = property
        self.data = property.value

        self.event_port = EventPort(parent)

        self.geometry('{}x{}'.format(
            800 * self.context.ui_scaling_factor, 600 * self.context.ui_scaling_factor))

        item_types = []
        if isinstance(self.data, daq.IDict):
            item_types.append(str(self.property.key_type))
        if type(self.data) in (daq.IList, daq.IDict):
            item_types.append(str(self.property.item_type))
        item_types = f'<{", ".join(item_types)}>' if item_types else ''

        ttk.Label(self, text=f'{property.name}{item_types}:', anchor=tk.W).pack(
            fill=tk.X, padx=10, pady=5)

        # List
        list_frame = ttk.Frame(self)

        self.listbox = tk.Listbox(list_frame)
        self.listbox.pack(fill=tk.BOTH, expand=True, side=tk.LEFT)

        scroll_bar = ttk.Scrollbar(
            list_frame, orient=tk.VERTICAL, command=self.listbox.yview)
        self.listbox.configure(yscroll=scroll_bar.set)
        scroll_bar.pack(fill=tk.Y, side=tk.RIGHT)

        list_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)

        # Upper buttons
        button_frame_top = ttk.Frame(self)

        self.add_button = ttk.Button(
            button_frame_top, text='Add', command=self.add_item, width=10)
        self.add_button.pack(
            padx=10, side=tk.LEFT)
        self.edit_button = ttk.Button(
            button_frame_top, text='Edit', command=self.edit_item, width=10)
        self.edit_button.pack(
            padx=10, side=tk.LEFT)
        self.delete_button = ttk.Button(
            button_frame_top, text='Delete', command=self.delete_item, width=10)
        self.delete_button.pack(
            padx=10, side=tk.LEFT)
        self.clear_button = ttk.Button(
            button_frame_top, text='Clear', command=self.clear, width=10)
        self.clear_button.pack(
            padx=10, side=tk.LEFT)

        button_frame_top.pack(anchor=tk.E, pady=10)

        # Bottom buttons
        button_frame_bottom = ttk.Frame(self)

        self.set_button = ttk.Button(
            button_frame_bottom, text='Set', command=self.set_property, width=10)
        self.set_button.pack(
            padx=10, side=tk.LEFT)
        self.cancel_button = ttk.Button(
            button_frame_bottom, text='Close', command=self.close, width=10)
        self.cancel_button.pack(
            padx=10, side=tk.LEFT)

        button_frame_bottom.pack(anchor=tk.E, pady=10)

        # State

        self.protocol('WM_DELETE_WINDOW', self.close)

        if type(self.data) not in (daq.IList, daq.IDict):
            self.listbox['state'] = tk.DISABLED
            self.add_button['state'] = tk.DISABLED
            self.edit_button['state'] = tk.DISABLED
            self.delete_button['state'] = tk.DISABLED
            self.set_button['state'] = tk.DISABLED
            self.cancel_button.focus_set()
        else:
            self.listbox.bind('<Insert>', lambda e: self.add_item())
            self.listbox.bind('<Return>', lambda e: self.edit_item())
            self.listbox.bind('<Delete>', lambda e: self.delete_item())
            self.listbox.focus_set()

        self.fill()

    def fill(self):
        if self.data is None or type(self.data) not in (daq.IList, daq.IDict):
            return

        self.listbox.delete(0, tk.END)

        try:
            if isinstance(self.data, daq.IList):
                for item in self.data:
                    self.listbox.insert(tk.END, str(item))
            elif isinstance(self.data, daq.IDict):
                self.data_index = []
                for key, value in self.data.items():
                    self.data_index.append(key)
                    self.listbox.insert(tk.END, f'{str(key)} : {str(value)}')
        except RuntimeError as e:
            utils.show_error(
                'Display error', f'Can\'t display data: {e}', self)

        self.listbox.focus_set()
        self.listbox.select_set(tk.END)

    def add_item(self):
        if isinstance(self.data, daq.IList):
            value = simpledialog.askstring('Add item', 'Value:', parent=self)
            if value is None:
                return
            try:
                self.data.append(utils.value_to_coretype(
                    value, self.property.item_type))
                self.fill()
            except Exception as e:
                utils.show_error('Add item error',
                                 f'Can\'t add item: {e}', self)
        elif isinstance(self.data, daq.IDict):
            key = simpledialog.askstring('Add item', 'Key:', parent=self)
            if key is None:
                return
            self.update()  # it is here for next dialog to be able to grab focus
            value = simpledialog.askstring('Add item', 'Value:', parent=self)
            if value is None:
                return
            try:
                self.data[utils.value_to_coretype(key, self.property.key_type)] = utils.value_to_coretype(
                    value, self.property.item_type)
                self.fill()
            except Exception as e:
                utils.show_error('Add item error',
                                 f'Can\'t add item: {e}', self)

    def edit_item(self):
        selection = self.listbox.curselection()
        if selection:
            if isinstance(self.data, daq.IList):
                new_value = simpledialog.askstring(
                    'Edit item', 'Value:', parent=self, initialvalue=self.data[selection[0]])
                if new_value:
                    try:
                        self.data[selection[0]] = utils.value_to_coretype(
                            new_value, self.property.item_type)
                        self.fill()
                    except Exception as e:
                        utils.show_error('Edit item error',
                                         f'Can\'t edit item: {e}', self)
            elif isinstance(self.data, daq.IDict):
                key = self.data_index[selection[0]]
                new_value = simpledialog.askstring(
                    'Edit item', 'Value:', parent=self, initialvalue=self.data[key])
                if new_value:
                    try:
                        self.data[utils.value_to_coretype(key, self.property.key_type)] = utils.value_to_coretype(
                            new_value, self.property.item_type)
                        self.fill()
                    except Exception as e:
                        utils.show_error('Edit item error',
                                         f'Can\'t edit item: {e}', self)

    def delete_item(self):
        selection = self.listbox.curselection()
        if selection:
            if isinstance(self.data, daq.IList):
                del self.data[selection[0]]
                self.fill()
            elif isinstance(self.data, daq.IDict):
                del self.data[self.data_index[selection[0]]]
                self.fill()

    def clear(self):
        if type(self.data) in (daq.IList, daq.IDict):
            self.data.clear()
            self.fill()

    def set_property(self):
        if self.property.value != self.data:
            self.property.value = self.data
            self.event_port.emit()

    def close(self):
        self.destroy()
