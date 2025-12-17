import tkinter as tk
from tkinter import ttk


class CollapsibleFrame(tk.Frame):
    """A frame with a header that can be clicked to collapse/expand content"""

    def __init__(self, parent, title: str, context=None, start_collapsed: bool = False, accordion_group=None):
        super().__init__(parent, bg=getattr(context, "colors", {}).get("panel", "#FFFFFF"))
        self.context = context
        self.is_collapsed = start_collapsed
        self.accordion_group = accordion_group

        # Register with accordion group if provided
        if accordion_group is not None:
            accordion_group.append(self)
            # Schedule initial expand state setup for accordion mode
            self.after_idle(self._set_accordion_expand_state)

        # Get colors
        C = getattr(context, "colors", {})
        bg_color = C.get("panel", "#FFFFFF")
        border_color = C.get("border", "#D9DDE7")
        fg_color = C.get("fg", "#111827")
        hover_color = C.get("btn_h", "#F4F6FA")

        # Top separator
        top_separator = tk.Frame(self, height=1, bg=border_color)
        top_separator.pack(fill="x", pady=(0, 0))

        # Header frame
        self.header = tk.Frame(self, bg=bg_color, cursor="hand2")
        self.header.pack(fill="x", padx=0, pady=0)

        # Arrow label (▶ when collapsed, ▼ when expanded)
        self.arrow_label = tk.Label(
            self.header,
            text="▼" if not start_collapsed else "▶",
            bg=bg_color,
            fg=fg_color,
            font=("Arial", 9),
            cursor="hand2"
        )
        self.arrow_label.pack(side="left", padx=(5, 2))

        # Title label
        self.title_label = tk.Label(
            self.header,
            text=title,
            bg=bg_color,
            fg=fg_color,
            font=("Arial", 10, "bold"),
            cursor="hand2"
        )
        self.title_label.pack(side="left", padx=(2, 5), pady=3)

        # Bottom separator
        bottom_separator = tk.Frame(self, height=1, bg=border_color)
        bottom_separator.pack(fill="x", pady=(0, 0))

        # Content frame
        self.content = tk.Frame(self, bg=bg_color)
        if not start_collapsed:
            self.content.pack(fill="both", expand=True, pady=(5, 0))

        # Bind click events
        self.header.bind("<Button-1>", self.toggle)
        self.arrow_label.bind("<Button-1>", self.toggle)
        self.title_label.bind("<Button-1>", self.toggle)

        # Hover effect
        self.header.bind("<Enter>", lambda e: self._set_header_bg(hover_color))
        self.header.bind("<Leave>", lambda e: self._set_header_bg(bg_color))

    def _set_accordion_expand_state(self):
        """Set initial expand state for accordion mode"""
        if self.accordion_group is not None:
            if not self.is_collapsed:
                self.pack_configure(expand=True)
            else:
                self.pack_configure(expand=False)

    def _set_header_bg(self, color):
        """Set background color for header and its children"""
        self.header.configure(bg=color)
        self.arrow_label.configure(bg=color)
        self.title_label.configure(bg=color)

    def toggle(self, event=None):
        """Toggle between collapsed and expanded states"""
        if self.is_collapsed:
            self.expand()
        else:
            self.collapse()

    def _on_expand(self):
        """Called when this frame is expanded. Collapse others in accordion group."""
        if self.accordion_group is not None:
            for frame in self.accordion_group:
                if frame is not self and not frame.is_collapsed:
                    frame.collapse()

    def collapse(self):
        """Collapse the content"""
        if not self.is_collapsed:
            self.content.pack_forget()
            self.arrow_label.configure(text="▶")
            self.is_collapsed = True
            # In accordion mode, collapsed frames don't expand
            if self.accordion_group is not None:
                self.pack_configure(expand=False)

    def expand(self):
        """Expand the content"""
        if self.is_collapsed:
            # Collapse others in accordion group first
            self._on_expand()

            self.content.pack(fill="both", expand=True)
            self.arrow_label.configure(text="▼")
            self.is_collapsed = False
            # In accordion mode, expanded frames take all available space
            if self.accordion_group is not None:
                self.pack_configure(expand=True)
