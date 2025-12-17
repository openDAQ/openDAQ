import tkinter as tk
from tkinter import ttk
from .base import PropertyView
from .value_editor_helper import (
    create_value_editor, get_editor_value,
    convert_text_to_value, is_enumeration, apply_enumeration_value
)


class DictPropertyView(PropertyView):
    def __init__(self, prop, context=None):
        super().__init__(prop, context)
        self._item_keys = {}  # item_id -> original_key

    def format_value(self) -> str:
        try:
            d = self.prop.value
            if d is None:
                return "Dictionary (empty)"
            count = len(d)
            return f"Dictionary ({count} items)"
        except Exception:
            return "Dictionary"

    def build_tree(self, tree, parent_id, current_path: str = ""):
        """Build tree items for dictionary entries"""
        try:
            dict_value = self.prop.value
            if dict_value:
                tag = 'readonly' if not self.editable() else 'editable'
                for key, value in dict_value.items():
                    item_id = tree.insert(
                        parent_id, 'end',
                        text=str(key),
                        values=(str(value),),
                        tags=(tag,)
                    )
                    # Store reference to this view and key
                    tree._item_to_view[item_id] = self
                    self._item_keys[item_id] = key
        except Exception:
            pass

    def is_dict_item(self, item_id: str) -> bool:
        """Check if item_id belongs to this dict's items"""
        return item_id in self._item_keys

    def get_item_key(self, item_id: str):
        """Get the original key for this item"""
        return self._item_keys.get(item_id)

    def create_item_editor(self, parent, item_id: str, edit_key: bool) -> tk.Widget:
        """Create editor for dict item key or value"""
        key = self._item_keys.get(item_id)
        dict_value = self.prop.value

        if edit_key:
            # Keys are always strings - use entry
            e = ttk.Entry(parent)
            e.insert(0, str(key))
            e.selection_range(0, tk.END)
            return e
        else:
            # Values - use smart editor
            try:
                value = dict_value[key] if key in dict_value else ""
                return create_value_editor(parent, value)
            except Exception:
                e = ttk.Entry(parent)
                e.insert(0, "")
                e.selection_range(0, tk.END)
                return e

    def apply_item_key_change(self, item_id: str, new_key_text: str):
        """Change the key in dictionary"""
        old_key = self._item_keys.get(item_id)
        if old_key is None:
            return

        dict_value = self.prop.value
        if new_key_text != str(old_key):
            # Remove old key and add new one
            value = dict_value.pop(old_key)
            dict_value[new_key_text] = value
            self._item_keys[item_id] = new_key_text
            # Apply changes to property
            self.prop.value = dict_value

    def apply_item_value_change(self, item_id: str, new_value_text: str):
        """Change the value in dictionary"""
        key = self._item_keys.get(item_id)
        if key is None:
            return

        dict_value = self.prop.value
        if key in dict_value:
            old_value = dict_value[key]

            # Handle enumeration specially
            if is_enumeration(old_value):
                new_value = apply_enumeration_value(old_value, new_value_text, self.context)
            else:
                # Convert to appropriate type
                new_value = convert_text_to_value(new_value_text, old_value)

            dict_value[key] = new_value
        else:
            dict_value[key] = new_value_text

        # Apply changes to property
        self.prop.value = dict_value

    def format_item_key(self, item_id: str) -> str:
        """Format key for display"""
        key = self._item_keys.get(item_id)
        return str(key) if key is not None else ""

    def format_item_value(self, item_id: str) -> str:
        """Format value for display"""
        key = self._item_keys.get(item_id)
        if key is None:
            return ""
        try:
            dict_value = self.prop.value
            value = dict_value[key] if key in dict_value else ""
            return str(value)
        except Exception:
            return ""

    def handle_double_click(self, tree, item_id, column):
        """Handle double click on dict item. Returns editor if handled."""
        if not self.is_dict_item(item_id):
            return None

        # Determine which column was clicked
        edit_column = 'key' if column == "#0" else 'value'

        # Store information about what we are editing
        if not hasattr(tree, '_dict_edit_column'):
            tree._dict_edit_column = {}
        tree._dict_edit_column[item_id] = edit_column

        # Create editor
        if edit_column == 'key':
            bbox = tree.bbox(item_id, column="#0")
        else:
            bbox = tree.bbox(item_id, column="value")

        if not bbox:
            return None

        x, y, w, h = bbox
        editor = self.create_item_editor(tree, item_id, edit_key=(edit_column == 'key'))
        editor.place(x=x, y=y, width=w, height=h)
        editor.focus_set()

        return editor

    def close_editor(self, tree, item_id, editor, commit: bool):
        """Close editor for dict item"""
        if not self.is_dict_item(item_id):
            # This is dict property itself, use base implementation
            super().close_editor(tree, item_id, editor, commit)
            return

        # This is a dict item
        if commit:
            text = get_editor_value(editor)
            edit_column = getattr(tree, '_dict_edit_column', {}).get(item_id, 'value')

            if edit_column == 'key':
                self.apply_item_key_change(item_id, text)
                tree.item(item_id, text=self.format_item_key(item_id))
            else:
                self.apply_item_value_change(item_id, text)
                tree.set(item_id, "value", self.format_item_value(item_id))

        # Clear column information (always, even on cancel)
        if hasattr(tree, '_dict_edit_column') and item_id in tree._dict_edit_column:
            del tree._dict_edit_column[item_id]

    def handle_right_click(self, tree, item_id, event):
        """Handle right click on dict or dict item. Returns True if handled."""
        if not self.editable():
            return False

        # Is this a dict item?
        if self.is_dict_item(item_id):
            # Right click on dict item - add and delete
            menu = tk.Menu(tree, tearoff=0)
            menu.add_command(label="Add item", command=lambda: self._add_item(tree, tree.parent(item_id)))
            menu.add_command(label="Delete item", command=lambda: self._delete_item(tree, item_id))
            menu.post(event.x_root, event.y_root)
            return True

        # Check if this is the dict itself
        view = tree._item_to_view.get(item_id)
        if view == self:
            # Right click on dict itself - add only
            menu = tk.Menu(tree, tearoff=0)
            menu.add_command(label="Add item", command=lambda: self._add_item(tree, item_id))
            menu.post(event.x_root, event.y_root)
            return True

        return False

    def _add_item(self, tree, parent_id):
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
        # Apply changes to property
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
        self._item_keys[item_id] = new_key

        # Update item count display in parent
        tree.set(parent_id, "value", self.format_value())

        # Open editor for the new item's key
        tree.selection_set(item_id)
        tree.see(item_id)

    def _delete_item(self, tree, item_id):
        """Delete item from dictionary"""
        key = self._item_keys.get(item_id)
        if key is None:
            return

        # Remove from dictionary
        dict_value = self.prop.value
        if key in dict_value:
            del dict_value[key]
            # Apply changes to property
            self.prop.value = dict_value

        # Remove from internal structures
        del self._item_keys[item_id]
        if item_id in tree._item_to_view:
            del tree._item_to_view[item_id]

        # Get parent before deletion
        parent_id = tree.parent(item_id)

        # Remove from tree
        tree.delete(item_id)

        # Update item count display in parent
        if parent_id:
            tree.set(parent_id, "value", self.format_value())
