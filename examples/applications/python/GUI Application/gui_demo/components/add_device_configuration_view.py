import tkinter as tk
from tkinter import ttk
from .generic_properties_treeview import PropertiesTreeview

class AddDeviceConfigView(ttk.Frame):
    def __init__(self, parent, **kwargs):
        ttk.Frame.__init__(self, parent, **kwargs)
        self.title = ""
        self.config = None
        self.context = None

        # Create empty defaults
        self.label = ttk.Label(self, text="", font=("Arial", 10, "bold"))
        self.label.pack(side=tk.TOP, fill=tk.X, anchor=tk.W, pady=(7, 7))

        self.editor = tk.Listbox(self)
        self.editor.pack(side=tk.TOP, fill=tk.BOTH, expand=True)

    def edit(self, node, context):
        self.title = node.name
        self.config = node.value
        self.context = context

        self.update_title()
        self.update_editor()

    def clear(self):
        self.title = ""
        self.config = None
        self.context = None

        self.update_title()
        self.update_editor()

    def update_title(self):
        self.label.configure(text=self.title)

    def update_editor(self):
        self.editor.pack_forget()
        self.editor.destroy()

        if self.config is None:
            self.editor = tk.Listbox(self)
        else:
            self.editor = PropertiesTreeview(self, self.config, self.context)

        self.editor.pack(side=tk.TOP, fill=tk.BOTH, expand=True)
