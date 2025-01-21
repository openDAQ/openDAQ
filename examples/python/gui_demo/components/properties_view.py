import tkinter as tk
from tkinter import ttk

from ..app_context import AppContext
from .metadata_fields_selector_dialog import MetadataFieldsSelectorDialog
from .generic_properties_treeview import PropertiesTreeview

class PropertiesView(ttk.Frame):
    def __init__(self, parent: ttk.Frame, node=None, context: AppContext = None, **kwargs):
        ttk.Frame.__init__(self, parent, **kwargs)
        self.context = context
        self.configure(padding=(10, 5))

        header_frame = ttk.Frame(self)

        ttk.Label(header_frame, text='Properties').pack(side=tk.LEFT, pady=5)
        tk.Button(header_frame, text='Edit', image=self.context.icons['settings'], borderwidth=0,
                  command=lambda: MetadataFieldsSelectorDialog(
                      self, self.context).show()
                  ).pack(side=tk.RIGHT, anchor=tk.E)

        header_frame.pack(fill=tk.X)

        PropertiesTreeview(self, node, context)
