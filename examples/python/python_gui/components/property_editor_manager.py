"""Manager for property editor lifecycle"""
import tkinter as tk
from tkinter import ttk


class PropertyEditorManager:
    """Manages the lifecycle of property editors in a tree view"""

    def __init__(self, tree: ttk.Treeview, editor_state):
        self.tree = tree
        self.editor_state = editor_state

    def start_editor(self, item_id: str, view) -> bool:
        """Start editing a property. Returns True if editor was created."""
        self.close_editor(commit=True)

        bbox = self.tree.bbox(item_id, column="value")
        if not bbox:
            return False

        x, y, w, h = bbox
        editor = view.create_editor(self.tree)
        if not editor:
            return False

        editor.place(x=x, y=y, width=w, height=h)
        editor.focus_set()

        self.editor_state.set(editor, item_id, view)
        self._bind_editor_events(editor)
        return True

    def close_editor(self, commit: bool):
        """Close the active editor"""
        if not self.editor_state.is_active():
            return

        state = self.editor_state
        editor = state.editor
        item_id = state.item_id
        view = state.view

        self.editor_state.clear()

        try:
            if item_id and view:
                view.close_editor(self.tree, item_id, editor, commit)
        except Exception as e:
            print(f"Error closing editor: {e}")
        finally:
            try:
                editor.destroy()
            except Exception:
                pass

    def _bind_editor_events(self, editor: tk.Widget):
        """Bind standard editor events"""
        editor.bind("<Return>", lambda e: self.close_editor(commit=True))
        editor.bind("<FocusOut>", lambda e: self.close_editor(commit=True))
        editor.bind("<Escape>", lambda e: self.close_editor(commit=False))
