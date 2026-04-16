import tkinter as tk
from tkinter import ttk

from ..app_context import AppContext


class Dialog(tk.Toplevel):
    def __init__(self, parent, title, context: AppContext, **kwargs):
        tk.Toplevel.__init__(self, parent, **kwargs)
        self.withdraw()  # keep hidden until positioned to avoid top-left flash
        self.title(title)
        self.parent = parent
        self.context = context

        self.configure(padx=10, pady=5)
        self.attributes('-topmost', False)
        self.transient(parent)
        
        # Bind Escape key to close the dialog
        self.bind("<Escape>", lambda event: self.close())

    def center_window(self):
        self.update_idletasks()  # ensure geometry is fully computed while hidden
        w = self.winfo_width()
        h = self.winfo_height()
        main_window = self.parent.winfo_toplevel()
        x = main_window.winfo_rootx() + main_window.winfo_width() // 2 - w // 2
        y = main_window.winfo_rooty() + main_window.winfo_height() // 2 - h // 2
        self.geometry(f'{w}x{h}+{x}+{y}')

    def initial_update(self):
        pass

    def show(self):
        self.initial_update()   # populate content while dialog is still hidden
        self.deiconify()        # appear fully populated in one step
        self.center_window()    # geometry is fully known now, position before showing
        self.update_idletasks()
        self.event_generate('<<DialogReady>>')
        self.focus_set()
        self.grab_set()         # Prevent interaction with other windows
        self.wait_window(self)
        
    def close(self):
        self.destroy()
