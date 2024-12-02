import tkinter as tk
from tkinter import ttk

import opendaq as daq

from .. import utils
from ..app_context import AppContext
from .dialog import Dialog


class MetadataDialog(Dialog):

    def __init__(self, parent, node: daq.IProperty, context: AppContext, **kwargs):
        super().__init__(parent, f'{node.name} metadata', context, **kwargs)

        self.parent = parent
        self.node = node
        self.context = context

        self.geometry(f'{600}x{800}')
        tree_frame = ttk.Frame(self)

        tree = ttk.Treeview(tree_frame, columns=(
            'value'), show='tree headings')

        scroll_bar = ttk.Scrollbar(
            tree_frame, orient=tk.VERTICAL, command=tree.yview)
        tree.configure(yscrollcommand=scroll_bar.set)
        scroll_bar.pack(side=tk.RIGHT, fill=tk.Y)
        tree.pack(fill=tk.BOTH, expand=True)
        tree_frame.pack(fill=tk.BOTH, expand=True)

        # define headings
        tree.heading('#0', anchor=tk.W, text='Name')
        tree.heading('value', anchor=tk.W, text='Value')
        # layout
        tree.column('#0', anchor=tk.W, minwidth=100, width=120)
        tree.column('value', anchor=tk.W, minwidth=100, width=300)

        tree.bind('<Button-3>', self.handle_right_click)

        self.tree = tree
        self.initial_update_func = self.update

    def update(self):
        self.tree.delete(*self.tree.get_children())

        def fill_tree(key, value, parent=''):

            display_value = value
            # for user to see property value without expanding the tree
            if isinstance(value, daq.IProperty):
                display_value = value.value
                if value.value_type == daq.CoreType.ctBool:
                    display_value = utils.yes_no[display_value]
            else:
                display_value = utils.metadata_converters[key](
                    value) if key in utils.metadata_converters else value
            id = self.tree.insert(
                parent, tk.END, text=str(key), values=(str(display_value), ))

            # displaying Nones but not traversing further
            if value is None:
                return

            if isinstance(value, daq.IPropertyObject):
                for property in value.all_properties if self.context.view_hidden_components else value.visible_properties:
                    fill_tree(property.name, property, id)
            elif isinstance(value, daq.IList):
                try:
                    for i, item in enumerate(value):
                        fill_tree(i, item, id)
                except Exception:
                    pass
            elif isinstance(value, daq.IDict):
                try:
                    for k, v in value.items():
                        fill_tree(k, v, id)
                except Exception:
                    pass
            elif issubclass(type(value), daq.IBaseObject):
                for name in utils.get_attributes_of_node(value):
                    v = getattr(value, name)
                    fill_tree(name, v, id)

        for name in utils.get_attributes_of_node(self.node):
            fill_tree(name, getattr(self.node, name), '')

    def handle_copy(self):
        selected_iid = utils.treeview_get_first_selection(self.tree)
        if selected_iid is None:
            return
        item = self.tree.item(selected_iid)
        self.clipboard_clear()
        self.clipboard_append(item['values'][0])

    def handle_right_click(self, event):
        utils.treeview_select_item(self.tree, event)
        menu = tk.Menu(self, tearoff=0)
        menu.add_command(label='Copy', command=self.handle_copy)
        menu.tk_popup(event.x_root, event.y_root)
