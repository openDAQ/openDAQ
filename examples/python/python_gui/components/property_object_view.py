import tkinter as tk
from tkinter import ttk
from opendaq import IPropertyObject, CoreType
from app_context import AppContext
from .properties import (
    make_property_view,
    PropertyView,
)


class PropertyObjectView(ttk.Treeview):
    def __init__(self, parent: tk.Frame, context: AppContext, property_object: IPropertyObject, expand_tree: bool = True):
        super().__init__(parent, columns=("value",), show='tree headings')
        self.context = context
        self.property_object = property_object

        self.heading('#0', text='Property')
        self.heading('value', text='Value')

        self.tag_configure('readonly', foreground='gray')

        self._item_to_view: dict[str, PropertyView] = {}

        self.bind("<Double-1>", self.on_double_click)
        self.bind("<Button-3>", self.on_right_click)

        self.build_tree('', self.property_object)

        if expand_tree:
            self.expand_all('')

    def build_tree(self, parent_id, property_obj: IPropertyObject):
        props = property_obj.all_properties if self.context.show_invisible_components else property_obj.visible_properties

        for prop in props:
            view = make_property_view(prop)
            tag = 'readonly' if prop.read_only else 'editable'

            item_id = self.insert(
                parent_id, 'end',
                text=prop.name,
                values=(view.format_value(),),
                tags=(tag,),
                open=(prop.value_type in (CoreType.ctObject, CoreType.ctDict, CoreType.ctList))
            )
            self._item_to_view[item_id] = view

            # Делегируем построение детей самим классам
            view.build_tree(self, item_id)

    def expand_all(self, item_id: str = ''):
        for child in self.get_children(item_id):
            self.item(child, open=True)
            self.expand_all(child)

    def _close_editor(self, commit: bool):
        if not self.context.editor_state.is_active():
            return

        state = self.context.editor_state
        # Save editor reference before clearing
        editor = state.editor
        item_id = state.item_id
        view = state.view

        self.context.editor_state.clear()

        try:
            if item_id and view:
                # Delegate editor closing to view class
                view.close_editor(self, item_id, editor, commit)
        except Exception:
            pass
        finally:
            try:
                editor.destroy()
            except Exception:
                pass

    def _start_editor(self, item_id: str, view: PropertyView):
        # Ensure only one editor is active (close previous)
        self._close_editor(commit=True)

        bbox = self.bbox(item_id, column="value")
        if not bbox:
            return
        x, y, w, h = bbox

        editor = view.create_editor(self)
        editor.place(x=x, y=y, width=w, height=h)
        editor.focus_set()

        self.context.editor_state.set(editor, item_id, view)

        editor.bind("<Return>", lambda e: self._close_editor(commit=True))
        editor.bind("<FocusOut>", lambda e: self._close_editor(commit=True))
        editor.bind("<Escape>", lambda e: self._close_editor(commit=False))

    def on_double_click(self, event):
        item_id = self.identify_row(event.y)
        col = self.identify_column(event.x)
        if not item_id:
            return

        view = self._item_to_view.get(item_id)
        if not view or not view.editable():
            return

        # Close previous editor
        self._close_editor(commit=True)

        # Give view a chance to handle double click
        editor = view.handle_double_click(self, item_id, col)
        if editor:
            # View created and placed editor
            self.context.editor_state.set(editor, item_id, view)
            editor.bind("<Return>", lambda e: self._close_editor(commit=True))
            editor.bind("<FocusOut>", lambda e: self._close_editor(commit=True))
            editor.bind("<Escape>", lambda e: self._close_editor(commit=False))
            return

        # Standard handling: open editor only for value column
        if col == "#1":
            self._start_editor(item_id, view)

    def on_right_click(self, event):
        item_id = self.identify_row(event.y)
        if not item_id:
            return

        view = self._item_to_view.get(item_id)
        if not view:
            return

        # Give view a chance to handle right click
        view.handle_right_click(self, item_id, event)
