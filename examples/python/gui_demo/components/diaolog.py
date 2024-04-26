import tkinter as tk


class Dialog(tk.Toplevel):
    def __init__(self, parent, title, context, **kwargs):
        tk.Toplevel.__init__(self, parent, **kwargs)
        self.title(title)
        self.parent = parent
        self.context = context
        self.initial_update_func = None

        self.configure(padx=10, pady=5)
        self.wm_attributes("-topmost", True)

    def center_window(self):
        x = self.parent.winfo_rootx() + self.parent.winfo_width() // 2 - \
            self.winfo_width() // 2
        y = self.parent.winfo_rooty() + self.parent.winfo_height() // 2 - \
            self.winfo_height() // 2
        self.geometry(f'+{x}+{y}')

    def show(self):
        self.wait_visibility()
        self.center_window()
        if self.initial_update_func is not None:
            self.initial_update_func()
        self.grab_set()
        self.wait_window(self)
