"""Refactored property object view - simplified and focused"""
import tkinter as tk
from tkinter import ttk
from opendaq import IPropertyObject, CoreType
from app_context import AppContext
from .properties import make_property_view, PropertyView
from .property_editor_manager import PropertyEditorManager
from .property_event_handler import PropertyEventHandler
from .tree_hover_mixin import TreeHoverMixin


class PropertyObjectView(TreeHoverMixin, ttk.Treeview):
    """
    Simplified property object view that displays IPropertyObject properties in a tree.

    Responsibilities:
    - Build and display property tree
    - Handle double-click and right-click events
    - Delegate editor management to PropertyEditorManager
    - Delegate event handling to PropertyEventHandler
    - Mix in hover effects from TreeHoverMixin
    """

    def __init__(
        self,
        parent: tk.Frame,
        context: AppContext,
        property_object: IPropertyObject,
        expand_tree: bool = False,
        property_path: str = "",
    ):
        super().__init__(
            parent,
            columns=("value",),
            show="tree headings",
            style="Props.Treeview",
            selectmode="browse",
        )
        self.context = context
        self.property_object = property_object
        self.property_path = property_path

        # Setup columns
        self.heading("#0", text="Property", anchor="w")
        self.heading("value", text="Value", anchor="w")
        self.column("#0", width=280, minwidth=140, stretch=True, anchor="w")
        self.column("value", width=420, minwidth=180, stretch=True, anchor="w")

        # Configure readonly tag
        muted_color = self._get_theme_color("muted", "#6B7280")
        self.tag_configure("readonly", foreground=muted_color)

        # Internal mappings
        self._item_to_view: dict[str, PropertyView] = {}
        self._property_to_item: dict[str, str] = {}

        # Setup components
        self.editor_manager = PropertyEditorManager(self, context.editor_state)
        self.event_handler = PropertyEventHandler(context, property_object, property_path)

        # Register for property updates
        self.event_handler.register_update_callback("", self._on_property_updated)

        # Setup hover effects
        hover_color = self._get_theme_color("hover", "#E5E7EB")
        self.setup_hover(hover_color)

        # Bind events
        self.bind("<Double-1>", self._on_double_click)
        self.bind("<Button-3>", self._on_right_click)
        self.bind("<Button-2>", self._on_right_click)  # macOS
        self.bind("<F5>", self._on_refresh)

        # Build tree
        self.build_tree("", self.property_object)

        if expand_tree:
            self.expand_all("")

    def destroy(self):
        """Clean up before destroying widget"""
        self.event_handler.cleanup()
        super().destroy()

    # ========== Tree Building ==========

    def build_tree(self, parent_id: str, property_obj: IPropertyObject, current_path: str = ""):
        """Build tree recursively for property object"""
        # Track this property object in event handler
        self.event_handler.add_nested_property_object(current_path, property_obj)

        # Get properties to display
        props = (
            property_obj.all_properties
            if self.context.show_invisible_components
            else property_obj.visible_properties
        )

        for prop in props:
            view = make_property_view(prop, self.context)
            tag = "readonly" if prop.read_only else "editable"

            # Build full path
            full_path = f"{current_path}.{prop.name}" if current_path else prop.name

            # Insert item
            item_id = self.insert(
                parent_id,
                "end",
                text=prop.name,
                values=(view.format_value(),),
                tags=(tag,),
                open=(prop.value_type in (CoreType.ctObject, CoreType.ctDict,
                                         CoreType.ctList, CoreType.ctStruct)),
            )

            # Store mappings
            self._item_to_view[item_id] = view
            self._property_to_item[full_path] = item_id

            # Let view build its children (for nested objects, dicts, lists, etc.)
            view.build_tree(self, item_id, full_path)

    def expand_all(self, item_id: str = ""):
        """Recursively expand all tree items"""
        for child in self.get_children(item_id):
            self.item(child, open=True)
            self.expand_all(child)

    # ========== Event Handling ==========

    def _on_double_click(self, event):
        """Handle double-click on property"""
        item_id = self.identify_row(event.y)
        col = self.identify_column(event.x)
        if not item_id:
            return

        view = self._item_to_view.get(item_id)
        if not view or not view.editable():
            return

        # Close any open editor
        self.editor_manager.close_editor(commit=True)

        # Let view handle double-click (might toggle bool, cycle enum, etc.)
        result = view.handle_double_click(self, item_id, col)

        # If view handled it (returned True or a widget), we're done
        if result is True:
            return

        if result and result is not True:
            # View returned a widget - set it up as editor
            editor = result
            self.context.editor_state.set(editor, item_id, view)
            editor.bind("<Return>", lambda e: self.editor_manager.close_editor(commit=True))
            editor.bind("<FocusOut>", lambda e: self.editor_manager.close_editor(commit=True))
            editor.bind("<Escape>", lambda e: self.editor_manager.close_editor(commit=False))
            return

        # Otherwise, start default editor for value column
        if col == "#1":
            self.editor_manager.start_editor(item_id, view)

    def _on_right_click(self, event):
        """Handle right-click on property"""
        item_id = self.identify_row(event.y)
        if not item_id:
            return

        view = self._item_to_view.get(item_id)
        if view:
            view.handle_right_click(self, item_id, event)

    def _on_refresh(self, event=None):
        """Refresh all property values (F5)"""
        self.refresh_all_values()

    def _on_property_updated(self, prop_path: str, new_value):
        """Callback when property value changes"""
        item_id = self._property_to_item.get(prop_path)
        if not item_id or not self.exists(item_id):
            return

        view = self._item_to_view.get(item_id)
        if not view:
            return

        try:
            formatted_value = view.format_value()
            self.item(item_id, values=(formatted_value,))
        except Exception as e:
            print(f"Error updating property {prop_path}: {e}")

    def refresh_all_values(self):
        """Manually refresh all displayed property values"""
        for item_id, view in self._item_to_view.items():
            if not self.exists(item_id):
                continue

            try:
                formatted_value = view.format_value()
                self.item(item_id, values=(formatted_value,))
            except Exception as e:
                print(f"Error refreshing item {item_id}: {e}")

    # ========== Utility Methods ==========

    def _get_theme_color(self, key: str, default: str) -> str:
        """Get color from context theme, or use default"""
        colors = getattr(self.context, "colors", None)
        if hasattr(colors, "get"):
            return colors.get(key, default)
        return default
