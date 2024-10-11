import opendaq as daq
import tkinter as tk
from tkinter import ttk

from ..utils import *
from ..app_context import *
from ..event_port import *
from .dialog import Dialog


class AddConfigDialog(Dialog):
    def __init__(self, parent, context: AppContext, connection_string=None):
        super().__init__(parent, "Add with config", context)
        self.connection_string = connection_string
        self.geometry('{}x{}'.format(
            1200 * self.context.ui_scaling_factor, 600 * self.context.ui_scaling_factor))

        tk.Label(self, text=self.connection_string).pack(
            pady=10, padx=10, anchor=tk.NW, side=tk.TOP)

        button_frame = tk.Frame(self)
        button_frame.pack(side=tk.BOTTOM, anchor=tk.E)

        tk.Button(button_frame, text="Add", command=self.add).pack(
            padx=10, ipadx=20 * self.context.ui_scaling_factor, side=tk.LEFT)
        tk.Button(button_frame, text="Cancel", command=self.cancel).pack(
            padx=10, ipadx=10 * self.context.ui_scaling_factor, side=tk.LEFT)

        self.frame = tk.Frame(self, background="green")
        self.frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)

        self.tree = ttk.Treeview(self.frame, columns=(
            'value',), show='tree headings')
        self.tree.heading('#0', anchor=tk.W, text='Property')
        self.tree.heading('value', anchor=tk.W, text='Value')
        self.tree.column('#0', anchor=tk.W)
        self.tree.column('value', anchor=tk.W)
        self.tree.pack(fill=tk.BOTH, expand=True)

        self.device_config = context.instance.create_default_add_device_config()
        self.display_config_options('', self.device_config)

        self.protocol("WM_DELETE_WINDOW", self.cancel)

    # initial display of properties
    def display_config_options(self, parent_node, context):
        def printed_value(value_type, value):
            if value_type == daq.CoreType.ctBool:
                return yes_no[value]
            else:
                return value

        for property in context.visible_properties:
            prop = context.get_property_value(property.name)
            if isinstance(prop, daq.IBaseObject) and daq.IPropertyObject.can_cast_from(prop):
                casted_property = daq.IPropertyObject.cast_from(prop)
                node_id = self.tree.insert(
                    parent_node, 'end', text=property.name, open=True)
                self.display_config_options(node_id, casted_property)
            else:
                property_value = printed_value(
                    property.item_type, context.get_property_value(property.name))
                self.tree.insert(parent_node, 'end',
                                 text=property.name, values=(property_value,))
                self.tree.bind('<Double-1>', self.edit_value)

    def save_value(self, entry, item_id, column, path):
        new_value = entry.get()
        self.tree.set(item_id, column, new_value)
        entry.destroy()
        self.update_property_value(path, new_value)

    # after pressing enter or if entry is out of focus it changes property value
    def update_property_value(self, path, new_value):
        def update_property(context, path, new_value, depth=0):
            for property in context.visible_properties:
                if property.name == path[depth]:
                    if depth == len(path) - 1:
                        context.set_property_value(
                            property.name, daq.EvalValue(new_value))
                        return
                    prop = context.get_property_value(property.name)
                    if isinstance(prop, daq.IBaseObject) and daq.IPropertyObject.can_cast_from(prop):
                        casted_property = daq.IPropertyObject.cast_from(prop)
                        update_property(casted_property, path,
                                        new_value, depth + 1)

        update_property(self.device_config, path, new_value)

    def edit_value(self, event):
        item_id = self.tree.selection()[0]
        column = self.tree.identify_column(event.x)
        if column == '#1':
            x, y, width, height = self.tree.bbox(item_id, column)
            value = self.tree.set(item_id, column)
            entry = tk.Entry(self.tree)
            entry.place(x=x, y=y, width=width, height=height)
            entry.insert(0, value)
            entry.focus()
            path = self.get_item_path(item_id)
            entry.bind('<Return>', lambda e: self.save_value(
                entry, item_id, column, path))
            entry.bind('<FocusOut>', lambda e: self.save_value(
                entry, item_id, column, path))

    def add(self):
        self.destroy()

    def cancel(self):
        self.device_config = None
        self.destroy()

    # my solution for having multiple properties with the same name
    def get_item_path(self, item_id):
        path = []
        while item_id:
            item_text = self.tree.item(item_id, 'text')
            path.insert(0, item_text)
            item_id = self.tree.parent(item_id)
        return path
