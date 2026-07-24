import tkinter as tk
from tkinter import ttk
from tkinter import simpledialog
from fractions import Fraction

import opendaq as daq

from .. import utils
from ..app_context import AppContext
from .function_dialog import FunctionDialog
from .metadata_dialog import MetadataDialog
from .metadata_fields_selector_dialog import MetadataFieldsSelectorDialog


class PropertiesTreeview(ttk.Treeview):

    # deterministic property kinds (daq.PropertyType) used for dispatch
    SELECTION_KINDS = (daq.PropertyType.IndexSelection,
                       daq.PropertyType.Selection,
                       daq.PropertyType.SparseSelection)
    METHOD_KINDS = (daq.PropertyType.Function, daq.PropertyType.Procedure)
    CONTAINER_KINDS = (daq.PropertyType.Object, daq.PropertyType.Struct,
                       daq.PropertyType.List, daq.PropertyType.Dict)
    ITEM_CONTAINER_KINDS = (daq.PropertyType.List, daq.PropertyType.Dict)
    SCALAR_KINDS = (daq.PropertyType.Int, daq.PropertyType.Float,
                    daq.PropertyType.String)

    # the deterministic kind of a property; reference properties report the
    # kind of the property they currently point to
    @staticmethod
    def _property_kind(prop):
        try:
            kind = prop.property_type
            if kind == daq.PropertyType.Reference:
                return prop.referenced_property.property_type
        except (RuntimeError, AttributeError):
            return daq.PropertyType.Undefined
        return kind

    def __init__(self, parent, node=None, context: AppContext = None, read_only=False, **kwargs):
        self.hidden = kwargs.pop("hidden", [])
        self._metadata_fields = list(context.metadata_fields)
        ttk.Treeview.__init__(self, parent, columns=('value', *self._metadata_fields), show='tree headings', **kwargs)

        self.context = context
        self.node = node
        self._overlay_comboboxes = {}
        self._overlay_items = {}
        self._row_props = {}
        self._row_parent_props = {}
        self._last_method_results = {}
        self._active_dropdown_cb = None
        self._last_configure_size = (0, 0)
        self._syncing_overlays = False
        self.read_only = read_only

        style = ttk.Style(self)
        style.configure('Selection.TCombobox',
                        fieldbackground='white',
                        background='white',
                        foreground='#1a1a1a',
                        selectbackground='white',
                        selectforeground='#1a1a1a')
        style.configure('Editable.TCombobox',
                        fieldbackground='white',
                        background='white',
                        foreground='#1a1a1a')
        style.configure('Overlay.TCheckbutton', background='white')

        self._scroll_bar = ttk.Scrollbar(
            self, orient=tk.VERTICAL, command=self.yview)
        self.configure(yscrollcommand=lambda *a: (
            self._scroll_bar.set(*a), self._sync_overlays()))
        self._scroll_bar.pack(side=tk.RIGHT, fill=tk.Y)
        self._scroll_bar_x = ttk.Scrollbar(
            self, orient=tk.HORIZONTAL, command=self.xview)
        self.configure(xscrollcommand=lambda *a: (
            self._scroll_bar_x.set(*a), self.after_idle(self._sync_overlays)))
        self._scroll_bar_x.pack(side=tk.BOTTOM, fill=tk.X)
        self.pack(fill=tk.BOTH, expand=True)

        self.tag_configure('readonly', foreground='gray')

        # define headings
        self.heading('#0', anchor=tk.W, text='Property name')
        self.heading('value', anchor=tk.W, text='Value')
        # layout
        # Keep first two columns responsive to available width.
        self.column('#0', anchor=tk.W, minwidth=50, width=int(200 * self.context.dpi_factor), stretch=True)
        self.column('value', anchor=tk.W, minwidth=50, width=int(200 * self.context.dpi_factor), stretch=True)

        for field in self._metadata_fields:
            self.heading(field, anchor=tk.W,
                         text=utils.snake_case_to_title(field))
            self.column(field, anchor=tk.W, minwidth=100, width=int(100 * self.context.dpi_factor), stretch=False)

        # bind double-click to editing (if not read_only)
        if not self.read_only:
            self.bind('<Double-1>', lambda event=None: self.edit_value())
        self.bind('<Button-3>', lambda event: self.show_menu(event))
        self.bind('<MouseWheel>', lambda e=None: self.after_idle(self._sync_overlays))
        self.bind('<ButtonRelease-1>', lambda e=None: self.after(10, self._sync_overlays), add='+')
        self.bind('<Configure>', self._on_configure)
        self.bind('<Map>', lambda e=None: self.after_idle(self._sync_overlays) if e.widget is self else None)
        self._toplevel_bind_id = self.winfo_toplevel().bind(
            '<<DialogReady>>', lambda e=None: self._sync_overlays(), add='+')
        self.bind('<Destroy>', self._on_destroy)

        self.refresh()

    def refresh(self):
        collapsed = self._collect_collapsed_paths('')
        scroll_pos = self.yview()

        self._clear_overlay_comboboxes()
        self._overlay_items = {}
        self._row_props = {}
        self._row_parent_props = {}
        self.delete(*self.get_children())
        if self.node is not None:
            if daq.IPropertyObject.can_cast_from(self.node):
                self.fill_properties('', daq.IPropertyObject.cast_from(self.node), self.hidden)
            self._restore_collapsed_paths('', collapsed)
            self._collect_overlay_items('')
        self.after_idle(self._sync_overlays)

        self.after_idle(lambda: self.yview_moveto(scroll_pos[0]))

    def _collect_collapsed_paths(self, parent_iid):
        collapsed = set()
        for iid in self.get_children(parent_iid):
            path = tuple(utils.get_item_path(self, iid))
            if not self.item(iid, 'open'):
                collapsed.add(path)
            collapsed |= self._collect_collapsed_paths(iid)
        return collapsed

    def _restore_collapsed_paths(self, parent_iid, collapsed):
        for iid in self.get_children(parent_iid):
            path = tuple(utils.get_item_path(self, iid))
            if path in collapsed:
                self.item(iid, open=False)
            self._restore_collapsed_paths(iid, collapsed)

    def fill_list(self, parent_iid, l, read_only, prop=None):
        for i, value in enumerate(l):
            iid = self.insert('' if not parent_iid else parent_iid,
                             tk.END, text=str(i), values=(self._format_value(value),))
            self._row_parent_props[iid] = prop
            if read_only:
                self.item(iid, tags=('readonly',))

    def fill_dict(self, parent_iid, d, read_only, prop=None):
        for key, value in d.items():
            iid = self.insert('' if not parent_iid else parent_iid,
                             tk.END, text=str(key), values=(self._format_value(value),))
            self._row_parent_props[iid] = prop
            if read_only:
                self.item(iid, tags=('readonly',))

    def fill_struct(self, parent_iid, node, read_only, prop=None):
        # Special-case: ComplexNumber
        if isinstance(node, complex):
            # Display as real/imag
            iid = self.insert(parent_iid, tk.END, text="Real", values=(self._format_value(node.real),))
            iid2 = self.insert(parent_iid, tk.END, text="Imag", values=(self._format_value(node.imag),))
            return

        # Avoid crash; display raw value
        if not hasattr(node, "as_dictionary"):
            self.insert(parent_iid, tk.END, text="(value)", values=(str(node),))
            return

        for key, value in node.as_dictionary.items():
            iid = self.insert('' if not parent_iid else parent_iid,
                             tk.END, text=key, values=(self._format_value(value),))
            self._row_parent_props[iid] = prop
            if read_only:
                self.item(iid, tags=('readonly',))

    def fill_properties(self, parent_iid, node, hidden):
        def printed_value(value_type, value):
            if value is None:
                return 'N/A'
            if value_type == daq.CoreType.ctBool:
                return utils.yes_no[value]
            elif value_type == daq.CoreType.ctFloat:
                return self._format_value(value) 
            else:
                return value
        
        for property_info in self.context.properties_of_component(node):
            if property_info.name in hidden:
                # This property was marked as hidden
                continue

            kind = self._property_kind(property_info)

            if kind in self.SELECTION_KINDS:
                if property_info.selection_values is not None \
                        and len(property_info.selection_values) > 0:
                    property_value = printed_value(
                        property_info.item_type, node.get_property_selection_value(property_info.name))
                else:
                    property_value = 'Selection list is empty'
            elif kind in self.METHOD_KINDS:
                property_value = self._last_method_results.get(property_info.name, '')
            elif kind == daq.PropertyType.Struct:
                property_value = ''
            elif kind == daq.PropertyType.Object:
                property_value = ''
            elif kind == daq.PropertyType.Enumeration:
                property_value = self._enum_value_name(
                    node.get_property_value(property_info.name))
            elif kind in self.ITEM_CONTAINER_KINDS:
                property_value = str(node.get_property_value(property_info.name))
            else:
                property_value = printed_value(
                    property_info.value_type, node.get_property_value(property_info.name))
            
            unit_symbol = utils.prettify_unit(property_info.unit)
            if unit_symbol and property_value != '':
                property_value = f'{property_value} {unit_symbol}'

            meta_fields = [None] * len(self._metadata_fields)
            try:
                for i, field in enumerate(self._metadata_fields):
                    metadata_value = getattr(property_info, field)
                    metadata_value = utils.metadata_converters[field](
                        metadata_value) if field in utils.metadata_converters else metadata_value
                    meta_fields[i] = metadata_value
            except Exception as e:
                print(e)

            # Insert a treeview entry widget for the property
            if kind in self.METHOD_KINDS:
                display_name = '          ' + property_info.name
            else:
                display_name = property_info.name

            iid = self.insert(
                '' if not parent_iid else parent_iid,
                tk.END,
                open=True,
                text=display_name,
                values=(property_value, *meta_fields))
            self._row_props[iid] = property_info

            is_single_value_selection = (
                kind in self.SELECTION_KINDS
                and property_info.selection_values is not None
                and len(property_info.selection_values) == 1
            )
            if kind not in self.METHOD_KINDS and kind not in self.CONTAINER_KINDS:
                if property_info.read_only or self.read_only or is_single_value_selection:
                    self.item(iid, tags=('readonly',))

            if kind == daq.PropertyType.Object:
                hidden_children = [s.removeprefix(f"{property_info.name}.") for s in hidden if s.startswith(f"{property_info.name}.")]
                self.fill_properties(
                    iid, node.get_property_value(property_info.name), hidden_children)
            elif kind == daq.PropertyType.Struct:
                self.fill_struct(
                    iid, node.get_property_value(property_info.name), property_info.read_only,
                    prop=property_info)
            elif kind == daq.PropertyType.List:
                self.fill_list(iid, node.get_property_value(property_info.name), property_info.read_only,
                               prop=property_info)
            elif kind == daq.PropertyType.Dict:
                self.fill_dict(iid, node.get_property_value(property_info.name), property_info.read_only,
                               prop=property_info)

    def handle_copy(self):
        selected_item = utils.treeview_get_first_selection(self)
        if selected_item is None:
            return
        item = self.item(selected_item)

        self.clipboard_clear()
        value = '' if len(item['values']) == 0 else item['values'][0]
        self.clipboard_append(str(value).strip())

    def handle_show_metadata(self):
        selected_item = utils.treeview_get_first_selection(self)
        if selected_item is None:
            return

        path = utils.get_item_path(self, selected_item)
        prop = utils.get_property_for_path(self.context, path, self.node)
        if not prop:
            return

        MetadataDialog(self, prop, self.context).show()

    def nearest_property_with_path(self, path):
        prop = utils.get_property_for_path(self.context, path, self.node)
        while prop is None and len(path) > 0:
            path = path[:-1]
            prop = utils.get_property_for_path(self.context, path, self.node)
        return prop, path

    def handle_paste(self):
        selected_item_id = utils.treeview_get_first_selection(self)
        if selected_item_id is None:
            return

        path = utils.get_item_path(self, selected_item_id)
        prop, prop_path = self.nearest_property_with_path(path)

        if not prop:
            return

        path_diff = list(set(path) - set(prop_path))
        if len(path_diff) > 1:
            return

        value = prop.value
        try:
            kind = self._property_kind(prop)
            if kind == daq.PropertyType.Object:
                pass  # ignoring paste to objects
            elif kind == daq.PropertyType.Struct:
                field_type = [
                    field for field in prop.struct_type.field_types if field.name == path_diff[0]][0].core_type
                setattr(value, path_diff[0], utils.value_to_coretype(
                    self.clipboard_get(), field_type))
                prop.value = value
            elif kind == daq.PropertyType.List:
                value[int(path_diff[0])] = utils.value_to_coretype(
                    self.clipboard_get(), prop.item_type)
                prop.value = value
            elif kind == daq.PropertyType.Dict:
                value[utils.value_to_coretype(path_diff[0], prop.key_type)] = utils.value_to_coretype(
                    self.clipboard_get(), prop.item_type)
                prop.value = value
            else:
                prop.value = utils.value_to_coretype(
                    self.clipboard_get(), prop.value_type)

            self.refresh()

        except Exception as e:
            utils.show_error('Paste error', f'Can\'t paste: {e}', parent=self)

    def handle_clear_property_value(self):
        selected_item_id = utils.treeview_get_first_selection(self)
        if selected_item_id is None:
            return

        path = utils.get_item_path(self, selected_item_id)
        if not path or not daq.IPropertyObject.can_cast_from(self.node):
            return

        component = daq.IPropertyObject.cast_from(self.node)
        for segment in path[:-1]:
            child = component.get_property_value(segment)
            if not daq.IPropertyObject.can_cast_from(child):
                return
            component = daq.IPropertyObject.cast_from(child)

        component.clear_property_value(path[-1])
        self.refresh()

    def handle_clear_property_values(self):
        selected_item_id = utils.treeview_get_first_selection(self)
        if selected_item_id is None:
            return

        path = utils.get_item_path(self, selected_item_id)
        prop = utils.get_property_for_path(self.context, path, self.node)
        if prop is None:
            return

        if self._property_kind(prop) == daq.PropertyType.Object \
                and daq.IPropertyObject.can_cast_from(prop.value):
            obj = daq.IPropertyObject.cast_from(prop.value)
            obj.clear_property_values()
        elif daq.IPropertyObject.can_cast_from(self.node):
            component = daq.IPropertyObject.cast_from(self.node)
            for segment in path[:-1]:
                child = component.get_property_value(segment)
                if not daq.IPropertyObject.can_cast_from(child):
                    return
                component = daq.IPropertyObject.cast_from(child)
            component.clear_property_value(path[-1])

        self.refresh()

    def show_menu(self, event):
        if event is None:
            return
        region = self.identify_region(event.x, event.y)
        menu = tk.Menu(self, tearoff=0)
        if region == 'heading':
            menu.add_command(
                label='Select columns',
                command=lambda: MetadataFieldsSelectorDialog(self, self.context).show()
            )
        else:
            utils.treeview_select_item(self, event)
            selected_item_id = utils.treeview_get_first_selection(self)
            if not selected_item_id:
                return

            path = utils.get_item_path(self, selected_item_id)
            prop = self._row_props.get(selected_item_id) or \
                utils.get_property_for_path(self.context, path, self.node)

            # rows of list/dict items resolve to the containing property
            parent_prop = None
            if prop is None and len(path) > 1:
                parent_prop = self._row_parent_props.get(selected_item_id) or \
                    utils.get_property_for_path(self.context, path[:-1], self.node)

            is_container = prop is not None \
                and self._property_kind(prop) in self.CONTAINER_KINDS
            is_readonly = 'readonly' in self.item(selected_item_id, 'tags')
            if not is_container:
                menu.add_command(label='Copy', command=self.handle_copy)
            if not self.read_only and not is_readonly and not is_container:
                menu.add_command(label='Paste', command=self.handle_paste)
            if not self.read_only and not is_container and not is_readonly:
                menu.add_command(label='Clear property value', command=self.handle_clear_property_value)
            if not self.read_only and is_container:
                menu.add_command(label='Clear property values', command=self.handle_clear_property_values)
            if not self.read_only and prop is not None \
                    and self._property_kind(prop) in self.ITEM_CONTAINER_KINDS \
                    and not prop.read_only:
                menu.add_command(
                    label='Add item',
                    command=lambda p=prop: self.handle_add_container_item(p))
            if not self.read_only and not is_readonly and parent_prop is not None \
                    and self._property_kind(parent_prop) in self.ITEM_CONTAINER_KINDS \
                    and not parent_prop.read_only:
                menu.add_command(
                    label='Delete item',
                    command=lambda p=parent_prop, k=path[-1]:
                        self.handle_delete_container_item(p, k))
            if not is_container:
                menu.add_separator()

            menu.add_command(label='Metadata', command=self.handle_show_metadata)
            
        menu.tk_popup(event.x_root, event.y_root)

    # returns the real dict key whose display text matches the tree row
    def _dict_key_for_text(self, prop, key_text):
        for key in prop.value.keys():
            if str(key) == key_text:
                return key
        return self._coerce_item_value(key_text, prop.key_type)

    # properties do not always carry an item/key type; fall back to inferring
    # the type from the entered text then
    @staticmethod
    def _coerce_item_value(raw, core_type):
        if core_type is None or core_type == daq.CoreType.ctUndefined:
            return utils.str_to_num_or_eval(raw)
        return utils.value_to_coretype(raw, core_type)

    def handle_add_container_item(self, prop):
        try:
            data = prop.value
            if self._property_kind(prop) == daq.PropertyType.Dict:
                key = simpledialog.askstring('Add item', 'Key:', parent=self)
                if key is None:
                    return
                self.update()  # let the next dialog grab focus
                value = simpledialog.askstring('Add item', 'Value:', parent=self)
                if value is None:
                    return
                data[self._coerce_item_value(key, prop.key_type)] = \
                    self._coerce_item_value(value, prop.item_type)
            else:
                value = simpledialog.askstring('Add item', 'Value:', parent=self)
                if value is None:
                    return
                data.append(self._coerce_item_value(value, prop.item_type))
            prop.value = data
            self.refresh()
        except Exception as e:
            utils.show_error('Add item error', f'Can\'t add item: {e}', self)

    def handle_delete_container_item(self, prop, key_text):
        try:
            data = prop.value
            if self._property_kind(prop) == daq.PropertyType.Dict:
                del data[self._dict_key_for_text(prop, key_text)]
            else:
                del data[int(key_text)]
            prop.value = data
            self.refresh()
        except Exception as e:
            utils.show_error('Delete item error', f'Can\'t delete item: {e}', self)

    def update_property(self, component, path, new_value, depth=0):
        for property in self.context.properties_of_component(component):
            if property.name == path[depth]:
                if depth == len(path) - 1:
                    component.set_property_value(property.name, new_value)
                    return
                prop = component.get_property_value(property.name)
                if isinstance(prop, daq.IBaseObject) and daq.IPropertyObject.can_cast_from(prop):
                    cast_property = daq.IPropertyObject.cast_from(prop)
                    self.update_property(cast_property, path, new_value, depth + 1)

    def save_simple_value(self, entry, path):
        new_value = entry.get()
        try:
            self.update_property(self.node, path, new_value)
            self.refresh()
        except Exception:
            pass
        finally:
            entry.destroy()

    def save_struct_value(self, entry, parent, name):
        new_raw = entry.get()

        try:
            old_struct = parent.value
            old_dict = old_struct.as_dictionary

            # Convert to same Python type
            old_val = old_dict[name]
            ty = type(old_val)

            new_val = ty(new_raw)

            # Special handling for protected struct types
            struct_type_name = old_struct.struct_type.name

            new_dict = daq.Dict()
            for k, v in old_dict.items():
                new_dict[k] = new_val if k == name else v

            if struct_type_name == "Range":
                new_struct = daq.Range(new_dict["LowValue"], new_dict["HighValue"])
            elif struct_type_name == "Unit":
                new_struct = daq.Unit(new_dict["Id"], new_dict["Symbol"], new_dict["Name"], new_dict["Quantity"])
            else:
                tm = self.context.instance.context.type_manager
                new_struct = daq.Struct(
                    daq.String(struct_type_name),
                    new_dict,
                    tm
                )

            parent.value = new_struct
            self.refresh()

        except Exception as e:
            print("Failed to set value:", e)
        finally:
            entry.destroy()
        
    def _clear_overlay_comboboxes(self):
        self._active_dropdown_cb = None
        for cb in self._overlay_comboboxes.values():
            try:
                cb.destroy()
            except Exception:
                pass
        self._overlay_comboboxes = {}

    def _post_combobox_dropdown(self, cb):
        try:
            self.tk.call('ttk::combobox::Post', cb)
        except Exception:
            cb.event_generate('<Down>')

    def _unpost_combobox_dropdown(self, cb):
        try:
            self.tk.call('ttk::combobox::Unpost', cb)
        except Exception:
            try:
                cb.event_generate('<Escape>')
            except Exception:
                pass
    def _get_selection_options(self, selection_values):
        labels, indices = [], []

        if daq.IDict.can_cast_from(selection_values):
            for idx, label in daq.IDict.cast_from(selection_values).items():
                labels.append(self._format_value(label))
                indices.append(idx)
        else:
            for i, label in enumerate(daq.IList.cast_from(selection_values)):
                labels.append(self._format_value(label))
                indices.append(i)
        return labels, indices

    def _collect_overlay_items(self, parent_iid):
        if self.read_only:
            return

        for iid, prop in self._row_props.items():
            if prop is not None:
                kind = self._property_kind(prop)
                if kind in self.METHOD_KINDS:
                    self._overlay_items[iid] = prop
                elif not prop.read_only:
                    if (kind == daq.PropertyType.Bool
                            or (kind in self.SELECTION_KINDS
                                and prop.selection_values is not None
                                and len(prop.selection_values) > 1)
                            or kind == daq.PropertyType.Enumeration
                            or (kind in self.SCALAR_KINDS
                                and prop.suggested_values is not None
                                and len(prop.suggested_values) > 0)):
                        self._overlay_items[iid] = prop

    def _create_overlay_for_item(self, iid, prop):
        kind = self._property_kind(prop)
        if kind in self.METHOD_KINDS:
            self._place_method_button(iid, prop)
        elif kind == daq.PropertyType.Bool:
            self._place_bool_checkbox(iid, prop)
        elif kind in self.SELECTION_KINDS \
                and prop.selection_values is not None \
                and len(prop.selection_values) > 0:
            self._place_selection_combobox(iid, prop)
        elif kind == daq.PropertyType.Enumeration:
            self._place_enum_combobox(iid, prop)
        elif (kind in self.SCALAR_KINDS
              and prop.suggested_values is not None and len(prop.suggested_values) > 0):
            self._place_suggested_combobox(iid, prop)

    def _sync_overlays(self):
        if self._syncing_overlays:
            return
        self._syncing_overlays = True
        try:
            if not self.winfo_viewable():
                return

            sb_x_h = self._scroll_bar_x.winfo_height() if self._scroll_bar_x.winfo_ismapped() else 0
            sb_y_w = self._scroll_bar.winfo_width() if self._scroll_bar.winfo_ismapped() else 0
            visible_w = self.winfo_width() - sb_y_w
            visible_h = self.winfo_height() - sb_x_h

            for iid, item_prop in self._overlay_items.items():
                is_method = self._property_kind(item_prop) in self.METHOD_KINDS
                bbox = self.bbox(iid, '#0' if is_method else '#1')
                if bbox:
                    if iid not in self._overlay_comboboxes:
                        self._create_overlay_for_item(iid, item_prop)

                    if iid in self._overlay_comboboxes:
                        x, py, w, ph = self._get_overlay_place_geometry(bbox)
                        
                        if is_method:
                            indent = self._tree_indent()
                            x += indent
                            w = max(1, w - indent)
                        
                        if x >= visible_w or (x + w) <= 0:
                            fully_visible = False
                        else:
                            w = min(w, visible_w - x)
                            fully_visible = py >= 0 and py + ph <= visible_h
                            
                        
                        if fully_visible:
                            self._overlay_comboboxes[iid].place(x=x, y=py, width=w, height=ph)
                            self._overlay_comboboxes[iid].lift()
                        else:
                            self._overlay_comboboxes[iid].place_forget()
                else:
                    if iid in self._overlay_comboboxes:
                        self._overlay_comboboxes[iid].place_forget()
        finally:
            self._syncing_overlays = False

    def _make_combobox(self, iid, values, current_value, editable=False):
        bbox = self.bbox(iid, '#1')
        if not bbox:
            return None
        x, combo_y, width, combo_height = self._get_overlay_place_geometry(bbox)
        #width -= self._scroll_bar.winfo_width()

        state = 'normal' if editable else 'readonly'
        combo_style = 'Editable.TCombobox' if editable else 'Selection.TCombobox'
        cb = ttk.Combobox(self, values=values, state=state, style=combo_style)
        cb.set(current_value)
        cb.place(x=x, y=combo_y, width=width, height=combo_height)
        if not editable:
            cb.configure(takefocus=False)
            
            def _toggle_dropdown(e, _cb=cb):
                # Manually toggle dropdown and block default Tk click handling
                # to avoid close-on-press and reopen-on-release behavior.
                active = self._active_dropdown_cb
                if active is _cb:
                    self._unpost_combobox_dropdown(_cb)
                    self._active_dropdown_cb = None
                    return 'break'

                if active is not None:
                    try:
                        if active.winfo_exists():
                            self._unpost_combobox_dropdown(active)
                    except Exception:
                        pass

                self._active_dropdown_cb = _cb
                _cb.focus_set()
                _cb.after_idle(lambda: self._post_combobox_dropdown(_cb))
                return 'break'

            def _clear_active_if_needed(e, _cb=cb):
                if self._active_dropdown_cb is _cb:
                    self._active_dropdown_cb = None

            cb.bind('<Button-1>', _toggle_dropdown)
            cb.bind('<Escape>', _clear_active_if_needed)
            cb.bind('<FocusOut>', _clear_active_if_needed)
            cb.bind('<KeyPress>', lambda e : 'break')

        self._bind_overlay_mousewheel(cb)
        return cb

    # True when the combobox dropdown list is currently open
    def _combobox_posted(self, cb):
        try:
            popdown = self.tk.call('ttk::combobox::PopdownWindow', cb)
            return bool(int(self.tk.call('winfo', 'viewable', popdown)))
        except (tk.TclError, ValueError):
            return False

    # forwards the mouse wheel over an overlay widget to the tree, so the
    # view keeps scrolling; an open combobox dropdown keeps the wheel for
    # scrolling its own list
    def _bind_overlay_mousewheel(self, widget):
        def scroll(units):
            if isinstance(widget, ttk.Combobox) and self._combobox_posted(widget):
                return 'break'
            self.yview_scroll(units, 'units')
            self._sync_overlays()
            return 'break'

        widget.bind('<MouseWheel>', lambda e: scroll(int(-1 * (e.delta / 120))))
        widget.bind('<Button-4>', lambda e: scroll(-1))
        widget.bind('<Button-5>', lambda e: scroll(1))

    def _place_bool_checkbox(self, iid, prop):
        bbox = self.bbox(iid, '#1')
        if not bbox:
            return
        x, y, width, height = self._get_overlay_place_geometry(bbox)
        var = tk.BooleanVar(value=bool(prop.value))
        cb = ttk.Checkbutton(self, variable=var, takefocus=False, style='Overlay.TCheckbutton')
        cb.place(x=x, y=y, width=width, height=height)

        def on_change(_prop=prop, _var=var):
            try:
                _prop.value = _var.get()
            except Exception as e:
                print("Failed to set bool:", e)
                return
            self.refresh()

        cb.configure(command=on_change)
        self._bind_overlay_mousewheel(cb)
        self._overlay_comboboxes[iid] = cb

    def _place_method_button(self, iid, prop):
        bbox = self.bbox(iid, '#0')
        if not bbox:
            return
        x, y, width, height = self._get_overlay_place_geometry(bbox)
        
        indent = self._tree_indent()
        x += indent
        width = max(1, width - indent)

        def execute(_prop=prop, _iid=iid):
            result = None
            has_args = bool(_prop.callable_info.arguments)
            
            method_class = daq.IFunction \
                if self._property_kind(_prop) == daq.PropertyType.Function \
                else daq.IProcedure
            method = method_class.cast_from(_prop.value)

            if has_args:
                dialog = FunctionDialog(self, _prop, method, self.context)
                dialog.show()
                result = dialog.result
            else:
                try:
                    res = method()
                    result = res if _prop.value_type == daq.CoreType.ctFunc else True
                except Exception as e:
                    result = e

            if result is True or result is False:
                result_str = 'OK' if result else 'Fail'
            elif isinstance(result, Exception):
                result_str = str(result)
            elif isinstance(result, str) and result == '':
                result_str = '""'
            else:
                result_str = self._format_value(result)

            self._last_method_results[_prop.name] = result_str
            if self.exists(_iid):
                self.set(_iid, 'value', result_str)
                
        btn = ttk.Button(self, text=prop.name, command=execute)
        btn.place(x=x, y=y, width=width, height=height)
        self._bind_overlay_mousewheel(btn)
        self._overlay_comboboxes[iid] = btn
        
    def _tree_indent(self):
        try:
            return int(str(self.tk.call(
                "ttk::style", "lookup", "Treeview", "-indent")))
        except Exception:
            return 20

    def _place_selection_combobox(self, iid, prop):
        labels, indices = self._get_selection_options(prop.selection_values)
        unit_symbol = utils.prettify_unit(prop.unit)
        if unit_symbol:
            labels = [f'{l} {unit_symbol}' for l in labels]
        if not labels:
            return
        if prop.item_type != daq.CoreType.ctUndefined:
            current_idx = prop.value
            current_label = labels[indices.index(current_idx)] if current_idx in indices else labels[0]
        else:
            current_label = self._format_value(prop.value)
            
        cb = self._make_combobox(iid, labels, current_label)
        if cb is None:
            return

        def on_change(event, _prop=prop, _labels=labels, _indices=indices, _cb=cb):
            try:
                if _prop.item_type != daq.CoreType.ctUndefined:
                    _prop.value = _indices[_labels.index(_cb.get())]
                else:
                    _prop.value = _cb.get()
            except Exception as e:
                print("Failed to set selection:", e)
                return
            self.refresh()

        cb.bind('<<ComboboxSelected>>', on_change)
        self._overlay_comboboxes[iid] = cb

    def _place_enum_combobox(self, iid, prop):
        if not daq.IEnumeration.can_cast_from(prop.value):
            return
        enum = daq.IEnumeration.cast_from(prop.value)
        enum_type = enum.enumeration_type
        keys = [k for k, _ in enum_type.as_dictionary.items()]
        current_key = keys[enum.value] if 0 <= enum.value < len(keys) else keys[0]
        cb = self._make_combobox(iid, keys, current_key)
        if cb is None:
            return

        def on_change(event, _prop=prop, _enum_type=enum_type, _cb=cb):
            try:
                _prop.value = daq.Enumeration(
                    daq.String(_enum_type.name),
                    daq.String(_cb.get()),
                    self.context.instance.context.type_manager,
                )
            except Exception as e:
                print("Failed to set enum:", e)
                return
            self.refresh()

        cb.bind('<<ComboboxSelected>>', on_change)
        self._overlay_comboboxes[iid] = cb

    def _place_suggested_combobox(self, iid, prop):
        sv = prop.suggested_values
        suggestions = []
        if daq.IList.can_cast_from(sv):
            suggestions = [self._format_value(v) for v in daq.IList.cast_from(sv)]
        elif daq.IDict.can_cast_from(sv):
            suggestions = [self._format_value(v) for _, v in daq.IDict.cast_from(sv).items()]
        if not suggestions:
            return

        unit_symbol = utils.prettify_unit(prop.unit)
        current_display = self._format_value(prop.value)
        if unit_symbol:
            suggestions = [f'{s} {unit_symbol}' for s in suggestions]
            current_display = f'{current_display} {unit_symbol}'

        cb = self._make_combobox(iid, suggestions, current_display, editable=True)
        if cb is None:
            return

        def save(_cb=cb, _prop=prop, _unit=unit_symbol):
            raw = _cb.get()
            if _unit:
                raw = raw.removesuffix(f' {_unit}').strip()
            _prop.value = utils.value_to_coretype(raw, _prop.value_type)
            self.refresh()

        cb.bind('<Return>', lambda e: save())
        cb.bind('<<ComboboxSelected>>', lambda e: save())
        self._overlay_comboboxes[iid] = cb

    def _on_configure(self, event):
        try:
            w, h = self.winfo_width(), self.winfo_height()
            if (w, h) != self._last_configure_size and w > 1 and h > 1:
                self._last_configure_size = (w, h)
                self.after_idle(self._sync_overlays)
        except tk.TclError:
            pass

    def _get_overlay_place_geometry(self, bbox):
        x, y, width, height = bbox
        vertical_inset = max(1, int(self.context.dpi_factor))
        place_y = y + vertical_inset
        place_height = max(1, height - 2 * vertical_inset)
        return x, place_y, width, place_height

    def edit_struct_property(self, selected_item_id, name, parent):
        x, y, width, height = self.bbox(selected_item_id, '#1')
        entry = ttk.Entry(self)
        entry.place(x=x, y=y, width=width, height=height)
        entry.insert(0, parent.value.get(name))
        entry.focus()
        entry.bind('<Return>', lambda e: self.save_struct_value(entry, parent, name))
        entry.bind('<FocusOut>', lambda e: self.save_struct_value(entry, parent, name))

    def edit_simple_property(self, selected_item_id, property_value, path):
        x, y, width, height = self.bbox(selected_item_id, '#1')
        entry = ttk.Entry(self)
        entry.place(x=x, y=y, width=width, height=height)
        entry.insert(0, property_value)
        entry.focus()
        entry.bind('<Return>', lambda e: self.save_simple_value(entry, path))
        entry.bind('<FocusOut>', lambda e: self.save_simple_value(entry, path))

    # in-place editor for one list/dict item row; the row itself carries the
    # index or key, so no separate editor window is needed
    def edit_container_item(self, selected_item_id, parent_prop, key_text):
        bbox = self.bbox(selected_item_id, '#1')
        if not bbox:
            return
        x, y, width, height = bbox

        data = parent_prop.value
        if self._property_kind(parent_prop) == daq.PropertyType.Dict:
            current = data[self._dict_key_for_text(parent_prop, key_text)]
        else:
            current = data[int(key_text)]

        entry = ttk.Entry(self)
        entry.place(x=x, y=y, width=width, height=height)
        entry.insert(0, str(current))
        entry.focus()

        def save(event=None):
            if not entry.winfo_exists():
                return
            try:
                new_value = self._coerce_item_value(
                    entry.get(), parent_prop.item_type)
                items = parent_prop.value
                if self._property_kind(parent_prop) == daq.PropertyType.Dict:
                    items[self._dict_key_for_text(parent_prop, key_text)] = new_value
                else:
                    items[int(key_text)] = new_value
                parent_prop.value = items
                self.refresh()
            except Exception as e:
                utils.show_error('Edit item error', f'Can\'t edit item: {e}', self)
            finally:
                entry.destroy()

        def cancel(event=None):
            entry.destroy()

        entry.bind('<Return>', save)
        entry.bind('<FocusOut>', save)
        entry.bind('<Escape>', cancel)

    def edit_value(self):
        selected_item_id = utils.treeview_get_first_selection(self)
        if selected_item_id is None:
            return

        name = self.item(selected_item_id, 'text')
        path = utils.get_item_path(self, selected_item_id)

        # handle struct
        if len(path) > 1 and selected_item_id in self._row_parent_props:
            parent = self._row_parent_props.get(selected_item_id) or \
                utils.get_property_for_path(self.context, path[:-1], self.node)
            
            if 'readonly' in self.item(selected_item_id, 'tags'):
                return
 
            parent_kind = self._property_kind(parent)
            if type(parent.value) is complex or type(parent.value) is Fraction:
                return
            elif parent_kind == daq.PropertyType.Struct:
                self.edit_struct_property(selected_item_id, name, parent)
                return
            elif parent_kind in self.ITEM_CONTAINER_KINDS:
                if not parent.read_only:
                    self.edit_container_item(selected_item_id, parent, name)
                return

        prop = self._row_props.get(selected_item_id) or \
            utils.get_property_for_path(self.context, path, self.node)

        if not prop:
            return

        kind = self._property_kind(prop)

        if kind == daq.PropertyType.Enumeration:
            return  # handled by overlay combobox

        if kind in self.METHOD_KINDS:
            method_class = daq.IFunction if kind == daq.PropertyType.Function else daq.IProcedure
            method = method_class.cast_from(prop.value)
            
            if prop.callable_info.arguments:
                dialog = FunctionDialog(self, prop, method, self.context)
                dialog.show()
                result = dialog.result
            else:
                try:
                    res = method()
                    result = res if prop.value_type == daq.CoreType.ctFunc else True
                except Exception as e:
                    result = e

            if result is True or result is False:
                result_str = 'OK' if result else 'Fail'
            elif isinstance(result, Exception):
                result_str = str(result)
            elif isinstance(result, str) and result == '':
                result_str = '""'
            else:
                result_str = self._format_value(result)
                
            self._last_method_results[prop.name] = result_str
            if self.exists(selected_item_id):
                self.set(selected_item_id, 'value', result_str)
            return

        if prop.read_only:
            return

        if kind == daq.PropertyType.Bool:
            return  # handled by overlay combobox
        elif kind in self.SELECTION_KINDS:
            return  # handled by overlay combobox
        elif kind in self.ITEM_CONTAINER_KINDS:
            return  # items are edited in place, right click adds/removes them
        elif kind in self.SCALAR_KINDS:
            if prop.suggested_values is not None and len(prop.suggested_values) > 0:
                return  # handled by overlay combobox
            self.edit_simple_property(selected_item_id, prop.value, path)

    def _on_destroy(self, event):
        if event.widget is self:
            try:
                self.winfo_toplevel().unbind('<<DialogReady>>', self._toplevel_bind_id)
            except Exception:
                pass

    # display name of an enumeration property's current value
    @staticmethod
    def _enum_value_name(value):
        try:
            if daq.IEnumeration.can_cast_from(value):
                enum = daq.IEnumeration.cast_from(value)
                keys = list(enum.enumeration_type.as_dictionary.keys())
                if 0 <= enum.value < len(keys):
                    return keys[enum.value]
        except RuntimeError:
            pass
        return str(value)

    @staticmethod
    def _format_value(value):
        try:
            f = float(value)
            if f == int(f):
                return str(int(f)).strip()
            rounded = float(f'{f:.7g}')
            return str(rounded).strip()
        except Exception:
            return str(value)

