import tkinter as tk
from tkinter import ttk

from ..app_context import AppContext


class Dialog(tk.Toplevel):
    def __init__(self, parent, title, context: AppContext, **kwargs):
        tk.Toplevel.__init__(self, parent, **kwargs)
        self.title(title)
        self.parent = parent
        self.context = context

        self.configure(padx=10, pady=5)
        self.attributes('-topmost', True)
        self.transient(parent)
        
        # Bind Escape key to close the dialog
        self.bind("<Escape>", lambda event: self.close())

    def center_window(self):
        main_window = self.parent.winfo_toplevel()
        x = main_window.winfo_rootx() + main_window.winfo_width() // 2 - \
            self.winfo_width() // 2
        y = main_window.winfo_rooty() + main_window.winfo_height() // 2 - \
            self.winfo_height() // 2
        self.geometry(f'+{x}+{y}')
        
    def initial_update(self):
        pass

    def show(self):
        self.wait_visibility()
        self.center_window()
        self.initial_update()
        self.grab_set()         # Prevent interaction with other windows
        self.focus_set()        # Ensure focus is on this dialog
        self.wait_window(self)
        
    def close(self):
        self.destroy()
