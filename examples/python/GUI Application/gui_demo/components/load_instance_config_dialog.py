import tkinter as tk
from tkinter import ttk
from tkinter.filedialog import askopenfile

import opendaq as daq

from .. import utils
from ..app_context import AppContext
from ..event_port import EventPort
from .dialog import Dialog
from .edit_container_property import EditContainerPropertyDialog


class LoadInstanceConfigDialog(Dialog):
    def __init__(self, parent, context: AppContext):
        super().__init__(parent, 'Load configuration', context)
        self.event_port = EventPort(self, event_callback=self.on_refresh_event)
        self.geometry('{}x{}'.format(
            1200 * self.context.ui_scaling_factor, 600 * self.context.ui_scaling_factor))

        self.protocol('WM_DELETE_WINDOW', self.cancel)

        button_frame = ttk.Frame(self)
        button_frame.pack(side=tk.BOTTOM, anchor=tk.E, pady=10)

        ttk.Button(button_frame, text='Load configuration', command=self.load_configuration).pack(
            padx=10, ipadx=20 * self.context.ui_scaling_factor, side=tk.LEFT)
        ttk.Button(button_frame, text='Cancel', command=self.cancel).pack(
            padx=10, ipadx=10 * self.context.ui_scaling_factor, side=tk.LEFT)

        self.tree_frame = ttk.Frame(self)

        self.tree = ttk.Treeview(self.tree_frame, columns=(
            'value',), show='tree headings')
        self.tree.heading('#0', anchor=tk.W, text='Property')
        self.tree.heading('value', anchor=tk.W, text='Value')
        self.tree.column('#0', anchor=tk.W)
        self.tree.column('value', anchor=tk.W)
        self.tree.pack(fill=tk.BOTH, expand=True, side=tk.LEFT)

        scroll_bar = ttk.Scrollbar(
            self.tree_frame, orient=tk.VERTICAL, command=self.tree.yview)
        self.tree.configure(yscrollcommand=scroll_bar.set)
        scroll_bar.pack(side=tk.RIGHT, fill=tk.Y)

        self.tree_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)

        self.updata_params = daq.UpdateParameters()
        self.display_config_options('', self.updata_params)

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

    # after pressing enter or if entry is out of focus it changes property value
    def update_property_value(self, path, new_value):
        def update_property(context, path, new_value, depth=0):
            for property in self.context.properties_of_component(context):
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

        update_property(self.updata_params, path, new_value)

    def edit_value(self, event):
        item_id = self.tree.selection()[0]
        path = utils.get_item_path(self.tree, item_id)
        prop = utils.get_property_for_path(
            self.context, path, self.updata_params)
        if prop.value_type in (daq.CoreType.ctDict, daq.CoreType.ctList):
            EditContainerPropertyDialog(self, prop, self.updata_params).show()
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

    def load_configuration(self):
        file = askopenfile(parent=self, initialfile='config.json', title='Load configuration',
                           defaultextension='.json', filetypes=[('All Files', '*.*'), ('Json', '*.json')])
        if file is None:
            return
        config_string = file.read()
        file.close()

        self.context.instance.load_configuration(
            config_string, self.updata_params)
        self.destroy()

    def cancel(self):
        self.updata_params = None
        self.destroy()

    def on_refresh_event(self, event):
        self.display_config_options('', self.updata_params)
