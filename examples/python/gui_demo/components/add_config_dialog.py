import tkinter as tk
from tkinter import ttk

import opendaq as daq

from .. import utils
from ..app_context import AppContext
from ..event_port import EventPort
from .dialog import Dialog
from .edit_container_property import EditContainerPropertyDialog


class AddConfigDialog(Dialog):
    def __init__(self, parent, context: AppContext, connection_string=None):
        super().__init__(parent, 'Add with config', context)
        self.connection_string = connection_string
        self.event_port = EventPort(self, event_callback=self.on_refresh_event)
        self.geometry('{}x{}'.format(
            1200 * self.context.ui_scaling_factor, 600 * self.context.ui_scaling_factor))

        self.protocol('WM_DELETE_WINDOW', self.cancel)

        ttk.Label(self, text=self.connection_string).pack(
            pady=10, padx=10, anchor=tk.NW, side=tk.TOP)

        button_frame = ttk.Frame(self)
        button_frame.pack(side=tk.BOTTOM, anchor=tk.E, pady=10)

        ttk.Button(button_frame, text='Add', command=self.add).pack(
            padx=10, ipadx=20 * self.context.ui_scaling_factor, side=tk.LEFT)
        ttk.Button(button_frame, text='Cancel', command=self.cancel).pack(
            padx=10, ipadx=10 * self.context.ui_scaling_factor, side=tk.LEFT)

        tree_frame = ttk.Frame(self)

        self.tree = ttk.Treeview(tree_frame, columns=(
            'value',), show='tree headings')
        self.tree.heading('#0', anchor=tk.W, text='Property')
        self.tree.heading('value', anchor=tk.W, text='Value')
        self.tree.column('#0', anchor=tk.W)
        self.tree.column('value', anchor=tk.W)
        self.tree.bind('<Double-1>', self.edit_value)

        tree_scroll = ttk.Scrollbar(
            tree_frame, orient=tk.VERTICAL, command=self.tree.yview)
        self.tree.configure(yscrollcommand=tree_scroll.set)

        self.tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        tree_scroll.pack(side=tk.RIGHT, fill=tk.Y)
        tree_frame.pack(fill=tk.BOTH, expand=True)

        self.device_config = context.instance.create_default_add_device_config()
        self.display_config_options('', self.device_config)

    # initial display of properties

    def display_config_options(self, parent_node, prop_object):
        def printed_value(value_type, value):
            if value_type == daq.CoreType.ctBool:
                return utils.yes_no[value]
            else:
                return value

        if not parent_node:
            self.tree.delete(*self.tree.get_children())

        for property in self.context.properties_of_component(prop_object):
            prop = prop_object.get_property_value(property.name)
            if isinstance(prop, daq.IBaseObject) and daq.IPropertyObject.can_cast_from(prop):
                casted_property = daq.IPropertyObject.cast_from(prop)
                node_id = self.tree.insert(
                    parent_node, tk.END, text=property.name, open=True)
                self.display_config_options(node_id, casted_property)
            else:
                property_value = printed_value(
                    property.item_type, prop_object.get_property_value(property.name))
                self.tree.insert(parent_node, tk.END,
                                 text=property.name, values=(property_value,))
                self.tree.bind('<Double-1>', self.edit_value)

    def save_value(self, entry, item_id, column, path):
        new_value = entry.get()
        self.tree.set(item_id, column, new_value)
        entry.destroy()
        self.update_property_value(path, new_value)

    # after pressing enter or if entry is out of focus it changes property
    # value
    def update_property_value(self, path, new_value):
        def update_property(context, path, new_value, depth=0):
            for property in self.context.properties_of_component(context):
                if property.name == path[depth]:
                    if depth == len(path) - 1:
                        eval_result = daq.EvalValue(new_value).result
                        context.set_property_value(property.name, eval_result)
                        return
                    prop = context.get_property_value(property.name)
                    if isinstance(prop, daq.IBaseObject) and daq.IPropertyObject.can_cast_from(prop):
                        casted_property = daq.IPropertyObject.cast_from(prop)
                        update_property(casted_property, path,
                                        new_value, depth + 1)

        update_property(self.device_config, path, new_value)

    def edit_value(self, event):
        item_id = self.tree.selection()[0]
        path = utils.get_item_path(self.tree, item_id)
        prop = utils.get_property_for_path(
            self.context, path, self.device_config)
        if prop.value_type in (daq.CoreType.ctDict, daq.CoreType.ctList):
            EditContainerPropertyDialog(self, prop, self.context).show()
        elif prop.value_type == daq.CoreType.ctBool:
            column = self.tree.identify_column(event.x)
            if column == '#1':
                prop.value = not prop.value
                self.tree.set(item_id, column, str(prop.value))
        else:
            column = self.tree.identify_column(event.x)
            if column == '#1':
                x, y, width, height = self.tree.bbox(item_id, column)
                value = self.tree.set(item_id, column)
                entry = ttk.Entry(self.tree)
                entry.place(x=x, y=y, width=width, height=height)
                entry.insert(0, value)
                entry.focus()
                entry.bind('<Return>', lambda e: self.save_value(
                    entry, item_id, column, path))
                entry.bind('<FocusOut>', lambda e: self.save_value(
                    entry, item_id, column, path))

    def add(self):
        self.destroy()

    def cancel(self):
        self.device_config = None
        self.destroy()

    def on_refresh_event(self, event):
        self.display_config_options('', self.device_config)
