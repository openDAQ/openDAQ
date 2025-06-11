from components.folder_tree_element import *

class FunctionBlockTreeElement(FolderTreeElement):
    def __init__(self, 
                 context: AppContext, 
                 tree: ttk.Treeview,
                 daq_component: daq.IFunctionBlock):
        super().__init__(context, tree, daq_component)
        self.type = "FunctionBlock"
        self.icon_name = "function_block"

    