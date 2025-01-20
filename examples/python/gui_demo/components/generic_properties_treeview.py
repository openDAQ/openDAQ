import tkinter as tk
from tkinter import ttk

import opendaq as daq

from .. import utils
from ..app_context import AppContext
from .function_dialog import FunctionDialog
from .edit_container_property import EditContainerPropertyDialog
from .metadata_dialog import MetadataDialog


class PropertiesTreeview(ttk.Treeview):
    def __init__(self, parent, node=None, context: AppContext = None, **kwargs):
        ttk.Treeview.__init__(self, parent, columns=('value', *context.metadata_fields), show='tree headings', **kwargs)

        self.context = context
        self.node = node

        scroll_bar = ttk.Scrollbar(
            self, orient=tk.VERTICAL, command=self.yview)
        self.configure(yscrollcommand=scroll_bar.set)
        scroll_bar.pack(side=tk.RIGHT, fill=tk.Y)
        scroll_bar_x = ttk.Scrollbar(
            self, orient=tk.HORIZONTAL, command=self.xview)
        self.configure(xscrollcommand=scroll_bar_x.set)
        scroll_bar_x.pack(side=tk.BOTTOM, fill=tk.X)
        self.pack(fill=tk.BOTH, expand=True)

        self.tag_configure('readonly', foreground='gray')

        # define headings
        self.heading('#0', anchor=tk.W, text='Property name')
        self.heading('value', anchor=tk.W, text='Value')
        # layout
        self.column('#0', anchor=tk.W, minwidth=200)
        self.column('#1', anchor=tk.W, minwidth=200)

        for field in self.context.metadata_fields:
            self.heading(field, anchor=tk.W,
                         text=utils.snake_case_to_title(field))
            self.column(field, anchor=tk.W, minwidth=200)

        # bind double-click to editing
        self.bind('<Double-1>', lambda event: self.edit_value())
        self.bind('<Button-3>', lambda event: self.show_menu(event))

        self.refresh()

    def refresh(self):
        self.delete(*self.get_children())
        if self.node is not None:
            if daq.IPropertyObject.can_cast_from(self.node):
                self.fill_properties('', daq.IPropertyObject.cast_from(self.node))

    def fill_list(self, parent_iid, l, read_only):
        for i, value in enumerate(l):
            iid = self.insert('' if not parent_iid else parent_iid,
                             tk.END, text=str(i), values=(str(value),))
            if read_only:
                self.item(iid, tags=('readonly',))

    def fill_dict(self, parent_iid, d, read_only):
        for key, value in d.items():
            iid = self.insert('' if not parent_iid else parent_iid,
                             tk.END, text=str(key), values=(str(value),))
            if read_only:
                self.item(iid, tags=('readonly',))

    def fill_struct(self, parent_iid, node, read_only):
        for key, value in node.as_dictionary.items():
            iid = self.insert('' if not parent_iid else parent_iid,
                             tk.END, text=key, values=(value,))
            if read_only:
                self.item(iid, tags=('readonly',))

    def fill_properties(self, parent_iid, node):
        def printed_value(value_type, value):
            if value_type == daq.CoreType.ctBool:
                return utils.yes_no[value]
            else:
                return value

        for property_info in self.context.properties_of_component(node):
            if property_info.selection_values is not None:
                if len(property_info.selection_values) > 0:
                    property_value = printed_value(
                        property_info.item_type, node.get_property_selection_value(property_info.name))
                else:
                    property_value = 'Selection list is empty'
            elif property_info.value_type == daq.CoreType.ctProc:
                property_value = 'Method'
            elif property_info.value_type == daq.CoreType.ctFunc:
                property_value = 'Method'
            elif property_info.value_type == daq.CoreType.ctStruct:
                property_value = 'Struct {{{}}}'.format(property_info.name)
            else:
                property_value = printed_value(
                    property_info.value_type, node.get_property_value(property_info.name))

            meta_fields = [None] * len(self.context.metadata_fields)

            try:
                for i, field in enumerate(self.context.metadata_fields):
                    metadata_value = getattr(property_info, field)
                    metadata_value = utils.metadata_converters[field](
                        metadata_value) if field in utils.metadata_converters else metadata_value
                    meta_fields[i] = metadata_value
            except Exception as e:
                print(e)

            unit_symbol = property_info.unit.symbol if property_info.unit is not None else ''
            iid = self.insert('' if not parent_iid else parent_iid, tk.END, open=True, text=property_info.name, values=(f'{property_value} {unit_symbol}', *meta_fields))
            if property_info.read_only:
                self.item(iid, tags=('readonly',))

            if property_info.value_type == daq.CoreType.ctObject:
                self.fill_properties(
                    iid, node.get_property_value(property_info.name))
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

    def show_menu(self, event):
        utils.treeview_select_item(self, event)

        menu = tk.Menu(self, tearoff=0)
        menu.add_command(label='Copy', command=self.handle_copy)
        menu.add_command(label='Paste',
                         command=self.handle_paste)
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

        property_name = self.item(selected_item_id, 'text')
        path = utils.get_item_path(self, selected_item_id)
        prop = utils.get_property_for_path(self.context, path, self.node)
        if not prop:
            return

        if prop.value_type == daq.CoreType.ctFunc:
            f = daq.IFunction.cast_from(prop.value)
            FunctionDialog(self, prop, f, self.context).show()
            return

        if prop.value_type == daq.CoreType.ctProc:
            p = daq.IProcedure.cast_from(prop.value)
            FunctionDialog(self, prop, p, self.context).show()
            return

        if prop.read_only:
            return

        if prop.value_type == daq.CoreType.ctBool:
            prop.value = not prop.value
            self.refresh() # is needed
        elif prop.value_type == daq.CoreType.ctInt and prop.selection_values is not None:
            prop.value = utils.show_selection('Enter the new value for {}:'.format(property_name),
                                              prop.value, prop.selection_values)
            self.refresh() # is needed
        elif prop.value_type in (daq.CoreType.ctDict, daq.CoreType.ctList):
            EditContainerPropertyDialog(self, prop, self.context).show()
            self.refresh() # TODO needed? check
        elif prop.value_type in (daq.CoreType.ctString, daq.CoreType.ctFloat, daq.CoreType.ctInt):
            self.edit_simple_property(selected_item_id, prop.value, path)
