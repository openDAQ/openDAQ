import tkinter as tk
from tkinter import ttk

import opendaq as daq

from .. import utils
from ..event_port import EventPort
from ..app_context import AppContext
from .dialog import Dialog


class MetadataFieldsSelectorDialog(Dialog):
    def __init__(self, parent, context: AppContext):
        Dialog.__init__(self, parent, 'Metadata fields to display', context)
        self.event_port = EventPort(parent)
        self.fields = []
        try:
            my_prop = daq.StringProperty(daq.String(
                'MyString'), daq.String('foo'), daq.Boolean(True))
            self.fields = utils.get_attributes_of_node(my_prop)
            self.fields.remove('name')
            self.fields.remove('value')
        except:
            pass

        self.protocol('WM_DELETE_WINDOW', self.cancel)

        self.geometry(f'{400}x{600}')

        tree_frame = ttk.Frame(self)
        self.tree = ttk.Treeview(tree_frame)
        scroll_bar = ttk.Scrollbar(
            tree_frame, orient=tk.VERTICAL, command=self.tree.yview)
        self.tree.configure(yscrollcommand=scroll_bar.set)
        scroll_bar.pack(side=tk.RIGHT, fill=tk.Y)
        self.tree.pack(fill=tk.BOTH, expand=True)
        tree_frame.pack(fill=tk.BOTH, expand=True)

        # define headings
        self.tree.heading('#0', anchor=tk.W, text='Fields')
        # layout
        self.tree.column('#0', anchor=tk.W, minwidth=100)

        bottom_button_frame = ttk.Frame(self)
        ttk.Button(bottom_button_frame, text='OK',
                   command=self.ok).pack(padx=10, side=tk.LEFT)
        ttk.Button(bottom_button_frame, text='Cancel',
                   command=self.cancel).pack(padx=10, side=tk.LEFT)
        bottom_button_frame.pack(side=tk.BOTTOM, anchor=tk.E, pady=(5, 0))

        upper_button_frame = ttk.Frame(self)
        ttk.Button(upper_button_frame, text='Reset',
                   command=self.reset_selection).pack(padx=10, side=tk.LEFT)
        ttk.Button(upper_button_frame, text='Deselect all',
                   command=self.clean_selection).pack(padx=10, side=tk.LEFT)
        upper_button_frame.pack(side=tk.BOTTOM, anchor=tk.E, pady=(5, 0))

        for field in self.fields:
            self.tree.insert('', 'end', text=utils.snake_case_to_title(field))
            if field in self.context.metadata_fields:
                self.tree.selection_add(self.tree.get_children()[-1])

    def ok(self):
        self.context.metadata_fields = [utils.title_to_snake_case(self.tree.item(
            item, 'text')) for item in self.tree.selection()]
        self.event_port.emit()
        self.destroy()

    def cancel(self):
        self.destroy()

    def reset_selection(self):
        self.clean_selection()
        for item in self.tree.get_children():
            if self.tree.item(item, 'text') in self.context.metadata_fields:
                self.tree.selection_add(item)

    def clean_selection(self):
        self.tree.selection_remove(*self.tree.selection())
