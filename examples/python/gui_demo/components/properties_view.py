import tkinter as tk
from tkinter import ttk, simpledialog

import opendaq as daq

from .. import utils
from ..event_port import EventPort
from ..app_context import AppContext
from .function_dialog import FunctionDialog
from .edit_container_property import EditContainerPropertyDialog


class PropertiesView(ttk.Frame):
    def __init__(self, parent: ttk.Frame, node=None, context: AppContext = None, **kwargs):
        ttk.Frame.__init__(self, parent, **kwargs)
        self.parent = parent
        self.node = node
        self.context = context
        self.event_port = EventPort(self)

        self.configure(padding=(10, 5))

        ttk.Label(self, text='Properties').pack(anchor=tk.W)
        tree = ttk.Treeview(self, columns=('value'), show='tree headings')

        scroll_bar = ttk.Scrollbar(self, orient=tk.VERTICAL, command=tree.yview)
        tree.configure(yscrollcommand=scroll_bar.set)
        scroll_bar.pack(side=tk.RIGHT, fill=tk.Y)
        tree.pack(fill=tk.BOTH, expand=True)

        # define headings
        tree.heading('#0', anchor=tk.W, text='Property name')
        tree.heading('value', anchor=tk.W, text='Value')
        # layout
        tree.column('#0', anchor=tk.W)
        tree.column('#1', anchor=tk.W)

        # bind double-click to editing
        tree.bind('<Double-1>', self.handle_double_click)

        tree.bind('<Button-3>', self.handle_right_click)
        self.tree = tree
        self.nodes_by_iids = {}

        self.refresh()

    def refresh(self):
        self.tree.delete(*self.tree.get_children())
        if self.node is not None:
            if daq.IPropertyObject.can_cast_from(self.node):
                self.fillProperties(
                    '', daq.IPropertyObject.cast_from(self.node))

    def fillStruct(self, parent_iid, node):
        for key, value in node.as_dictionary.items():
            iid = key if parent_iid is None else parent_iid + '.' + key
            self.nodes_by_iids[iid] = node
            self.tree.insert('' if not parent_iid else parent_iid,
                             tk.END, iid=iid, text=key, values=(value,))

    def fillProperties(self, parent_iid, node):
        def printed_value(value_type, value):
            if value_type == daq.CoreType.ctBool:
                return utils.yes_no[value]
            else:
                return value

        for property_info in self.context.properties_of_component(node):
            iid = property_info.name if parent_iid is None else parent_iid + '.' + property_info.name
            self.nodes_by_iids[iid] = node

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

            self.tree.insert('' if not parent_iid else parent_iid, tk.END, iid=iid, text=property_info.name, values=(
                property_value,))

            if property_info.value_type == daq.CoreType.ctObject:
                self.fillProperties(
                    iid, node.get_property_value(property_info.name))
            elif property_info.value_type == daq.CoreType.ctStruct:
                self.fillStruct(
                    iid, node.get_property_value(property_info.name))

    def handle_copy(self):
        selected_item = utils.treeview_get_first_selection(self.tree)
        if selected_item is None:
            return
        item = self.tree.item(selected_item)
        property_name = item['text']
        node = self.nodes_by_iids.get(selected_item)
        node = daq.IPropertyObject.cast_from(node)
        if not node:
            return
        property_value = node.get_property_value(property_name)
        self.clipboard_clear()
        self.clipboard_append(property_value)

    def handle_paste(self):
        selected_item = utils.treeview_get_first_selection(self.tree)

        if selected_item is None:
            return
        item = self.tree.item(selected_item)
        property_name = item['text']

        node = self.nodes_by_iids.get(selected_item)

        node = daq.IPropertyObject.cast_from(node)
        if not node:
            return
        property_info = node.get_property(property_name)
        if not property_info:
            return
        if property_info.read_only:
            return
        property_value = self.clipboard_get()
        if property_value is None:
            return
        node.set_property_value(property_name, property_value)
        self.refresh()
        self.event_port.emit()

    def handle_right_click(self, event):
        menu = tk.Menu(self, tearoff=0)
        menu.add_command(label='Copy', command=self.handle_copy)
        menu.add_command(label='Paste',
                         command=self.handle_paste)
        menu.tk_popup(event.x_root, event.y_root)

    def handle_double_click(self, event):
        selected_item = utils.treeview_get_first_selection(self.tree)
        if selected_item is None:
            return
        item = self.tree.item(selected_item)
        property_name = item['text']
        node = self.nodes_by_iids.get(selected_item)
        if not node:
            return
        if not daq.IPropertyObject.can_cast_from(node):
            return
        node = daq.IPropertyObject.cast_from(node)
        if not node:
            return

        property_info = node.get_property(property_name)
        property_value = node.get_property_value(property_name)
        old_value = property_value
        if not property_info:
            return

        if property_info.value_type == daq.CoreType.ctFunc:
            f = daq.IFunction.cast_from(property_value)
            FunctionDialog(self, property_info, f, self.context).show()
            return

        if property_info.value_type == daq.CoreType.ctProc:
            p = daq.IProcedure.cast_from(property_value)
            FunctionDialog(self, property_info, p, self.context).show()
            return

        if (property_info is None or property_info.read_only):
            return

        prompt = 'Enter the new value for {}:'.format(property_name)

        if property_info.value_type == daq.CoreType.ctBool:
            property_value = not property_value
        elif property_info.value_type == daq.CoreType.ctInt:
            if property_info.selection_values is not None:
                property_value = utils.show_selection(
                    prompt, property_value, property_info.selection_values)
            else:
                min_value = property_info.min_value
                max_value = property_info.max_value
                if min_value is not None and max_value is not None:
                    property_value = simpledialog.askinteger(
                        property_name, prompt=prompt, initialvalue=property_value, minvalue=min_value.int_value,
                        maxvalue=max_value.int_value)
                else:
                    property_value = simpledialog.askinteger(
                        property_name, prompt=prompt, initialvalue=property_value)

        elif property_info.value_type == daq.CoreType.ctFloat:
            min_value = property_info.min_value
            max_value = property_info.max_value
            if min_value is not None and max_value is not None:
                property_value = simpledialog.askfloat(
                    property_name, prompt=prompt, initialvalue=property_value, minvalue=min_value.float_value,
                    maxvalue=max_value.float_value)
            else:
                property_value = simpledialog.askfloat(
                    property_name, prompt=prompt, initialvalue=property_value)
        elif property_info.value_type == daq.CoreType.ctString:
            property_value = simpledialog.askstring(
                property_name, prompt=prompt, initialvalue=property_value)
        elif property_info.value_type in (daq.CoreType.ctDict, daq.CoreType.ctList):
            EditContainerPropertyDialog(self, property_info, self.context).show()
        else:
            return

        if property_value is None:
            return

        if old_value != property_value:
            node.set_property_value(property_name, property_value)
            self.refresh()
            self.event_port.emit()
