import tkinter as tk
from tkinter import ttk

import opendaq as daq

from .. import utils
from ..event_port import EventPort
from ..app_context import AppContext
from .dialog import Dialog


class MetadataFieldsSelectorDialog(Dialog):
    def __init__(self, parent, context: AppContext):
        Dialog.__init__(self, parent, 'Visible columns', context)
        self.event_port = EventPort(parent)
        self.fields = []
        self._item_to_field = {}
        self._checked_fields = set(context.metadata_fields)
        try:
            builder = daq.StringPropertyBuilder(daq.String(
                'MyString'), daq.String('foo'))
            builder.description = 'bar'
            my_prop = builder.build()
            self.fields = utils.get_attributes_of_node(my_prop)
            self.fields.remove('name')
            self.fields.remove('value')
        except:
            pass

        self.protocol('WM_DELETE_WINDOW', self.cancel)

        self.geometry(f'{int(400 * context.dpi_factor)}x{int(600 * context.dpi_factor)}')

        tree_frame = ttk.Frame(self)
        style = ttk.Style(self)
        selected_bg = style.lookup('Treeview', 'background', ['selected']) or 'SystemHighlight'
        selected_fg = style.lookup('Treeview', 'foreground', ['selected']) or 'SystemHighlightText'
        self.tree = ttk.Treeview(
            tree_frame,
            columns=('visible',),
            show='tree headings',
            selectmode='none'
        )
        scroll_bar = ttk.Scrollbar(
            tree_frame, orient=tk.VERTICAL, command=self.tree.yview)
        self.tree.configure(yscrollcommand=scroll_bar.set)
        scroll_bar.pack(side=tk.RIGHT, fill=tk.Y)
        self.tree.pack(fill=tk.BOTH, expand=True)
        tree_frame.pack(fill=tk.BOTH, expand=True)

        # define headings
        self.tree.heading('#0', anchor=tk.W, text='Columns')
        self.tree.heading('visible', anchor=tk.CENTER, text='Visible')
        # layout
        self.tree.column('#0', anchor=tk.W, minwidth=240, stretch=True)
        self.tree.column('visible', anchor=tk.CENTER, minwidth=80, width=int(80 * context.dpi_factor), stretch=False)
        self.tree.tag_configure('visible', background=selected_bg, foreground=selected_fg)

        self.tree.bind('<Button-1>', self._on_tree_click, add='+')
        self.tree.bind('<Double-1>', self._on_tree_double_click, add='+')

        bottom_button_frame = ttk.Frame(self)
        ttk.Button(bottom_button_frame, text='OK', command=self.ok).pack(side=tk.RIGHT)
        bottom_button_frame.pack(side=tk.BOTTOM, fill=tk.X, padx=10, pady=(5, 0))

        for field in self.fields:
            checked = field in self._checked_fields
            item = self.tree.insert(
                '',
                'end',
                text=utils.snake_case_to_title(field),
                values=('Yes' if checked else 'No',),
                tags=('visible',) if checked else ()
            )
            self._item_to_field[item] = field

    def _set_field_checked(self, item, checked):
        field = self._item_to_field.get(item)
        if field is None:
            return
        if checked:
            self._checked_fields.add(field)
        else:
            self._checked_fields.discard(field)
        self.tree.item(item, values=('Yes' if checked else 'No',), tags=('visible',) if checked else ())

    def _toggle_item(self, item):
        field = self._item_to_field.get(item)
        if field is None:
            return
        self._set_field_checked(item, field not in self._checked_fields)

    def _on_tree_click(self, event):
        item = self.tree.identify_row(event.y)
        if item:
            self._toggle_item(item)

    def _on_tree_double_click(self, event):
        item = self.tree.identify_row(event.y)
        if item:
            self._toggle_item(item)

    def ok(self):
        self.context.metadata_fields = [field for field in self.fields if field in self._checked_fields]
        self.event_port.emit()
        self.destroy()

    def cancel(self):
        self.destroy()
