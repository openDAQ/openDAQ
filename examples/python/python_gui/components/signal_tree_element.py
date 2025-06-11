from components.component_tree_element import *


class SignalTreeElement(ComponentTreeElement):
    def __init__(self, 
                 context: AppContext, 
                 tree: ttk.Treeview,
                 daq_component: daq.ISignal):
        super().__init__(context, tree, daq_component)
        self.type = "Signal"
        self.icon_name = "signal"

    