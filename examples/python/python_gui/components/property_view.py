import tkinter as tk
from tkinter import ttk
from opendaq import IPropertyObject
from app_context import AppContext

class PropertyView(ttk.Treeview):
    def __init__(self, parent: tk.Frame, context: AppContext, property_object: IPropertyObject):
        super().__init__(parent)
        self.context = context
        self.property_object = property_object

        self.create_widgets()

    def create_widgets(self):
        pass