"""Base class for collection property views (Dict, List, Struct)"""
import tkinter as tk
from tkinter import ttk
from .base import PropertyView
from abc import abstractmethod


class CollectionPropertyView(PropertyView):
    """Base class for properties that contain multiple items (Dict, List, Struct)"""

    def __init__(self, prop, context=None):
        super().__init__(prop, context)
        self._item_map = {}  # item_id -> item_key/index/field

    # ========== Abstract methods to implement in subclasses ==========

    @abstractmethod
    def get_items(self):
        """Return iterable of (display_key, value) tuples"""
        pass

    @abstractmethod
    def format_collection_value(self) -> str:
        """Format the collection summary (e.g., 'Dict (3 items)')"""
        pass

    @abstractmethod
    def create_item_editor(self, parent, item_id: str) -> tk.Widget:
        """Create editor widget for item"""
        pass

    @abstractmethod
    def apply_item_change(self, item_id: str, new_value_text: str):
        """Apply changed value to the collection"""
        pass

    @abstractmethod
    def format_item_value(self, item_id: str) -> str:
        """Format item value for display"""
        pass

    # ========== Optional methods ==========

    def supports_add_item(self) -> bool:
        """Whether this collection supports adding items"""
        return False

    def supports_delete_item(self) -> bool:
        """Whether this collection supports deleting items"""
        return False

    def add_item(self, tree, parent_id):
        """Add new item to collection"""
        pass

    def delete_item(self, tree, item_id):
        """Delete item from collection"""
        pass

    # ========== Common implementation ==========

    def format_value(self) -> str:
        """Default implementation delegates to format_collection_value"""
        return self.format_collection_value()

    def is_collection_item(self, item_id: str) -> bool:
        """Check if item_id belongs to this collection's items"""
        return item_id in self._item_map

    def get_item_key(self, item_id: str):
        """Get the key/index/field for this item"""
        return self._item_map.get(item_id)

    def build_tree(self, tree, parent_id, current_path: str = ""):
        """Build tree items for collection entries"""
        try:
            items = self.get_items()
            if not items:
                return

            tag = 'readonly' if not self.editable() else 'editable'
            for display_key, value in items:
                item_id = tree.insert(
                    parent_id, 'end',
                    text=str(display_key),
                    values=(str(value),),
                    tags=(tag,)
                )
                tree._item_to_view[item_id] = self
                self._item_map[item_id] = display_key
        except Exception as e:
            print(f"Error building tree for collection: {e}")

    def handle_double_click(self, tree, item_id, column):
        """Handle double click on collection item"""
        if not self.is_collection_item(item_id):
            return None

        # Only edit value column for items
        if column == "#0" and not self.can_edit_key():
            return None

        # Determine which column was clicked
        if column == "#0":
            bbox = tree.bbox(item_id, column="#0")
            edit_column = 'key'
        else:
            bbox = tree.bbox(item_id, column="value")
            edit_column = 'value'

        if not bbox:
            return None

        # Store what we're editing
        if not hasattr(tree, '_collection_edit_column'):
            tree._collection_edit_column = {}
        tree._collection_edit_column[item_id] = edit_column

        # Create editor
        x, y, w, h = bbox
        editor = self.create_item_editor(tree, item_id)
        editor.place(x=x, y=y, width=w, height=h)
        editor.focus_set()

        return editor

    def can_edit_key(self) -> bool:
        """Whether keys/indices can be edited (default: no)"""
        return False

    def close_editor(self, tree, item_id, editor, commit: bool):
        """Close editor for collection item"""
        if not self.is_collection_item(item_id):
            super().close_editor(tree, item_id, editor, commit)
            return

        if commit:
            text = self.get_editor_text(editor)
            self.apply_item_change(item_id, text)
            tree.set(item_id, "value", self.format_item_value(item_id))

        # Clear edit column info
        if hasattr(tree, '_collection_edit_column') and item_id in tree._collection_edit_column:
            del tree._collection_edit_column[item_id]

    def handle_right_click(self, tree, item_id, event):
        """Handle right click on collection or item"""
        if not self.editable():
            return False

        # Check if this is an item
        if self.is_collection_item(item_id):
            return self._show_item_context_menu(tree, item_id, event)

        # Check if this is the collection itself
        view = tree._item_to_view.get(item_id)
        if view == self:
            return self._show_collection_context_menu(tree, item_id, event)

        return False

    def _show_item_context_menu(self, tree, item_id, event) -> bool:
        """Show context menu for collection item"""
        menu = tk.Menu(tree, tearoff=0)
        has_items = False

        if self.supports_add_item():
            menu.add_command(label="Add item", command=lambda: self.add_item(tree, tree.parent(item_id)))
            has_items = True

        if self.supports_delete_item():
            menu.add_command(label="Delete", command=lambda: self.delete_item(tree, item_id))
            has_items = True

        if has_items:
            menu.post(event.x_root, event.y_root)
            return True

        return False

    def _show_collection_context_menu(self, tree, item_id, event) -> bool:
        """Show context menu for collection itself"""
        if self.supports_add_item():
            menu = tk.Menu(tree, tearoff=0)
            menu.add_command(label="Add item", command=lambda: self.add_item(tree, item_id))
            menu.post(event.x_root, event.y_root)
            return True
        return False
