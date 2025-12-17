"""Helper functions for creating appropriate editors for different value types"""
import tkinter as tk
from tkinter import ttk
import opendaq


def is_enumeration(value) -> bool:
    """Check if value is an enumeration"""
    try:
        return hasattr(value, 'enumeration_type') and hasattr(value, 'name')
    except Exception:
        return False


def is_bool(value) -> bool:
    """Check if value is a boolean"""
    return isinstance(value, bool)

def create_value_editor(parent, value) -> tk.Widget:
    """Create appropriate editor widget based on value type"""

    # Enumeration - use combobox
    if is_enumeration(value):
        try:
            enum_type = value.enumeration_type
            enumerator_names = list(enum_type.enumerator_names)
            current_name = value.name

            combo = ttk.Combobox(parent, values=enumerator_names, state="readonly")
            if current_name in enumerator_names:
                combo.set(current_name)
            elif enumerator_names:
                combo.set(enumerator_names[0])
            return combo
        except Exception:
            pass

    # Boolean - use combobox with True/False
    if is_bool(value):
        combo = ttk.Combobox(parent, values=["True", "False"], state="readonly")
        combo.set("True" if value else "False")
        return combo

    # Default - use entry
    entry = ttk.Entry(parent)
    entry.insert(0, str(value))
    entry.selection_range(0, tk.END)
    return entry


def get_editor_value(editor: tk.Widget) -> str:
    """Get text value from editor widget"""
    if isinstance(editor, ttk.Combobox):
        return editor.get()
    elif isinstance(editor, ttk.Entry):
        return editor.get()
    return ""


def convert_text_to_value(text: str, reference_value):
    """Convert text to appropriate type based on reference value"""

    # Enumeration - return the text (will be used to create new enum)
    if is_enumeration(reference_value):
        return text

    # Boolean
    if is_bool(reference_value):
        return text.lower() in ('true', '1', 'yes')

    # Numbers
    if isinstance(reference_value, int):
        try:
            return int(text)
        except (ValueError, TypeError):
            return text

    if isinstance(reference_value, float):
        try:
            return float(text)
        except (ValueError, TypeError):
            return text

    # Default - return as string
    return text


def apply_enumeration_value(enum_value, new_name: str, context=None):
    """Create new enumeration with the given name"""
    try:
        enum_type = enum_value.enumeration_type
        enumerator_names = list(enum_type.enumerator_names)

        if new_name not in enumerator_names:
            print(f"Warning: {new_name} not in enumerator names: {enumerator_names}")
            return enum_value

        # Create new enumeration using Enumeration constructor with type_manager
        if context is not None:
            type_manager = context.daq_instance.context.type_manager
            enum_type_name = enum_type.name
            # Convert strings to IString objects
            new_enum = opendaq.Enumeration(
                opendaq.String(enum_type_name),
                opendaq.String(new_name),
                type_manager
            )
            print(f"Applied enumeration change: {enum_value.name} -> {new_enum.name}")
            return new_enum
        else:
            # Fallback: try using attribute accessor (may fail without type_manager)
            new_enum = getattr(enum_type, new_name)
            print(f"Applied enumeration change: {enum_value.name} -> {new_enum.name}")
            return new_enum
    except Exception as e:
        print(f"Error applying enumeration value: {e}")
        import traceback
        traceback.print_exc()
        return enum_value
