"""Refactored dict property view using CollectionPropertyView base"""
import tkinter as tk
from tkinter import ttk
from .collection_property import CollectionPropertyView
from .value_editor_helper import (
    create_value_editor, get_editor_value,
    convert_text_to_value, is_enumeration, apply_enumeration_value
)


class DictPropertyView(CollectionPropertyView):
    """Property view for dictionary properties"""

    def get_items(self):
        """Return list of (key, value) tuples"""
        try:
            d = self.prop.value
            if d:
                return [(key, value) for key, value in d.items()]
        except Exception:
            pass
        return []

    def format_collection_value(self) -> str:
        try:
            d = self.prop.value
            if d is None:
                return "Dictionary (empty)"
            count = len(d)
            return f"Dictionary ({count} items)"
        except Exception:
            return "Dictionary"

    def create_item_editor(self, parent, item_id: str) -> tk.Widget:
        """Create editor for dict item (key or value)"""
        key = self._item_map.get(item_id)
        dict_value = self.prop.value

        # Check if we're editing key or value
        edit_column = getattr(parent, '_collection_edit_column', {}).get(item_id, 'value')

        if edit_column == 'key':
            # Editing key - use simple entry
            e = ttk.Entry(parent)
            e.insert(0, str(key))
            e.selection_range(0, tk.END)
            return e
        else:
            # Editing value - use smart editor
            try:
                value = dict_value[key] if key in dict_value else ""
                return create_value_editor(parent, value)
            except Exception:
                e = ttk.Entry(parent)
                e.insert(0, "")
                e.selection_range(0, tk.END)
                return e

    def apply_item_change(self, item_id: str, new_value_text: str):
        """Change the key or value in dictionary"""
        old_key = self._item_map.get(item_id)
        if old_key is None:
            return

        dict_value = self.prop.value

        # Check if we're editing key or value
        # Note: tree is not passed here, so we need a different approach
        # We'll assume value editing by default and handle key editing through override
        if old_key in dict_value:
            old_value = dict_value[old_key]

            if is_enumeration(old_value):
                new_value = apply_enumeration_value(old_value, new_value_text, self.context)
            else:
                new_value = convert_text_to_value(new_value_text, old_value)

            dict_value[old_key] = new_value
        else:
            dict_value[old_key] = new_value_text

        self.prop.value = dict_value

    def apply_key_change(self, item_id: str, new_key_text: str):
        """Change the key in dictionary"""
        old_key = self._item_map.get(item_id)
        if old_key is None:
            return

        dict_value = self.prop.value
        if new_key_text != str(old_key) and old_key in dict_value:
            # Move value to new key
            value = dict_value.pop(old_key)
            dict_value[new_key_text] = value
            self._item_map[item_id] = new_key_text
            self.prop.value = dict_value

    def format_item_value(self, item_id: str) -> str:
        """Format value for display"""
        key = self._item_map.get(item_id)
        if key is None:
            return ""
        try:
            dict_value = self.prop.value
            value = dict_value[key] if key in dict_value else ""
            return str(value)
        except Exception:
            return ""

    # ========== Key editing support ==========

    def can_edit_key(self) -> bool:
        """Dict keys can be edited"""
        return True

    def close_editor(self, tree, item_id, editor, commit: bool):
        """Close editor for dict item (handle both key and value editing)"""
        if not self.is_collection_item(item_id):
            super().close_editor(tree, item_id, editor, commit)
            return

        if commit:
            text = self.get_editor_text(editor)
            edit_column = getattr(tree, '_collection_edit_column', {}).get(item_id, 'value')

            if edit_column == 'key':
                self.apply_key_change(item_id, text)
                tree.item(item_id, text=text)  # Update key display
            else:
                self.apply_item_change(item_id, text)
                tree.set(item_id, "value", self.format_item_value(item_id))

        # Clear edit column info
        if hasattr(tree, '_collection_edit_column') and item_id in tree._collection_edit_column:
            del tree._collection_edit_column[item_id]

    # ========== Add/Delete support ==========

    def supports_add_item(self) -> bool:
        return self.editable()

    def supports_delete_item(self) -> bool:
        return self.editable()

    def add_item(self, tree, parent_id):
        """Add new item to dictionary"""
        dict_value = self.prop.value

        # Generate unique key
        new_key = "new_key"
        counter = 1
        while new_key in dict_value:
            new_key = f"new_key_{counter}"
            counter += 1

        # Add to dictionary
        dict_value[new_key] = ""
        self.prop.value = dict_value

        # Add to tree
        tag = 'readonly' if not self.editable() else 'editable'
        item_id = tree.insert(
            parent_id, 'end',
            text=str(new_key),
            values=("",),
            tags=(tag,)
        )
        tree._item_to_view[item_id] = self
        self._item_map[item_id] = new_key

        # Update count display
        tree.set(parent_id, "value", self.format_value())

        # Select new item
        tree.selection_set(item_id)
        tree.see(item_id)

    def delete_item(self, tree, item_id):
        """Delete item from dictionary"""
        key = self._item_map.get(item_id)
        if key is None:
            return

        # Remove from dictionary
        dict_value = self.prop.value
        if key in dict_value:
            del dict_value[key]
            self.prop.value = dict_value

        # Clean up
        del self._item_map[item_id]
        if item_id in tree._item_to_view:
            del tree._item_to_view[item_id]

        parent_id = tree.parent(item_id)
        tree.delete(item_id)

        # Update count display
        if parent_id:
            tree.set(parent_id, "value", self.format_value())
