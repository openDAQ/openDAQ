import tkinter as tk
from tkinter import ttk


class PropertyView:
    def __init__(self, prop):
        self.prop = prop

    def editable(self) -> bool:
        return not self.prop.read_only

    def format_value(self) -> str:
        return str(self.prop.value)

    def create_editor(self, parent) -> tk.Widget:
        e = ttk.Entry(parent)
        e.insert(0, str(self.prop.value))
        e.selection_range(0, tk.END)
        return e

    def get_editor_text(self, editor: tk.Widget) -> str:
        return editor.get()

    def parse(self, text: str):
        return text

    def apply_text(self, text: str):
        self.prop.value = self.parse(text)

    def build_tree(self, _tree, _parent_id):
        """Build child items in tree. Override in subclasses if needed."""
        pass

    def handle_double_click(self, tree, item_id, column):
        """
        Handle double click on item.
        Return editor widget if handled, None otherwise.
        Override in subclasses for custom behavior.
        """
        return None

    def close_editor(self, tree, item_id, editor, commit: bool):
        """
        Close editor and apply changes. Override for custom close behavior.
        Default implementation uses apply_text.
        """
        if commit:
            text = self.get_editor_text(editor)
            self.apply_text(text)
            tree.set(item_id, "value", self.format_value())

    def handle_right_click(self, _tree, _item_id, _event):
        """
        Handle right click on item.
        Return True if handled, False otherwise.
        Override in subclasses for custom behavior (e.g., context menus).
        """
        return False
