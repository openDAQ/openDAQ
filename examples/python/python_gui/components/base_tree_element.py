import tkinter as tk
from tkinter import ttk
from typing import Dict, Optional
from functools import lru_cache
from app_context import AppContext

class BaseTreeElement:
    def __init__(self, 
                 context: AppContext, 
                 tree: ttk.Treeview):
        self.context = context
        self.tree = tree
        self.children: Dict[str, BaseTreeElement] = {}

        self.local_id = "/"
        self.global_id = "/"
        self.name = self.local_id
        self.type = "PropertyObject"
        self.icon_name = None

    def init(self, parent: Optional['BaseTreeElement'] = None):
        self.parent = parent
        parent_tree_element = self.parent.tree_element if self.parent else ""
        if self.icon_name:
            image = self.context.icons[self.icon_name]
            self.tree_element = self.tree.insert(parent_tree_element, "end", text=self.name, open=True, iid=self.global_id, image=image)
        else:
            self.tree_element = self.tree.insert(parent_tree_element, "end", text=self.name, open=True, iid=self.global_id)

    @property
    def visible(self):
        return True

    def __set_name(self, name: str) -> None:
        self.name = name
        self.tree.item(self.tree_element, text=self.name)

    def show_filtered(self, parent_tree_element: str = "") -> None:
        visible = False
        if self.context.show_invisible_components or self.visible:
            if self.context.show_component_types is None:
                visible = True
            elif self.type in self.context.show_component_types:
                visible = True

        if visible:
            self.tree.reattach(self.global_id, parent_tree_element, "end")
            parent_tree_element = self.tree_element
        else:
            self.tree.detach(self.tree_element)

        for child in self.children.values():
            child.show_filtered(parent_tree_element)
    
    def add_child(self, child: 'BaseTreeElement') -> 'BaseTreeElement':
        self.children[child.local_id] = child
        child.init(self)
        return child

    def remove_child(self, child: 'BaseTreeElement') -> None:
        if child.local_id in self.children:
            del self.children[child.local_id]
            self.tree.delete(child.tree_element)

    @lru_cache(maxsize=128)
    def get_child(self, path: str) -> 'BaseTreeElement':
        if len(path) == 0:
            return self

        if path.startswith("/"):
            if path.startswith("/" + self.local_id):
                if len(path) == len(self.local_id) + 1:
                    return self
                path = path[len(self.local_id) + 2:]
            else:
                raise KeyError(f"No child found at path: {path}")
            
        parts = path.split("/", maxsplit = 1)
        if len(parts) == 1:
            return self.children[parts[0]]
        else:
            return self.children[parts[0]].get_child(parts[1])

    def on_selected(self, main_content: tk.Frame) -> None:
        selected_label = tk.Label(main_content, text=f"You selected: {self.global_id} ({self.type})", font=("Arial", 14))
        selected_label.pack(padx=20, pady=20, anchor="nw")

    def on_create_right_click_menu(self, main_content: tk.Frame) -> None:
        popup = tk.Menu(self.tree, tearoff=0)
        return popup