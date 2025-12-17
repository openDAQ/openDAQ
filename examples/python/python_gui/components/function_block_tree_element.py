from components.folder_tree_element import *

class FunctionBlockTreeElement(FolderTreeElement):
    def __init__(self, 
                 context: AppContext, 
                 tree: ttk.Treeview,
                 daq_component: daq.IFunctionBlock):
        super().__init__(context, tree, daq_component)
        self.type = "FunctionBlock"
        self.icon_name = "function_block"

    def add_function_block(self):
        ''' Add a new function block to the tree '''
        pass

    def on_create_right_click_menu(self, main_content: tk.Frame) -> None:
        popup = super().on_create_right_click_menu(main_content)
        popup.add_command(label='Remove', command=lambda: self.parent.remove_child(self))
        popup.add_command(label='Add Function block', command=lambda: self.add_function_block(self))
        return popup

    