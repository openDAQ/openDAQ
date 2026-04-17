import tkinter as tk
from tkinter import ttk
from fractions import Fraction

import opendaq as daq

from .. import utils
from ..app_context import AppContext
from .function_dialog import FunctionDialog
from .edit_container_property import EditContainerPropertyDialog
from .metadata_dialog import MetadataDialog
from .metadata_fields_selector_dialog import MetadataFieldsSelectorDialog


class PropertiesTreeview(ttk.Treeview):
    def __init__(self, parent, node=None, context: AppContext = None, read_only=False, **kwargs):
        self.hidden = kwargs.pop("hidden", [])
        self._metadata_fields = list(context.metadata_fields)
        ttk.Treeview.__init__(self, parent, columns=('value', *self._metadata_fields), show='tree headings', **kwargs)

        self.context = context
        self.node = node
        self._overlay_comboboxes = {}
        self._overlay_items = {}
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
            self.bind('<Double-1>', lambda event: self.edit_value())
        self.bind('<Button-3>', lambda event: self.show_menu(event))
        self.bind('<MouseWheel>', lambda e: self.after_idle(self._sync_overlays))
        self.bind('<ButtonRelease-1>', lambda e: self.after(10, self._sync_overlays), add='+')
        self.bind('<Configure>', self._on_configure)
        self.bind('<Map>', lambda e: self.after_idle(self._sync_overlays) if e.widget is self else None)
        self.winfo_toplevel().bind('<<DialogReady>>', lambda e: self._sync_overlays(), add='+')

        self.refresh()

    def refresh(self):
        collapsed = self._collect_collapsed_paths('')
        scroll_pos = self.yview()

        self._clear_overlay_comboboxes()
        self._overlay_items = {}
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

    def fill_list(self, parent_iid, l, read_only):
        for i, value in enumerate(l):
            iid = self.insert('' if not parent_iid else parent_iid,
                             tk.END, text=str(i), values=(self._format_value(value),))
            if read_only:
                self.item(iid, tags=('readonly',))

    def fill_dict(self, parent_iid, d, read_only):
        for key, value in d.items():
            iid = self.insert('' if not parent_iid else parent_iid,
                             tk.END, text=str(key), values=(self._format_value(value),))
            if read_only:
                self.item(iid, tags=('readonly',))

    def fill_struct(self, parent_iid, node, read_only):
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
            if read_only:
                self.item(iid, tags=('readonly',))

    def fill_properties(self, parent_iid, node, hidden):
        def printed_value(value_type, value):
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

            if property_info.selection_values is not None and property_info.item_type != daq.CoreType.ctUndefined:
                if len(property_info.selection_values) > 0:
                    property_value = printed_value(
                        property_info.item_type, node.get_property_selection_value(property_info.name))
                else:
                    property_value = 'Selection list is empty'
            elif property_info.value_type == daq.CoreType.ctProc:
                property_value = self._last_method_results.get(property_info.name, '')
            elif property_info.value_type == daq.CoreType.ctFunc:
                property_value = self._last_method_results.get(property_info.name, '')
            elif property_info.value_type == daq.CoreType.ctStruct:
                property_value = ''
            elif property_info.value_type == daq.CoreType.ctObject:
                property_value = ''
            elif property_info.value_type in (daq.CoreType.ctList, daq.CoreType.ctDict):
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
            iid = self.insert(
                '' if not parent_iid else parent_iid,
                tk.END,
                open=True,
                text=property_info.name,
                values=(property_value, *meta_fields))

            container_types = (daq.CoreType.ctObject, daq.CoreType.ctStruct, daq.CoreType.ctList, daq.CoreType.ctDict)
            is_single_value_selection = (
                property_info.selection_values is not None
                and len(property_info.selection_values) == 1
            )
            if property_info.value_type not in (daq.CoreType.ctFunc, daq.CoreType.ctProc):
                if property_info.value_type not in container_types:
                    if property_info.read_only or self.read_only or is_single_value_selection:
                        self.item(iid, tags=('readonly',))

            if property_info.value_type == daq.CoreType.ctObject:
                hidden_children = [s.removeprefix(f"{property_info.name}.") for s in hidden if s.startswith(f"{property_info.name}.")]
                self.fill_properties(
                    iid, node.get_property_value(property_info.name), hidden_children)  
            elif property_info.value_type == daq.CoreType.ctStruct:
                self.fill_struct(
                    iid, node.get_property_value(property_info.name), property_info.read_only)
            elif property_info.value_type == daq.CoreType.ctList:
                self.fill_list(iid, node.get_property_value(property_info.name), property_info.read_only)
            elif property_info.value_type == daq.CoreType.ctDict:
                self.fill_dict(iid, node.get_property_value(property_info.name), property_info.read_only)

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
            if prop.value_type == daq.CoreType.ctObject:
                pass  # ignoring paste to objects
            elif prop.value_type == daq.CoreType.ctStruct:
                field_type = [
                    field for field in prop.struct_type.field_types if field.name == path_diff[0]][0].core_type
                setattr(value, path_diff[0], utils.value_to_coretype(
                    self.clipboard_get(), field_type))
                prop.value = value
            elif prop.value_type == daq.CoreType.ctList:
                value[int(path_diff[0])] = utils.value_to_coretype(
                    self.clipboard_get(), prop.item_type)
                prop.value = value
            elif prop.value_type == daq.CoreType.ctDict:
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

        if prop.value_type == daq.CoreType.ctObject and daq.IPropertyObject.can_cast_from(prop.value):
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
            prop = utils.get_property_for_path(self.context, path, self.node)
            
            is_container = False
            if prop:
                container_types = (daq.CoreType.ctObject, daq.CoreType.ctStruct, 
                                   daq.CoreType.ctList, daq.CoreType.ctDict)
                is_container = prop.value_type in container_types

            is_readonly = 'readonly' in self.item(selected_item_id, 'tags')
            if not is_container:
                menu.add_command(label='Copy', command=self.handle_copy)
            if not self.read_only and not is_readonly and not is_container:
                menu.add_command(label='Paste', command=self.handle_paste)
            if not self.read_only and not is_container and not is_readonly:
                menu.add_command(label='Clear property value', command=self.handle_clear_property_value)
            if not self.read_only and is_container:
                menu.add_command(label='Clear property values', command=self.handle_clear_property_values)
            if not is_container:
                menu.add_separator()
                
            menu.add_command(label='Metadata', command=self.handle_show_metadata)
            
        menu.tk_popup(event.x_root, event.y_root)

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

        for iid in self.get_children(parent_iid):
            path = utils.get_item_path(self, iid)
            prop = utils.get_property_for_path(self.context, path, self.node)
            if prop:
                if prop.value_type in (daq.CoreType.ctFunc, daq.CoreType.ctProc):
                    self._overlay_items[iid] = prop
                elif not prop.read_only:
                    if (prop.value_type == daq.CoreType.ctBool
                            or (prop.selection_values is not None and len(prop.selection_values) > 1)
                            or prop.value_type == daq.CoreType.ctEnumeration
                            or (prop.value_type in (daq.CoreType.ctString, daq.CoreType.ctFloat, daq.CoreType.ctInt)
                                and prop.suggested_values is not None and len(prop.suggested_values) > 0)):
                        self._overlay_items[iid] = prop
            self._collect_overlay_items(iid)

    def _create_overlay_for_item(self, iid, prop):
        if prop.value_type in (daq.CoreType.ctFunc, daq.CoreType.ctProc):
            self._place_method_button(iid, prop)
        elif prop.value_type == daq.CoreType.ctBool:
            self._place_bool_checkbox(iid, prop)
        elif prop.selection_values is not None and len(prop.selection_values) > 0:
            self._place_selection_combobox(iid, prop)
        elif prop.value_type == daq.CoreType.ctEnumeration:
            self._place_enum_combobox(iid, prop)
        elif (prop.value_type in (daq.CoreType.ctString, daq.CoreType.ctFloat, daq.CoreType.ctInt)
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
                is_method = item_prop.value_type in (daq.CoreType.ctFunc, daq.CoreType.ctProc)
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

        cb.bind('<MouseWheel>', lambda e: 'break')
        cb.bind('<Button-4>', lambda e: 'break')
        cb.bind('<Button-5>', lambda e: 'break')
        return cb

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
        def _on_mousewheel(e):
            self.yview_scroll(int(-1 * (e.delta / 120)), 'units')
            self._sync_overlays()
            return 'break'

        cb.bind('<MouseWheel>', _on_mousewheel)
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
            
            method_class = daq.IFunction if _prop.value_type == daq.CoreType.ctFunc else daq.IProcedure
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

    def edit_value(self):
        selected_item_id = utils.treeview_get_first_selection(self)
        if selected_item_id is None:
            return

        name = self.item(selected_item_id, 'text')
        path = utils.get_item_path(self, selected_item_id)

        # handle struct
        if len(path) > 1:
            parent = utils.get_property_for_path(self.context, path[:-1], self.node)
            
            if 'readonly' in self.item(selected_item_id, 'tags'):
                return
 
            if type(parent.value) is complex or type(parent.value) is Fraction:
                return 
            elif parent.value_type == daq.CoreType.ctStruct:
                self.edit_struct_property(selected_item_id, name, parent)
                return
            elif parent.value_type == daq.CoreType.ctList:
                EditContainerPropertyDialog(self, parent, self.context).show()
                self.refresh()
                return

        prop = utils.get_property_for_path(self.context, path, self.node)

        if not prop:
            return

        if prop.value_type == daq.CoreType.ctEnumeration:
            return  # handled by overlay combobox

        if prop.value_type in (daq.CoreType.ctFunc, daq.CoreType.ctProc):
            method_class = daq.IFunction if prop.value_type == daq.CoreType.ctFunc else daq.IProcedure
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

        if prop.value_type == daq.CoreType.ctBool:
            return  # handled by overlay combobox
        elif prop.selection_values is not None:
            return  # handled by overlay combobox
        elif prop.value_type in (daq.CoreType.ctDict, daq.CoreType.ctList):
            EditContainerPropertyDialog(self, prop, self.context).show()
            self.refresh()
        elif prop.value_type in (daq.CoreType.ctString, daq.CoreType.ctFloat, daq.CoreType.ctInt):
            if prop.suggested_values is not None and len(prop.suggested_values) > 0:
                return  # handled by overlay combobox
            self.edit_simple_property(selected_item_id, prop.value, path)

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

