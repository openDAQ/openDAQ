"""Mixin for adding hover effects to Treeview widgets"""
from tkinter import ttk


class TreeHoverMixin:
    """Mixin that adds hover effects to ttk.Treeview"""

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._hover_iid = None
        self._hover_tag = "hover"

    def setup_hover(self, hover_color="#E5E7EB"):
        """Setup hover effect with given color"""
        # Configure hover tag
        self.tag_configure(self._hover_tag, background=hover_color)

        # Bind events
        self.bind("<Motion>", self._on_motion, add=True)
        self.bind("<Leave>", self._on_leave, add=True)
        self.bind("<<TreeviewSelect>>", self._on_select, add=True)

    def _add_hover(self, iid: str):
        """Add hover tag to item"""
        tags = self.item(iid, "tags") or ()
        if self._hover_tag not in tags:
            self.item(iid, tags=(*tags, self._hover_tag))

    def _remove_hover(self, iid: str):
        """Remove hover tag from item"""
        tags = self.item(iid, "tags") or ()
        if self._hover_tag in tags:
            self.item(iid, tags=tuple(t for t in tags if t != self._hover_tag))

    def _on_motion(self, event):
        """Handle mouse motion"""
        iid = self.identify_row(event.y)

        if iid == self._hover_iid:
            return

        # Remove hover from previous item
        if self._hover_iid and self.exists(self._hover_iid):
            self._remove_hover(self._hover_iid)

        self._hover_iid = iid

        if not iid or not self.exists(iid):
            self._hover_iid = None
            return

        # Don't highlight selected items
        if iid not in self.selection():
            self._add_hover(iid)

    def _on_select(self, event):
        """Handle selection change - remove hover from selected items"""
        if self._hover_iid and self.exists(self._hover_iid) and self._hover_iid in self.selection():
            self._remove_hover(self._hover_iid)

    def _on_leave(self, event):
        """Handle mouse leaving the tree"""
        if self._hover_iid and self.exists(self._hover_iid):
            self._remove_hover(self._hover_iid)
        self._hover_iid = None
