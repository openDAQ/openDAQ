import tkinter as tk
from tkinter import ttk

class CheckboxList(ttk.Frame):
    """A scrollable checklist widget using Checkbuttons."""

    def __init__(self, master, **kwargs):
        super().__init__(master, **kwargs)

        # Create the canvas + scrollbar setup
        self.canvas = tk.Canvas(self, borderwidth=0, highlightthickness=0)
        self.frame = ttk.Frame(self.canvas)
        self.scrollbar = ttk.Scrollbar(self, orient="vertical", command=self.canvas.yview)
        self.canvas.configure(yscrollcommand=self.scrollbar.set)

        self.scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        self.canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, pady=5)
        self.canvas.create_window((0, 0), window=self.frame, anchor=tk.NW, tags="self.frame")
        self.frame.bind("<Configure>", self.on_frame_configure)

        self._vars = {}
        self.callback = lambda x: None

    def on_frame_configure(self, e):
        bbox = list(self.canvas.bbox("all"))
        offset = 4
        bbox[0] += offset
        bbox[2] += offset
        bbox[1] -= offset
        bbox[3] -= offset
        self.canvas.configure(scrollregion=tuple(bbox))

    def insert(self, label, selected):
        """Add a new checkbutton item at the end."""
        var = tk.BooleanVar(value=selected)
        chk = ttk.Checkbutton(self.frame, text=label, variable=var, command=self.handle_checking)
        chk.pack(anchor="w", fill="x", padx=4, pady=1)
        self._vars[label] = var

    def get_checked(self):
        """Return a list of labels that are checked."""
        return [label for label, var in self._vars.items() if var.get()]

    def clear(self):
        """Remove all items."""
        for widget in self.inner_frame.winfo_children():
            widget.destroy()
        self._vars.clear()

    def register_callback(self, f):
        self.callback = f

    def handle_checking(self):
        self.callback(self.get_checked())
