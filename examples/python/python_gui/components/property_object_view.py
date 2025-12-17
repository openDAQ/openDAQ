import tkinter as tk
from tkinter import ttk
import opendaq
from opendaq import IPropertyObject, CoreType
from app_context import AppContext
from .properties import make_property_view, PropertyView


class PropertyObjectView(ttk.Treeview):
    def __init__(
        self,
        parent: tk.Frame,
        context: AppContext,
        property_object: IPropertyObject,
        expand_tree: bool = True,
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
        self.property_path = property_path  # Path to this property object

        self.heading("#0", text="Property", anchor="w")
        self.heading("value", text="Value", anchor="w")

        self.column("#0", width=280, minwidth=140, stretch=True, anchor="w")
        self.column("value", width=420, minwidth=180, stretch=True, anchor="w")

        # readonly цвет — из MainWindow theme (если есть), иначе мягкий серый
        muted = getattr(getattr(context, "colors", None), "get", lambda *_: None)("muted")
        self.tag_configure("readonly", foreground=muted or "#6B7280")

        self._item_to_view: dict[str, PropertyView] = {}
        self._property_to_item: dict[str, str] = {}
        self._event_handlers: dict[str, object] = {}

        self.bind("<Double-1>", self.on_double_click)
        self.bind("<Button-3>", self.on_right_click)
        self.bind("<Button-2>", self.on_right_click)  # macOS
        self.bind("<F5>", self.on_refresh)

        self.build_tree("", self.property_object)

        if expand_tree:
            self.expand_all("")

        # Subscribe to property value changes
        self._setup_property_change_listeners()

        # --- Hover state ---
        self._hover_iid: str | None = None
        self._hover_tag = "hover"
        self._tree_hover_bind(self)

    def destroy(self):
        """Clean up event handlers before destroying the widget"""
        try:
            if hasattr(self, '_core_event_handler'):
                daq_context = self.context.daq_instance.context
                daq_context.on_core_event - self._core_event_handler
        except Exception as e:
            print(f"Error unsubscribing from core events: {e}")

        super().destroy()

    # -------------------- Property Change Listeners --------------------

    def _setup_property_change_listeners(self):
        """Subscribe to property value changes via context core events"""
        # Only subscribe if this is a root property object (not nested)
        # Nested property objects will receive updates through the root
        if self.property_path:
            return  # This is a nested property object, don't subscribe

        try:
            # Subscribe to core events from the DAQ instance context
            daq_context = self.context.daq_instance.context
            self._core_event_handler = opendaq.QueuedEventHandler(self._on_core_event)
            daq_context.on_core_event + self._core_event_handler
        except Exception as e:
            print(f"Failed to subscribe to core events: {e}")

    def _on_core_event(self, sender, args):
        """Handle PropertyValueChanged core events"""
        try:
            if args.event_name == "PropertyValueChanged":
                core_args = opendaq.ICoreEventArgs.cast_from(args)

                owner = core_args.parameters["Owner"]
                prop_name = core_args.parameters["Name"]
                new_value = core_args.parameters["Value"]
                path = core_args.parameters["Path"]

                if path:
                    full_prop_path = f"{path}.{prop_name}"
                else:
                    full_prop_path = prop_name

                self._update_property_value(full_prop_path, new_value)
        except Exception as e:
            print(f"Error handling core event: {e}")

    def _update_property_value(self, prop_name: str, new_value):
        """Update the displayed value for a property"""
        item_id = self._property_to_item.get(prop_name)
        if not item_id or not self.exists(item_id):
            print(f"Property '{prop_name}' not found in tree. Available paths: {list(self._property_to_item.keys())}")
            return

        view = self._item_to_view.get(item_id)
        if not view:
            return

        try:
            # Update the view's internal value
            view.prop.value = new_value

            # Refresh the displayed value in the tree
            formatted_value = view.format_value()
            self.item(item_id, values=(formatted_value,))
        except Exception as e:
            print(f"Error updating property {prop_name}: {e}")

    def on_refresh(self, event=None):
        """Refresh all property values (F5 handler)"""
        self.refresh_all_values()

    def refresh_all_values(self):
        """Refresh all displayed property values from the property object"""
        for item_id, view in self._item_to_view.items():
            if not self.exists(item_id):
                continue

            try:
                # Update the displayed value in the tree
                formatted_value = view.format_value()
                self.item(item_id, values=(formatted_value,))
            except Exception as e:
                print(f"Error refreshing property value for item {item_id}: {e}")

    # -------------------- Theme helpers --------------------

    def _get_color(self, key: str, default: str | None = None) -> str | None:
        colors = getattr(self.context, "colors", None)
        if hasattr(colors, "get"):
            return colors.get(key, default)
        return default

    def _theme_disabled_foreground(self) -> str | None:
        """Best-effort: достать 'disabled' цвет из текущей темы."""
        try:
            s = ttk.Style(self)
            for sty in ("TLabel", "TEntry", "Treeview"):
                v = s.lookup(sty, "foreground", ("disabled",))
                if v:
                    return v
            v = s.lookup("Treeview", "foreground")
            return v or None
        except Exception:
            return None

    # -------------------- Tree building --------------------

    def build_tree(self, parent_id, property_obj: IPropertyObject, current_path: str = ""):
        props = (
            property_obj.all_properties
            if self.context.show_invisible_components
            else property_obj.visible_properties
        )

        for prop in props:
            view = make_property_view(prop, self.context)
            tag = "readonly" if prop.read_only else "editable"

            # Build full path for this property
            full_path = f"{current_path}.{prop.name}" if current_path else prop.name

            item_id = self.insert(
                parent_id,
                "end",
                text=prop.name,
                values=(view.format_value(),),
                tags=(tag,),
                open=(prop.value_type in (CoreType.ctObject, CoreType.ctDict, CoreType.ctList, CoreType.ctStruct)),
            )
            self._item_to_view[item_id] = view
            self._property_to_item[full_path] = item_id
            view.build_tree(self, item_id, full_path)

    def expand_all(self, item_id: str = ""):
        for child in self.get_children(item_id):
            self.item(child, open=True)
            self.expand_all(child)

    # -------------------- Editor lifecycle --------------------

    def _close_editor(self, commit: bool):
        if not self.context.editor_state.is_active():
            return

        state = self.context.editor_state
        editor = state.editor
        item_id = state.item_id
        view = state.view

        self.context.editor_state.clear()

        try:
            if item_id and view:
                view.close_editor(self, item_id, editor, commit)
        except Exception:
            pass
        finally:
            try:
                editor.destroy()
            except Exception:
                pass

    def _start_editor(self, item_id: str, view: PropertyView):
        self._close_editor(commit=True)

        bbox = self.bbox(item_id, column="value")
        if not bbox:
            return
        x, y, w, h = bbox

        editor = view.create_editor(self)
        editor.place(x=x, y=y, width=w, height=h)
        editor.focus_set()

        self.context.editor_state.set(editor, item_id, view)

        editor.bind("<Return>", lambda e: self._close_editor(commit=True))
        editor.bind("<FocusOut>", lambda e: self._close_editor(commit=True))
        editor.bind("<Escape>", lambda e: self._close_editor(commit=False))

    # -------------------- Events --------------------

    def on_double_click(self, event):
        item_id = self.identify_row(event.y)
        col = self.identify_column(event.x)
        if not item_id:
            return

        view = self._item_to_view.get(item_id)
        if not view or not view.editable():
            return

        self._close_editor(commit=True)

        result = view.handle_double_click(self, item_id, col)

        # If handle_double_click returns True, it handled the event (e.g., bool toggle)
        if result is True:
            return

        # If it returns a widget, set up the editor
        if result and result is not True:
            editor = result
            self.context.editor_state.set(editor, item_id, view)
            editor.bind("<Return>", lambda e: self._close_editor(commit=True))
            editor.bind("<FocusOut>", lambda e: self._close_editor(commit=True))
            editor.bind("<Escape>", lambda e: self._close_editor(commit=False))
            return

        # Otherwise, if column is value, start default editor
        if col == "#1":
            self._start_editor(item_id, view)

    def on_right_click(self, event):
        item_id = self.identify_row(event.y)
        if not item_id:
            return

        view = self._item_to_view.get(item_id)
        if not view:
            return

        view.handle_right_click(self, item_id, event)

    # -------------------- Hover --------------------

    def _tree_hover_bind(self, tree: ttk.Treeview):
        hover_bg = self._get_color("hover", "#E5E7EB")  # fallback
        tree.tag_configure(self._hover_tag, background=hover_bg)

        tree.bind("<Motion>", self._on_tree_motion, add=True)
        tree.bind("<Leave>", self._on_tree_leave, add=True)
        tree.bind("<<TreeviewSelect>>", self._on_tree_select, add=True)

    def _add_hover(self, tree: ttk.Treeview, iid: str):
        tags = tree.item(iid, "tags") or ()
        if self._hover_tag not in tags:
            tree.item(iid, tags=(*tags, self._hover_tag))

    def _remove_hover(self, tree: ttk.Treeview, iid: str):
        tags = tree.item(iid, "tags") or ()
        if self._hover_tag in tags:
            tree.item(iid, tags=tuple(t for t in tags if t != self._hover_tag))

    def _on_tree_motion(self, event):
        tree: ttk.Treeview = event.widget
        iid = tree.identify_row(event.y)

        if iid == self._hover_iid:
            return

        # снять hover со старого
        if self._hover_iid and tree.exists(self._hover_iid):
            self._remove_hover(tree, self._hover_iid)

        self._hover_iid = iid

        if not iid or not tree.exists(iid):
            self._hover_iid = None
            return

        # не подсвечиваем выбранную строку (чтобы не перебивать selection background)
        if iid not in tree.selection():
            self._add_hover(tree, iid)

    def _on_tree_select(self, event):
        tree: ttk.Treeview = event.widget
        # если выбрали hover-строку — убрать hover
        if self._hover_iid and tree.exists(self._hover_iid) and self._hover_iid in tree.selection():
            self._remove_hover(tree, self._hover_iid)

    def _on_tree_leave(self, event):
        tree: ttk.Treeview = event.widget
        if self._hover_iid and tree.exists(self._hover_iid):
            self._remove_hover(tree, self._hover_iid)
        self._hover_iid = None
