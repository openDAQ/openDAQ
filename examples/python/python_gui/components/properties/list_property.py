"""Refactored list property view using CollectionPropertyView base"""
import tkinter as tk
from tkinter import ttk
from .collection_property import CollectionPropertyView
from .value_editor_helper import (
    create_value_editor, get_editor_value,
    convert_text_to_value, is_enumeration, apply_enumeration_value
)


class ListPropertyView(CollectionPropertyView):
    """Property view for list properties"""

    def get_items(self):
        """Return list of (index_label, value) tuples"""
        try:
            lst = self.prop.value
            if lst:
                return [(f"[{i}]", value) for i, value in enumerate(lst)]
        except Exception:
            pass
        return []

    def format_collection_value(self) -> str:
        try:
            lst = self.prop.value
            if lst is None:
                return "List (empty)"
            count = len(lst)
            return f"List ({count} items)"
        except Exception:
            return "List"

    def create_item_editor(self, parent, item_id: str) -> tk.Widget:
        """Create editor for list item value"""
        index = self._get_index(item_id)
        list_value = self.prop.value

        try:
            value = list_value[index] if 0 <= index < len(list_value) else ""
            return create_value_editor(parent, value)
        except Exception:
            e = ttk.Entry(parent)
            e.insert(0, "")
            e.selection_range(0, tk.END)
            return e

    def _get_index(self, item_id: str) -> int:
        """Extract index from item label like '[0]'"""
        label = self._item_map.get(item_id, "[0]")
        try:
            return int(label.strip('[]'))
        except (ValueError, AttributeError):
            return 0

    def apply_item_change(self, item_id: str, new_value_text: str):
        """Change the value in list"""
        index = self._get_index(item_id)
        list_value = self.prop.value

        if 0 <= index < len(list_value):
            old_value = list_value[index]

            if is_enumeration(old_value):
                new_value = apply_enumeration_value(old_value, new_value_text, self.context)
            else:
                new_value = convert_text_to_value(new_value_text, old_value)

            list_value[index] = new_value
            self.prop.value = list_value

    def format_item_value(self, item_id: str) -> str:
        """Format value for display"""
        index = self._get_index(item_id)
        try:
            list_value = self.prop.value
            value = list_value[index] if 0 <= index < len(list_value) else ""
            return str(value)
        except Exception:
            return ""

    # ========== Add/Delete support ==========

    def supports_add_item(self) -> bool:
        return self.editable()

    def supports_delete_item(self) -> bool:
        return self.editable()

    def add_item(self, tree, parent_id):
        """Add new item to the back of the list"""
        list_value = self.prop.value
        list_value.append("")
        self.prop.value = list_value

        # Rebuild tree
        self._rebuild_tree(tree, parent_id)

    def delete_item(self, tree, item_id):
        """Delete item from list"""
        index = self._get_index(item_id)
        list_value = self.prop.value

        if 0 <= index < len(list_value):
            del list_value[index]
            self.prop.value = list_value

        parent_id = tree.parent(item_id)
        self._rebuild_tree(tree, parent_id)

    def _rebuild_tree(self, tree, parent_id):
        """Rebuild the entire list in the tree"""
        # Delete all items
        for child_id in tree.get_children(parent_id):
            if self.is_collection_item(child_id):
                del self._item_map[child_id]
                if child_id in tree._item_to_view:
                    del tree._item_to_view[child_id]
                tree.delete(child_id)

        # Rebuild
        self.build_tree(tree, parent_id)

        # Update count display
        if parent_id:
            tree.set(parent_id, "value", self.format_value())
