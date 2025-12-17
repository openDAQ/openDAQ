import tkinter as tk
from tkinter import ttk
from .base import PropertyView
import opendaq
from .value_editor_helper import (
    create_value_editor, get_editor_value,
    convert_text_to_value, is_enumeration, apply_enumeration_value
)


class StructPropertyView(PropertyView):
    def __init__(self, prop, context=None):
        super().__init__(prop, context)
        self._item_fields = {}  # item_id -> field_name

    def format_value(self) -> str:
        try:
            struct = self.prop.value
            if struct is None:
                return "Struct (empty)"
            field_names = struct.field_names
            count = len(field_names)
            return f"Struct ({count} fields)"
        except Exception:
            return "Struct"

    def build_tree(self, tree, parent_id, current_path: str = ""):
        """Build tree items for struct fields"""
        try:
            struct = self.prop.value
            if struct:
                field_names = struct.field_names
                tag = 'readonly' if not self.editable() else 'editable'
                for field_name in field_names:
                    try:
                        field_value = struct.get(field_name)
                        item_id = tree.insert(
                            parent_id, 'end',
                            text=str(field_name),
                            values=(str(field_value),),
                            tags=(tag,)
                        )
                        # Store reference to this view and field name
                        tree._item_to_view[item_id] = self
                        self._item_fields[item_id] = field_name
                    except Exception:
                        pass
        except Exception:
            pass

    def is_struct_field(self, item_id: str) -> bool:
        """Check if item_id belongs to this struct's fields"""
        return item_id in self._item_fields

    def get_field_name(self, item_id: str):
        """Get the field name for this item"""
        return self._item_fields.get(item_id)

    def create_field_editor(self, parent, item_id: str) -> tk.Widget:
        """Create editor for struct field value"""
        field_name = self._item_fields.get(item_id)
        struct = self.prop.value

        if field_name:
            try:
                value = struct.get(field_name)
                # Use smart editor based on value type
                return create_value_editor(parent, value)
            except Exception:
                pass

        # Fallback to entry
        e = ttk.Entry(parent)
        e.insert(0, "")
        e.selection_range(0, tk.END)
        return e

    def apply_field_change(self, item_id: str, new_value_text: str):
        """Change the field value in struct using StructBuilder"""
        field_name = self._item_fields.get(item_id)
        if field_name is None:
            return

        try:
            struct = self.prop.value
            # Create builder from existing struct
            builder = opendaq.StructBuilderFromStruct(struct)

            # Get the current value to determine type
            current_value = struct.get(field_name)

            # Handle enumeration specially
            if is_enumeration(current_value):
                new_value = apply_enumeration_value(current_value, new_value_text, self.context)
            else:
                # Convert to appropriate type
                new_value = convert_text_to_value(new_value_text, current_value)

            # Set new value
            builder.set(field_name, new_value)

            # Build and apply
            new_struct = builder.build()
            self.prop.value = new_struct
        except Exception as e:
            print(f"Error updating struct field: {e}")

    def format_field_value(self, item_id: str) -> str:
        """Format value for display"""
        field_name = self._item_fields.get(item_id)
        if field_name is None:
            return ""
        try:
            struct = self.prop.value
            value = struct.get(field_name)
            return str(value)
        except Exception:
            return ""

    def handle_double_click(self, tree, item_id, column):
        """Handle double click on struct field. Returns editor if handled."""
        if not self.is_struct_field(item_id):
            return None

        # Only edit value column for struct fields (keys are fixed)
        # column is "#0" for name column, "#1" for value column
        if column == "#0":
            return None

        bbox = tree.bbox(item_id, column="#1")
        if not bbox:
            return None

        x, y, w, h = bbox
        editor = self.create_field_editor(tree, item_id)
        editor.place(x=x, y=y, width=w, height=h)
        editor.focus_set()

        return editor

    def close_editor(self, tree, item_id, editor, commit: bool):
        """Close editor for struct field"""
        if not self.is_struct_field(item_id):
            # This is struct property itself, use base implementation
            super().close_editor(tree, item_id, editor, commit)
            return

        # This is a struct field
        if commit:
            text = get_editor_value(editor)
            self.apply_field_change(item_id, text)
            tree.set(item_id, "#1", self.format_field_value(item_id))
