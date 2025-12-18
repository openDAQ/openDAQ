"""Treeview with built-in hover effects"""
from tkinter import ttk
from .tree_hover_mixin import TreeHoverMixin


class HoverableTreeview(TreeHoverMixin, ttk.Treeview):
    """Treeview with hover effects already configured"""

    def __init__(self, parent, hover_color="#EEF4FF", **kwargs):
        super().__init__(parent, **kwargs)
        self.setup_hover(hover_color)
