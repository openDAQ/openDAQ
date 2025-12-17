import tkinter as tk
from tkinter import ttk, messagebox
import opendaq as daq

from app_context import AppContext
from components.component_factory import create_tree_element


class MainWindow(tk.Tk):
    def __init__(self, args):
        super().__init__()

        self.title("Main Window")
        self.geometry("1000x700")
        self.minsize(700, 450)

        self._hover = {"iid": None, "tags": ()}

        self.init_styles()
        self.context = AppContext(args)

        self.init_menu_bar()
        self.init_content_area()

        self.build_tree()
        self.context.set_show_invisible_components_changed_callback(self.on_view_change)

        self._tree_hover_bind(self.tree)

        # Start polling openDAQ events
        self.poll_opendaq_events()

    # -------------------- Styles --------------------

    def init_styles(self):
        import tkinter.font as tkfont

        self.colors = {
            "bg":     "#F6F7FB",
            "panel":  "#FFFFFF",
            "panel2": "#F2F4F8",
            "border": "#D9DDE7",
            "fg":     "#111827",
            "muted":  "#6B7280",
            "accent": "#2563EB",
            "select": "#DCE7FF",
            "hover":  "#EEF4FF",
            "btn":    "#FFFFFF",
            "btn_h":  "#F4F6FA",
            "btn_p":  "#E9EDF5",
            "thumb":  "#C9D1E1",   # цвет ползунка скролла
            "thumb_h":"#B8C3D8",
        }
        C = self.colors
        self.configure(bg=C["bg"])

        families = set(tkfont.families())
        for cand in ["SF Pro Text", "SF Pro Display", "Helvetica Neue", "Helvetica", "Segoe UI", "Inter", "Arial"]:
            if cand in families:
                font_family = cand
                break
        else:
            font_family = "Arial"

        self.font_base = (font_family, 12)
        self.font_small = (font_family, 11)
        self.font_bold = (font_family, 11, "bold")

        style = ttk.Style(self)
        try:
            style.theme_use("clam")  # на macOS это даёт больше контроля
        except tk.TclError:
            pass

        # Labels (на карточках)
        style.configure("TLabel", background=C["panel"], foreground=C["fg"], font=self.font_base)
        style.configure("Muted.TLabel", background=C["panel"], foreground=C["muted"], font=self.font_base)

        # Buttons (плоские)
        style.configure(
            "Modern.TButton",
            background=C["btn"],
            foreground=C["fg"],
            relief="flat",
            padding=(12, 8),
            font=self.font_small,
        )
        style.map("Modern.TButton", background=[("active", C["btn_h"]), ("pressed", C["btn_p"])])

        # Tool button (⚙) — маленькая, без серого “кирпича”
        style.configure(
            "Tool.TButton",
            background=C["panel"],
            foreground=C["muted"],
            relief="flat",
            padding=6,
            font=(font_family, 12),
        )
        style.map(
            "Tool.TButton",
            background=[("active", C["btn_h"]), ("pressed", C["btn_p"])],
            foreground=[("active", C["fg"]), ("pressed", C["fg"])],
        )

        # Combobox
        style.configure(
            "Modern.TCombobox",
            fieldbackground=C["panel2"],
            background=C["panel2"],
            foreground=C["fg"],
            arrowcolor=C["muted"],
            padding=(10, 7),
            font=self.font_small,
        )
        style.map(
            "Modern.TCombobox",
            fieldbackground=[("readonly", C["panel2"])],
            background=[("readonly", C["panel2"])],
            foreground=[("readonly", C["fg"])],
        )

        # Treeviews
        for tv in ("Sidebar.Treeview", "Props.Treeview"):
            style.configure(
                tv,
                background=C["panel"],
                fieldbackground=C["panel"],
                foreground=C["fg"],
                rowheight=28,
                font=self.font_small,
                borderwidth=0,
                relief="flat",
                padding=0,
                # Добиваем 1px рамки, которые clam рисует через border/focus colors:
                bordercolor=C["panel"],
                lightcolor=C["panel"],
                darkcolor=C["panel"],
                focuscolor=C["panel"],
            )
            style.map(
                tv,
                background=[("selected", C["select"])],
                foreground=[("selected", C["fg"])],
                bordercolor=[("focus", C["panel"]), ("!focus", C["panel"])],
                focuscolor=[("focus", C["panel"]), ("!focus", C["panel"])],
            )

            # Убираем "field" (он часто и оставляет “следы”)
            style.layout(tv, [("Treeview.treearea", {"sticky": "nswe"})])

            style.configure(
                f"{tv}.Heading",
                background=C["panel"],
                foreground=C["muted"],
                relief="flat",
                borderwidth=0,
                font=self.font_bold,
            )

        # ---- Scrollbar: тонкий и без стрелок ----
        style.layout(
            "Thin.Vertical.TScrollbar",
            [("Vertical.Scrollbar.trough", {"sticky": "ns", "children": [
                ("Vertical.Scrollbar.thumb", {"expand": "1", "sticky": "nswe"})
            ]})]
        )
        style.layout(
            "Thin.Horizontal.TScrollbar",
            [("Horizontal.Scrollbar.trough", {"sticky": "ew", "children": [
                ("Horizontal.Scrollbar.thumb", {"expand": "1", "sticky": "nswe"})
            ]})]
        )

        style.configure(
            "Thin.Vertical.TScrollbar",
            troughcolor=C["panel"],
            background=C["thumb"],
            bordercolor=C["panel"],
            lightcolor=C["panel"],
            darkcolor=C["panel"],
            troughborderwidth=0,
            borderwidth=0,
            arrowsize=0,
            width=10,
            relief="flat",
        )
        style.map("Thin.Vertical.TScrollbar", background=[("active", C["thumb_h"])])

        style.configure(
            "Thin.Horizontal.TScrollbar",
            troughcolor=C["panel"],
            background=C["thumb"],
            bordercolor=C["panel"],
            lightcolor=C["panel"],
            darkcolor=C["panel"],
            troughborderwidth=0,
            borderwidth=0,
            arrowsize=0,
            width=10,
            relief="flat",
        )
        style.map("Thin.Horizontal.TScrollbar", background=[("active", C["thumb_h"])])


    # -------------------- Menu --------------------

    def init_menu_bar(self):
        menu_bar = tk.Menu(self)
        self.config(menu=menu_bar)

        file_menu = tk.Menu(menu_bar, tearoff=0)
        menu_bar.add_cascade(label="File", menu=file_menu)
        file_menu.add_command(label="Load configuration")
        file_menu.add_command(label="Save configuration")
        file_menu.add_separator()
        file_menu.add_command(label="Exit", command=self.quit)

        view_menu = tk.Menu(menu_bar, tearoff=0)
        menu_bar.add_cascade(label="View", menu=view_menu)

        self.show_hidden_components_var = tk.BooleanVar(value=False)
        view_menu.add_checkbutton(
            label="Show hidden components",
            variable=self.show_hidden_components_var,
            command=self.on_show_hidden_components_change,
        )

    # -------------------- Layout --------------------

    def init_content_area(self):
        C = self.colors

        self.paned = tk.PanedWindow(
            self,
            orient=tk.HORIZONTAL,
            bg=C["bg"],
            bd=0,
            sashwidth=6,
            sashrelief="flat",
        )
        self.paned.pack(fill="both", expand=True)

        # ---- Sidebar card ----
        sidebar_outer = tk.Frame(self.paned, bg=C["border"])
        sidebar = tk.Frame(sidebar_outer, bg=C["panel"])
        sidebar.pack(fill="both", expand=True, padx=1, pady=1)
        self.init_sidebar(sidebar)

        # ---- Main card ----
        main_outer = tk.Frame(self.paned, bg=C["border"])
        main = tk.Frame(main_outer, bg=C["panel"])
        main.pack(fill="both", expand=True, padx=1, pady=1)
        self.init_main(main)

        self.paned.add(sidebar_outer, minsize=260)
        self.paned.add(main_outer, minsize=340)

    def init_sidebar(self, parent):
        parent.grid_columnconfigure(0, weight=1)
        parent.grid_rowconfigure(1, weight=1)

        self.init_selection_menu(parent)
        self.init_treeview(parent)

    def init_selection_menu(self, parent):
        selection_options = [
            "System Overview",
            "Signals",
            "Channels",
            "Function blocks",
            "Full Topology",
        ]
        self.selection_var = tk.StringVar(value=selection_options[0])

        combo = ttk.Combobox(
            parent,
            textvariable=self.selection_var,
            values=selection_options,
            state="readonly",
            style="Modern.TCombobox",
        )
        combo.bind("<<ComboboxSelected>>", self.on_view_change)
        combo.grid(row=0, column=0, sticky="ew", padx=12, pady=(12, 10))

    def init_treeview(self, parent):
        tree_container = tk.Frame(parent, bg=self.colors["panel"], bd=0, highlightthickness=0)
        tree_container.grid(row=1, column=0, sticky="nsew", padx=12, pady=(0, 12))
        tree_container.grid_columnconfigure(0, weight=1)
        tree_container.grid_rowconfigure(0, weight=1)

        self.tree = ttk.Treeview(tree_container, show="tree", style="Sidebar.Treeview")
        self.tree.configure(takefocus=False)  # чтобы не появлялся фокус-ободок

        ysb = ttk.Scrollbar(
            tree_container,
            orient="vertical",
            command=self.tree.yview,
            style="Thin.Vertical.TScrollbar",
        )
        self.tree.configure(yscrollcommand=ysb.set)

        self.tree.grid(row=0, column=0, sticky="nsew")
        ysb.grid(row=0, column=1, sticky="ns")

        self.tree.bind("<<TreeviewSelect>>", self.on_tree_select)
        self.tree.bind("<Button-3>", self.on_tree_right_click_menu)
        self.tree.bind("<Button-2>", self.on_tree_right_click_menu)  # macOS

    def init_main(self, parent):
        C = self.colors

        parent.grid_columnconfigure(0, weight=1)
        parent.grid_rowconfigure(2, weight=1)

        top = tk.Frame(parent, bg=C["panel"])
        top.grid(row=0, column=0, sticky="ew")
        top.grid_columnconfigure(0, weight=1)

        # маленькая tool button, без гигантской серой рамки
        ttk.Button(top, text="⚙", style="Tool.TButton").grid(row=0, column=1, padx=12, pady=10, sticky="e")

        sep = tk.Frame(parent, height=1, bg=C["border"])
        sep.grid(row=1, column=0, sticky="ew")

        self.main_content = tk.Frame(parent, bg=C["panel"])
        self.main_content.grid(row=2, column=0, sticky="nsew")

        self.selected_label = ttk.Label(self.main_content, text="Select an item from the tree", style="Muted.TLabel")
        self.selected_label.pack(padx=20, pady=20, anchor="nw")

    # -------------------- OpenDAQ Event Processing --------------------

    def poll_opendaq_events(self):
        """Periodically process openDAQ events and scheduler"""
        # Process event queue
        try:
            daq.event_queue.process_events()
        except Exception as e:
            print("Callback processing error:", e)

        # Process scheduler main loop iteration
        try:
            self.context.daq_instance.context.scheduler.run_main_loop_iteration()
        except Exception as e:
            print("Scheduler processing error:", e)

        # Re-schedule after 20 ms
        self.after(20, self.poll_opendaq_events)

    # -------------------- Behavior --------------------

    def build_tree(self):
        root_device = self.context.daq_instance.root_device
        self.root_tree_element = create_tree_element(self.context, self.tree, root_device)
        self.root_tree_element.init()
        self.root_tree_element.show_filtered()

    def on_show_hidden_components_change(self):
        self.context.set_show_invisible_components(self.show_hidden_components_var.get())

    def on_view_change(self, *_):
        selected_value = self.selection_var.get()

        components_to_show = None
        if selected_value == "System Overview":
            components_to_show = ["Device", "Folder", "Signal", "Channel", "FunctionBlock"]
        elif selected_value == "Signals":
            components_to_show = ["Device", "Signal"]
        elif selected_value == "Channels":
            components_to_show = ["Device", "Channel"]
        elif selected_value == "Function blocks":
            components_to_show = ["Device", "FunctionBlock"]
        elif selected_value == "Full Topology":
            components_to_show = None

        self.context.show_component_types = components_to_show
        self.root_tree_element.show_filtered()

    def on_tree_select(self, _event):
        for widget in self.main_content.winfo_children():
            widget.destroy()

        selected_item = self.tree.focus()
        if selected_item:
            try:
                self.root_tree_element.get_child(selected_item).on_selected(self.main_content)
            except ValueError as e:
                messagebox.showerror("Error", str(e))

    def on_tree_right_click_menu(self, event):
        selected_item = self.tree.identify_row(event.y)
        if not selected_item:
            return

        popup = None
        try:
            popup = self.root_tree_element.get_child(selected_item).on_create_right_click_menu(self.main_content)
            popup.tk_popup(event.x_root, event.y_root, 0)
        except ValueError as e:
            messagebox.showerror("Error", str(e))
        finally:
            try:
                if popup is not None:
                    popup.grab_release()
            except Exception:
                pass

    # -------------------- Hover --------------------

    def _tree_hover_bind(self, tree: ttk.Treeview):
        tree.tag_configure("hover", background=self.colors["hover"])
        tree.bind("<Motion>", self._on_tree_motion, add=True)
        tree.bind("<Leave>", self._on_tree_leave, add=True)

    def _on_tree_motion(self, event):
        tree = event.widget
        iid = tree.identify_row(event.y)

        if self._hover["iid"] == iid:
            return

        old = self._hover["iid"]
        if old:
            # old мог быть удалён во время rebuild/show_filtered
            if tree.exists(old):
                tree.item(old, tags=self._hover["tags"])
            else:
                self._hover["iid"] = None
                self._hover["tags"] = ()

        self._hover["iid"] = iid
        if not iid or not tree.exists(iid):
            self._hover["tags"] = ()
            return

        tags = tree.item(iid, "tags") or ()
        self._hover["tags"] = tags

        if iid != tree.focus() and "hover" not in tags:
            tree.item(iid, tags=(*tags, "hover"))

    def _on_tree_leave(self, _event):
        old = self._hover["iid"]
        if old and self.tree.exists(old):
            self.tree.item(old, tags=self._hover["tags"])
        self._hover["iid"] = None
        self._hover["tags"] = ()

