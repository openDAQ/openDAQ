import tkinter as tk
from tkinter import ttk, simpledialog

import opendaq as daq

from .. import utils
from ..event_port import EventPort
from ..app_context import AppContext
from .function_dialog import FunctionDialog
from .edit_container_property import EditContainerPropertyDialog
from .metadata_dialog import MetadataDialog
from .metadata_fields_selector_dialog import MetadataFieldsSelectorDialog


class PropertiesView(ttk.Frame):
    def __init__(self, parent: ttk.Frame, node=None, context: AppContext = None, **kwargs):
        ttk.Frame.__init__(self, parent, **kwargs)
        self.parent = parent
        self.node = node
        self.context = context
        self.event_port = EventPort(self)

        self.configure(padding=(10, 5))

        header_frame = ttk.Frame(self)

        ttk.Label(header_frame, text='Properties').pack(side=tk.LEFT, pady=5)
        tk.Button(header_frame, text='Edit', image=self.context.icons['settings'], borderwidth=0,
                  command=lambda: MetadataFieldsSelectorDialog(
                      self, self.context).show()
                  ).pack(
            side=tk.RIGHT, padx=30)

        header_frame.pack(fill=tk.X)
        tree = ttk.Treeview(self, columns=(
            'value', *self.context.metadata_fields), show='tree headings')

        scroll_bar = ttk.Scrollbar(
            self, orient=tk.VERTICAL, command=tree.yview)
        tree.configure(yscrollcommand=scroll_bar.set)
        scroll_bar.pack(side=tk.RIGHT, fill=tk.Y)
        scroll_bar_x = ttk.Scrollbar(
            self, orient=tk.HORIZONTAL, command=tree.xview)
        tree.configure(xscrollcommand=scroll_bar_x.set)
        scroll_bar_x.pack(side=tk.BOTTOM, fill=tk.X)
        tree.pack(fill=tk.BOTH, expand=True)

        # define headings
        tree.heading('#0', anchor=tk.W, text='Property name')
        tree.heading('value', anchor=tk.W, text='Value')
        # layout
        tree.column('#0', anchor=tk.W, minwidth=200)
        tree.column('#1', anchor=tk.W, minwidth=200)

        for field in self.context.metadata_fields:
            tree.heading(field, anchor=tk.W, text=field)
            tree.column(field, anchor=tk.W, minwidth=200)

        # bind double-click to editing
        tree.bind('<Double-1>', self.handle_double_click)

        tree.bind('<Button-3>', self.handle_right_click)
        self.tree = tree

        self.refresh()

    def refresh(self):
        self.tree.delete(*self.tree.get_children())
        if self.node is not None:
            if daq.IPropertyObject.can_cast_from(self.node):
                self.fillProperties(
                    '', daq.IPropertyObject.cast_from(self.node))

    def fillList(self, parent_iid, l):
        for i, value in enumerate(l):
            self.tree.insert('' if not parent_iid else parent_iid,
                             tk.END, text=str(i), values=(str(value),))

    def fillDict(self, parent_iid, d):
        for key, value in d.items():
            self.tree.insert('' if not parent_iid else parent_iid,
                             tk.END, text=str(key), values=(str(value),))

    def fillStruct(self, parent_iid, node):
        for key, value in node.as_dictionary.items():
            self.tree.insert('' if not parent_iid else parent_iid,
                             tk.END, text=key, values=(value,))

    def fillProperties(self, parent_iid, node):
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
                    property_info.item_type, node.get_property_value(property_info.name))

            meta_fields = [None] * len(self.context.metadata_fields)

            try:
                for i, field in enumerate(self.context.metadata_fields):
                    meta_fields[i] = getattr(property_info, field)
            except Exception as e:
                print(e)

            iid = self.tree.insert('' if not parent_iid else parent_iid, tk.END, text=property_info.name, values=(
                property_value, *meta_fields))

            if property_info.value_type == daq.CoreType.ctObject:
                self.fillProperties(
                    iid, node.get_property_value(property_info.name))
            elif property_info.value_type == daq.CoreType.ctStruct:
                self.fillStruct(
                    iid, node.get_property_value(property_info.name))
            elif property_info.value_type == daq.CoreType.ctList:
                self.fillList(iid, node.get_property_value(property_info.name))
            elif property_info.value_type == daq.CoreType.ctDict:
                self.fillDict(iid, node.get_property_value(property_info.name))

    def handle_copy(self):
        selected_item = utils.treeview_get_first_selection(self.tree)
        if selected_item is None:
            return
        item = self.tree.item(selected_item)

        self.clipboard_clear()
        value = '' if len(item['values']) == 0 else item['values'][0]
        self.clipboard_append(value)

    def handle_show_metadata(self):
        selected_item = utils.treeview_get_first_selection(self.tree)
        if selected_item is None:
            return

        path = utils.get_item_path(self.tree, selected_item)
        prop = utils.get_property_for_path(self.context, path, self.node)
        if not prop:
            return

        MetadataDialog(self, prop, self.context).show()

    def nearest_property_with_path(self, path):
        prop = utils.get_property_for_path(self.context, path, self.node)
        while prop is None and len(path) > 0:
            path = path[:-1]
            prop = utils.get_property_for_path(self.context, path, self.node)
        return (prop, path)

    def handle_paste(self):
        selected_item_id = utils.treeview_get_first_selection(self.tree)
        if selected_item_id is None:
            return

        path = utils.get_item_path(self.tree, selected_item_id)
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

            self.event_port.emit()

        except Exception as e:
            utils.show_error('Paste error', f'Can\'t paste: {e}', parent=self)

    def handle_right_click(self, event):
        utils.treeview_select_item(self.tree, event)

        menu = tk.Menu(self, tearoff=0)
        menu.add_command(label='Copy', command=self.handle_copy)
        menu.add_command(label='Paste',
                         command=self.handle_paste)
        menu.add_separator()
        menu.add_command(label='Metadata', command=self.handle_show_metadata)
        menu.tk_popup(event.x_root, event.y_root)

    def handle_double_click(self, event):
        selected_item_id = utils.treeview_get_first_selection(self.tree)
        if selected_item_id is None:
            return
        property_name = self.tree.item(selected_item_id, 'text')
        path = utils.get_item_path(self.tree, selected_item_id)
        prop = utils.get_property_for_path(self.context, path, self.node)
        if not prop:
            return

        old_value = prop.value
        property_value = prop.value

        if prop.value_type == daq.CoreType.ctFunc:
            f = daq.IFunction.cast_from(property_value)
            FunctionDialog(self, prop, f, self.context).show()
            return

        if prop.value_type == daq.CoreType.ctProc:
            p = daq.IProcedure.cast_from(property_value)
            FunctionDialog(self, prop, p, self.context).show()
            return

        if prop.read_only:
            return

        prompt = 'Enter the new value for {}:'.format(property_name)

        if prop.value_type == daq.CoreType.ctBool:
            property_value = not property_value
        elif prop.value_type == daq.CoreType.ctInt:
            if prop.selection_values is not None:
                property_value = utils.show_selection(
                    prompt, property_value, prop.selection_values)
            else:
                min_value = prop.min_value
                max_value = prop.max_value
                if min_value is not None and max_value is not None:
                    property_value = simpledialog.askinteger(
                        property_name, prompt=prompt, initialvalue=property_value, minvalue=min_value.int_value,
                        maxvalue=max_value.int_value)
                else:
                    property_value = simpledialog.askinteger(
                        property_name, prompt=prompt, initialvalue=property_value)

        elif prop.value_type == daq.CoreType.ctFloat:
            min_value = prop.min_value
            max_value = prop.max_value
            if min_value is not None and max_value is not None:
                property_value = simpledialog.askfloat(
                    property_name, prompt=prompt, initialvalue=property_value, minvalue=min_value.float_value,
                    maxvalue=max_value.float_value)
            else:
                property_value = simpledialog.askfloat(
                    property_name, prompt=prompt, initialvalue=property_value)
        elif prop.value_type == daq.CoreType.ctString:
            property_value = simpledialog.askstring(
                property_name, prompt=prompt, initialvalue=property_value)
        elif prop.value_type in (daq.CoreType.ctDict, daq.CoreType.ctList):
            EditContainerPropertyDialog(
                self, prop, self.context).show()
        else:
            return

        if property_value is None:
            return

        if old_value != property_value:
            prop.value = property_value
            self.refresh()
            self.event_port.emit()
