import tkinter as tk
from tkinter import ttk
from opendaq import IPropertyObject, CoreType
from app_context import AppContext

class PropertyView(ttk.Treeview):
    def __init__(self, parent: tk.Frame, context: AppContext, property_object: IPropertyObject):
        super().__init__(parent, columns=("value",), show='tree headings')
        self.context = context
        self.property_object = property_object

        self.heading('#0', text='Property')
        self.heading('value', text='Value')

        self.tag_configure('readonly', foreground='gray')
        # self.bind("<Double-1>", self.on_double_click)

        self.build_tree('', self.property_object)

    def build_tree(self, parent_id, property_obj: IPropertyObject):
        for prop in property_obj.all_properties:
            name = prop.name
            value = prop.value
            is_read_only = prop.read_only
            type = prop.value_type

            tag = 'readonly' if is_read_only else 'editable'

            display_value = self.format_value(name, value, type)

            item_id = self.insert(parent_id, 'end', text=name, values=(display_value,), tags=(tag,))

            if type == CoreType.ctObject:
                nested_obj = IPropertyObject.cast_from(value)
                self.build_tree(item_id, nested_obj)

    def format_value(self, name, value, type):
        if type == CoreType.ctBool:
            return "True" if value else "False"
        if type == CoreType.ctObject:
            return "IPropertyObject {" + name + "}"
        if type == CoreType.ctDict:
            return "Dictionary"
        if type == CoreType.ctList:
            return "List"
        if type == CoreType.ctFunc or type == CoreType.ctFunc:
            return "Function"
        return str(value)
