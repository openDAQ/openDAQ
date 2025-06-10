import tkinter as tk
from tkinter import ttk
from tkinter import messagebox
from app_context import AppContext
from components.component_creator import CreateTreeElement

class MainWindow(tk.Tk):
    def __init__(self, args):
        super().__init__()
        
        self.title("Main Window")
        self.geometry("800x600")
        self.minsize(600, 400)
        self.init_styles()
        self.context = AppContext(args)

        self.init_menu_bar()
        self.init_content_area()

        self.build_tree()


    def build_tree(self):
        root_device = self.context.daq_instance.root_device
        self.root_tree_element = CreateTreeElement(self.context, self.tree, root_device)
        self.root_tree_element.init()


    def init_menu_bar(self):
        menu_bar = tk.Menu(self)
        self.config(menu=menu_bar)

        file_menu = tk.Menu(menu_bar, tearoff=0)
        menu_bar.add_cascade(label='File', menu=file_menu)
        file_menu.add_command(label='Load configuration')
        file_menu.add_command(label='Save configuration')
        file_menu.add_separator()
        file_menu.add_command(label='Exit', command=self.quit)

        view_menu = tk.Menu(menu_bar, tearoff=0)
        menu_bar.add_cascade(label='View', menu=view_menu)
        self.show_hidden_components_var = tk.BooleanVar(value=False)
        view_menu.add_checkbutton(label='Show hidden components',
                                  variable=self.show_hidden_components_var,
                                  command=self.on_show_hidden_components_change)


    def init_content_area(self):
        self.paned_window = tk.PanedWindow(self, orient=tk.HORIZONTAL, sashrelief="flat")
        self.paned_window.pack(fill="both", expand=True)

        # Sidebar with right border
        sidebar_container = tk.Frame(self.paned_window, bg=self.cget("bg"))

        self.sidebar_frame = tk.Frame(sidebar_container, bg=self.cget("bg"))
        self.sidebar_frame.pack(side="left", fill="both", expand=True)

        # Right border as a thin frame
        border = tk.Frame(sidebar_container, width=1, bg="#cccccc")  # Light grey line
        border.pack(side="right", fill="y")

        self.init_selection_menu(self.sidebar_frame)
        self.init_treeview(self.sidebar_frame)
        self.init_selection_bottom_bar(self.sidebar_frame)

        self.paned_window.add(sidebar_container, minsize=200)

        # Main content
        self.main_content_frame = tk.Frame(self.paned_window, bg="white")
        self.init_top_bar(self.main_content_frame)
        self.init_main_content(self.main_content_frame)
        self.paned_window.add(self.main_content_frame, minsize=200)


    def init_selection_menu(self, parent):
        selection_options = [
            "System Overview",
            "Signals",
            "Channels",
            "Function blocks",
            "Full Topology"
        ]
        self.selection_var = tk.StringVar(value=selection_options[0])

        selection_menu = tk.OptionMenu(parent, self.selection_var, *selection_options)
        selection_menu.bind("<ButtonRelease-1>", self.on_view_change)
        selection_menu.config(
            font=("Arial", 10),
            background= self.cget("bg"),
            foreground="black",
            relief="flat",
            highlightthickness=0,
            borderwidth=0,
            anchor="w",
            padx=10
        )
        selection_menu.pack(fill="x", padx=5, pady=(10, 10))


    def on_show_hidden_components_change(self):
        self.context.show_invisible_components = self.show_hidden_components_var.get()
        self.on_view_change(None)


    def on_view_change(self, event):
        selected_value = self.selection_var.get()
       
        components_to_show = None
        if selected_value == "System Overview":
            components_to_show = ["Device", "Folder", "Signal", "Channel", "FunctionBlock"]
        elif selected_value == "Signals":
            components_to_show = ["Signal"]
        elif selected_value == "Channels":
            components_to_show = ["Channel"]
        elif selected_value == "Function blocks":
            components_to_show = ["FunctionBlock"]
        self.context.show_component_types = components_to_show

        self.root_tree_element.show_filtered()


    def init_treeview(self, parent):
        self.tree = ttk.Treeview(parent, show="tree", style="Custom.Treeview")
        self.tree.pack(fill="both", expand=True, padx=5, pady=(0, 5))
        self.tree.bind("<<TreeviewSelect>>", self.on_tree_select)
        self.tree.bind('<ButtonRelease-3>', self.on_tree_right_click_menu)


    def init_selection_bottom_bar(self, parent):
        self.bottom_bar = tk.Frame(parent, bg=self.cget("bg"))
        self.bottom_bar.pack(side="bottom", fill="x", padx=5, pady=5)

        # Top border (1px height)
        top_border = tk.Frame(self.bottom_bar, height=1, bg="#cccccc")
        top_border.pack(side="top", fill="x")

        add_device_button = ttk.Button(self.bottom_bar, text="Add Device")
        add_device_button.pack(side="top", fill="x", pady=(5, 5))  # Add spacing below border

        add_function_block_button = ttk.Button(self.bottom_bar, text="Add Function Block")
        add_function_block_button.pack(side="top", fill="x")

        add_server_button = ttk.Button(self.bottom_bar, text="Add server")
        add_server_button.pack(side="top", fill="x")


    def init_top_bar(self, parent):
        top_frame = tk.Frame(parent, height=50, bg=self.cget("bg"))
        top_frame.pack(side="top", fill="x")

        # Bottom border (1px height)
        bottom_border = tk.Frame(parent, height=1, bg="#cccccc")
        bottom_border.pack(side="top", fill="x")

        icon = tk.Button(top_frame, text="⚙", relief="flat", background="#e0e0e0")
        icon.pack(side="right", padx=10)


    def init_main_content(self, parent):
        self.main_content = tk.Frame(parent, bg='white')
        self.main_content.pack(side="left", fill="both", expand=True)

        self.selected_label = tk.Label(self.main_content, text="Select an item from the tree", font=("Arial", 14))
        self.selected_label.pack(padx=20, pady=20, anchor="nw")


    def on_tree_select(self, event):
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
        if selected_item:
            try:
                popup = self.root_tree_element.get_child(selected_item).on_create_right_click_menu(self.main_content)
                popup.tk_popup(event.x_root, event.y_root, 0)
            except ValueError as e:
                messagebox.showerror("Error", str(e))
            finally:
                popup.unpost()


    def init_styles(self):
        style = ttk.Style()
        darkGrey = "#555555"
        # General app font and colors
        self.option_add("*Label.Font", ("Arial", 12))
        self.option_add("*Label.Foreground", darkGrey)
        self.option_add("*Label.Background", "white")
        self.option_add("*Button.Font", ("Arial", 10))
        self.option_add("*Button.Background", "#e0e0e0")

        # Treeview style
        style.configure("Custom.Treeview",
                        font=("Arial", 12),
                        background=self.cget("bg"),
                        fieldbackground=self.cget("bg"),
                        foreground=darkGrey,
                        rowheight=40,
                        borderwidth=0,
                        relief="flat")

        style.map("Custom.Treeview",
                background=[("selected", "#e5e5e5")],
                foreground=[("selected", darkGrey)])

        style.configure("Custom.Treeview.Heading",
                        font=("Arial", 12, "bold"),
                        # background="white",
                        foreground=darkGrey,
                        relief="flat",
                        borderwidth=0)

