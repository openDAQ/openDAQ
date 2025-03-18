import tkinter as tk
from tkinter import ttk

import opendaq as daq

from .. import utils
from ..app_context import AppContext

class DataDescriptorTreeview(ttk.Treeview):
    def __init__(self, parent, data_descriptor : daq.IDataDescriptor = None, context: AppContext = None, **kwargs):
        ttk.Treeview.__init__(self, parent, columns=('value', 'access', *context.metadata_fields), show='tree headings', **kwargs)

        self.context = context

        scroll_bar = ttk.Scrollbar(
            self, orient=tk.VERTICAL, command=self.yview)
        self.configure(yscrollcommand=scroll_bar.set)
        scroll_bar.pack(side=tk.RIGHT, fill=tk.Y)
        scroll_bar_x = ttk.Scrollbar(
            self, orient=tk.HORIZONTAL, command=self.xview)
        self.configure(xscrollcommand=scroll_bar_x.set)
        scroll_bar_x.pack(side=tk.BOTTOM, fill=tk.X)
        self.pack(fill=tk.BOTH, expand=True)

        # define headings
        self.heading('#0', anchor=tk.W, text='Name')
        self.heading('#1', anchor=tk.W, text='Value')
        # layout
        self.column('#0', anchor=tk.W, minwidth=100, width=100)
        self.column('#1', anchor=tk.W, minwidth=100, width=300)

        for name in utils.get_attributes_of_node(data_descriptor):
            value = getattr(data_descriptor, name)
            self.fill_tree(name, value, '')

    def fill_tree(self, key, value, parent=''):
        display_value = value
        # for user to see property value without expanding the tree
        if isinstance(value, daq.IProperty):
            display_value = value.value
            if value.value_type == daq.CoreType.ctBool:
                display_value = utils.yes_no[display_value]
        else:
            display_value = utils.metadata_converters[key](
                value) if key in utils.metadata_converters else value
        id = self.insert(parent, tk.END, text=str(key), values=(str(display_value),))

        # displaying Nones but not traversing further
        if value is None:
            return

        if isinstance(value, daq.IPropertyObject):
            for property in value.all_properties if self.context.view_hidden_components else value.visible_properties:
                self.fill_tree(property.name, property, id)
        elif isinstance(value, daq.IList):
            try:
                for i, item in enumerate(value):
                    self.fill_tree(i, item, id)
            except Exception:
                pass
        elif isinstance(value, daq.IDict):
            try:
                for k, v in value.items():
                    self.fill_tree(k, v, id)
            except Exception:
                pass
        elif issubclass(type(value), daq.IBaseObject):
            for name in utils.get_attributes_of_node(value):
                v = getattr(value, name)
                self.fill_tree(name, v, id)