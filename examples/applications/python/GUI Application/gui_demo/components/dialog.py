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
        # before the window is mapped (which on Windows only happens once the
        # event loop runs) winfo_width/height report 1; use the requested
        # size for the centering math instead
        if w <= 1 or h <= 1:
            w = self.winfo_reqwidth()
            h = self.winfo_reqheight()

        main_window = self.parent.winfo_toplevel()
        main_w = main_window.winfo_width()
        main_h = main_window.winfo_height()
        if main_w <= 1 or main_h <= 1:
            # main window not mapped yet (startup dialog): center on screen
            x = (self.winfo_screenwidth() - w) // 2
            y = (self.winfo_screenheight() - h) // 2
        else:
            x = main_window.winfo_rootx() + main_w // 2 - w // 2
            y = main_window.winfo_rooty() + main_h // 2 - h // 2

        # position only -- never resize here, so an explicit geometry set by
        # the dialog is not clobbered while the window is still unmapped
        self.geometry(f'+{max(0, x)}+{max(0, y)}')

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

    # non-modal variant of show(): the main window stays interactive while
    # the dialog is open
    def show_floating(self):
        self.initial_update()
        self.deiconify()
        self.center_window()
        self.update_idletasks()
        self.event_generate('<<DialogReady>>')
        self.focus_set()
        
    def close(self):
        self.destroy()
