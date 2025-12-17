from components.folder_tree_element import *

class FunctionBlockTreeElement(FolderTreeElement):
    def __init__(self,
                 context: AppContext,
                 tree: ttk.Treeview,
                 daq_component: daq.IFunctionBlock):
        super().__init__(context, tree, daq_component)
        self.type = "FunctionBlock"
        self.icon_name = "function_block"

    def on_selected(self, main_content: tk.Frame) -> None:
        """Display function block with horizontal split: properties on left, empty on right"""
        # Get colors
        C = getattr(self.context, "colors", {})
        bg_color = C.get("panel", "#FFFFFF")

        # Create horizontal PanedWindow
        paned = tk.PanedWindow(main_content, orient=tk.HORIZONTAL, bg=bg_color, sashwidth=5, sashrelief=tk.RAISED)
        paned.pack(fill="both", expand=True)

        # Left panel - Properties (use parent's on_selected)
        left_panel = tk.Frame(paned, bg=bg_color)
        paned.add(left_panel, minsize=200)
        super().on_selected(left_panel)

        # Right panel - Empty for now
        right_panel = tk.Frame(paned, bg=bg_color)
        paned.add(right_panel, minsize=200)
        super().on_selected(right_panel)

    def add_function_block(self):
        ''' Add a new function block to the tree '''
        pass

    def on_create_right_click_menu(self, main_content: tk.Frame) -> None:
        popup = super().on_create_right_click_menu(main_content)
        popup.add_command(label='Remove', command=lambda: self.parent.remove_child(self))
        popup.add_command(label='Add Function block', command=lambda: self.add_function_block(self))
        return popup

    