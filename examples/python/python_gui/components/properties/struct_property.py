"""Refactored struct property view using CollectionPropertyView base"""
import tkinter as tk
from tkinter import ttk
import opendaq
from .collection_property import CollectionPropertyView
from .value_editor_helper import (
    create_value_editor, get_editor_value,
    convert_text_to_value, is_enumeration, apply_enumeration_value
)


class StructPropertyView(CollectionPropertyView):
    """Property view for struct properties"""

    def get_items(self):
        """Return list of (field_name, value) tuples"""
        try:
            struct = self.prop.value
            if struct:
                field_names = struct.field_names
                return [(name, struct.get(name)) for name in field_names]
        except Exception:
            pass
        return []

    def format_collection_value(self) -> str:
        try:
            struct = self.prop.value
            if struct is None:
                return "Struct (empty)"
            field_names = struct.field_names
            count = len(field_names)
            return f"Struct ({count} fields)"
        except Exception:
            return "Struct"

    def create_item_editor(self, parent, item_id: str) -> tk.Widget:
        """Create editor for struct field value"""
        field_name = self._item_map.get(item_id)
        struct = self.prop.value

        if field_name:
            try:
                value = struct.get(field_name)
                return create_value_editor(parent, value)
            except Exception:
                pass

        # Fallback
        e = ttk.Entry(parent)
        e.insert(0, "")
        e.selection_range(0, tk.END)
        return e

    def apply_item_change(self, item_id: str, new_value_text: str):
        """Change the field value in struct using StructBuilder"""
        field_name = self._item_map.get(item_id)
        if field_name is None:
            return

        try:
            struct = self.prop.value
            builder = opendaq.StructBuilderFromStruct(struct)

            current_value = struct.get(field_name)

            if is_enumeration(current_value):
                new_value = apply_enumeration_value(current_value, new_value_text, self.context)
            else:
                new_value = convert_text_to_value(new_value_text, current_value)

            builder.set(field_name, new_value)
            new_struct = builder.build()
            self.prop.value = new_struct
        except Exception as e:
            print(f"Error updating struct field: {e}")

    def format_item_value(self, item_id: str) -> str:
        """Format value for display"""
        field_name = self._item_map.get(item_id)
        if field_name is None:
            return ""
        try:
            struct = self.prop.value
            value = struct.get(field_name)
            return str(value)
        except Exception:
            return ""

    # Structs don't support add/delete - fields are fixed
    def supports_add_item(self) -> bool:
        return False

    def supports_delete_item(self) -> bool:
        return False
