from components.component_tree_element import *

class InputPortTreeElement(ComponentTreeElement):
    def __init__(self, 
                 context: AppContext, 
                 tree: ttk.Treeview,
                 daq_component: daq.IInputPort):
        super().__init__(context, tree, daq_component)
        self.type = "InputPort"
        self.icon_name = "input_port"

    