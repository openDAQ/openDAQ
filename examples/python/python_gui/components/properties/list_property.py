import tkinter as tk
from tkinter import ttk
from .base import PropertyView


class ListPropertyView(PropertyView):
    def __init__(self, prop):
        super().__init__(prop)
        self._item_indices = {}  # item_id -> index

    def format_value(self) -> str:
        try:
            lst = self.prop.value
            if lst is None:
                return "List (empty)"
            count = len(lst)
            return f"List ({count} items)"
        except Exception:
            return "List"

    def build_tree(self, tree, parent_id):
        """Build tree items for list entries"""
        try:
            list_value = self.prop.value
            if list_value:
                tag = 'readonly' if not self.editable() else 'editable'
                for index, value in enumerate(list_value):
                    item_id = tree.insert(
                        parent_id, 'end',
                        text=f"[{index}]",
                        values=(str(value),),
                        tags=(tag,)
                    )
                    # Store reference to this view and index
                    tree._item_to_view[item_id] = self
                    self._item_indices[item_id] = index
        except Exception:
            pass

    def is_list_item(self, item_id: str) -> bool:
        """Check if item_id belongs to this list's items"""
        return item_id in self._item_indices

    def get_item_index(self, item_id: str):
        """Get the index for this item"""
        return self._item_indices.get(item_id)

    def create_item_editor(self, parent, item_id: str) -> tk.Widget:
        """Create editor for list item value"""
        e = ttk.Entry(parent)
        index = self._item_indices.get(item_id)
        list_value = self.prop.value

        try:
            value = list_value[index] if 0 <= index < len(list_value) else ""
            e.insert(0, str(value))
        except Exception:
            e.insert(0, "")
        e.selection_range(0, tk.END)
        return e

    def apply_item_value_change(self, item_id: str, new_value_text: str):
        """Change the value in list"""
        index = self._item_indices.get(item_id)
        if index is None:
            return

        list_value = self.prop.value
        if 0 <= index < len(list_value):
            list_value[index] = new_value_text
            # Apply changes to property
            self.prop.value = list_value

    def format_item_index(self, item_id: str) -> str:
        """Format index for display"""
        index = self._item_indices.get(item_id)
        return f"[{index}]" if index is not None else ""

    def format_item_value(self, item_id: str) -> str:
        """Format value for display"""
        index = self._item_indices.get(item_id)
        if index is None:
            return ""
        try:
            list_value = self.prop.value
            value = list_value[index] if 0 <= index < len(list_value) else ""
            return str(value)
        except Exception:
            return ""

    def handle_double_click(self, tree, item_id, column):
        """Handle double click on list item. Returns editor if handled."""
        if not self.is_list_item(item_id):
            return None

        # Only allow editing values, not indices
        if column == "#0":
            return None

        # Get bounding box for value column
        bbox = tree.bbox(item_id, column="value")
        if not bbox:
            return None

        x, y, w, h = bbox
        editor = self.create_item_editor(tree, item_id)
        editor.place(x=x, y=y, width=w, height=h)
        editor.focus_set()

        return editor

    def close_editor(self, tree, item_id, editor, commit: bool):
        """Close editor for list item"""
        if not self.is_list_item(item_id):
            # This is list property itself, use base implementation
            super().close_editor(tree, item_id, editor, commit)
            return

        # This is a list item
        if commit:
            text = self.get_editor_text(editor)
            self.apply_item_value_change(item_id, text)
            tree.set(item_id, "value", self.format_item_value(item_id))

    def handle_right_click(self, tree, item_id, event):
        """Handle right click on list or list item. Returns True if handled."""
        if not self.editable():
            return False

        # Is this a list item?
        if self.is_list_item(item_id):
            # Right click on list item - add and delete
            menu = tk.Menu(tree, tearoff=0)
            menu.add_command(label="Add back", command=lambda: self._add_item_back(tree, tree.parent(item_id)))
            menu.add_command(label="Remove", command=lambda: self._delete_item(tree, item_id))
            menu.post(event.x_root, event.y_root)
            return True

        # Check if this is the list itself
        view = tree._item_to_view.get(item_id)
        if view == self:
            # Right click on list itself - push back
            menu = tk.Menu(tree, tearoff=0)
            menu.add_command(label="Add back", command=lambda: self._add_item_back(tree, item_id))
            menu.post(event.x_root, event.y_root)
            return True

        return False

    def _add_item_front(self, tree, parent_id):
        """Add new item to the front of the list"""
        list_value = self.prop.value

        # Add to front of list - create new list with empty string at front
        new_list = [""] + [list_value[i] for i in range(len(list_value))]
        list_value.clear()
        for item in new_list:
            list_value.append(item)

        # Apply changes to property
        self.prop.value = list_value

        # Rebuild tree
        self._rebuild_tree(tree, parent_id)

    def _add_item_back(self, tree, parent_id):
        """Add new item to the back of the list"""
        list_value = self.prop.value

        # Add to back of list
        list_value.append("")
        # Apply changes to property
        self.prop.value = list_value

        # Rebuild tree
        self._rebuild_tree(tree, parent_id)

    def _add_item(self, tree, parent_id):
        """Add new item to list (defaults to back)"""
        self._add_item_back(tree, parent_id)

    def _delete_item(self, tree, item_id):
        """Delete item from list"""
        index = self._item_indices.get(item_id)
        if index is None:
            return

        # Remove from list
        list_value = self.prop.value
        if 0 <= index < len(list_value):
            del list_value[index]
            # Apply changes to property
            self.prop.value = list_value

        # Get parent before deletion
        parent_id = tree.parent(item_id)

        # Rebuild tree
        self._rebuild_tree(tree, parent_id)

    def _move_item_up(self, tree, item_id):
        """Move item up in the list"""
        index = self._item_indices.get(item_id)
        if index is None or index <= 0:
            return

        list_value = self.prop.value
        # Swap with previous item
        list_value[index], list_value[index - 1] = list_value[index - 1], list_value[index]
        # Apply changes to property
        self.prop.value = list_value

        # Get parent
        parent_id = tree.parent(item_id)

        # Rebuild tree
        self._rebuild_tree(tree, parent_id)

        # Select the moved item (now at index-1)
        for child_id in tree.get_children(parent_id):
            if self._item_indices.get(child_id) == index - 1:
                tree.selection_set(child_id)
                tree.see(child_id)
                break

    def _move_item_down(self, tree, item_id):
        """Move item down in the list"""
        index = self._item_indices.get(item_id)
        list_value = self.prop.value
        if index is None or index >= len(list_value) - 1:
            return

        # Swap with next item
        list_value[index], list_value[index + 1] = list_value[index + 1], list_value[index]
        # Apply changes to property
        self.prop.value = list_value

        # Get parent
        parent_id = tree.parent(item_id)

        # Rebuild tree
        self._rebuild_tree(tree, parent_id)

        # Select the moved item (now at index+1)
        for child_id in tree.get_children(parent_id):
            if self._item_indices.get(child_id) == index + 1:
                tree.selection_set(child_id)
                tree.see(child_id)
                break

    def _rebuild_tree(self, tree, parent_id):
        """Rebuild the entire list in the tree"""
        # Delete all items
        for child_id in tree.get_children(parent_id):
            if child_id in self._item_indices:
                del self._item_indices[child_id]
                if child_id in tree._item_to_view:
                    del tree._item_to_view[child_id]
                tree.delete(child_id)

        # Rebuild tree items
        self.build_tree(tree, parent_id)

        # Update item count display in parent
        if parent_id:
            tree.set(parent_id, "value", self.format_value())
