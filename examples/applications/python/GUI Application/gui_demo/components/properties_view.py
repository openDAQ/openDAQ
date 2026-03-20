from tkinter import ttk

from ..app_context import AppContext
from .generic_properties_treeview import PropertiesTreeview

class PropertiesView(ttk.Frame):
    def __init__(self, parent: ttk.Frame, node=None, context: AppContext = None, **kwargs):
        ttk.Frame.__init__(self, parent, **kwargs)
        self.context = context
        self.configure(padding=(10, 5))

        PropertiesTreeview(self, node, context)
