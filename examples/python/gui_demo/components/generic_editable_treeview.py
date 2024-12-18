import tkinter as tk
from tkinter import ttk


class GenericEditableTreeview(ttk.Treeview):
    def __init__(self, master, columns, content, to_change):
        ttk.Treeview.__init__(self, master, columns=columns[1:],  show='tree headings')

        # register function for changing values
        self.to_change = to_change

        # generate headings
        for index, column in enumerate(columns):
            self.heading('#' + str(index), anchor=tk.W, text=column)

        self.render(content, '')
        self.bind('<Double-1>', self.edit)

    def render(self, content, parent):
        for part in content:
            if isinstance(part[1], list):
                # category
                new_parent = self.insert(parent, tk.END, text=part[0], open=True)
                self.render(part[1:], new_parent)
            else:
                # node
                self.insert(parent, tk.END, text=part[0], values=part[1:])

    def edit(self, event):
        self.set(self.selection()[0], self.to_change, 'test''')
