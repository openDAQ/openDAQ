from components.folder_tree_element import *


class ChannelTreeElement(FolderTreeElement):
    def __init__(self, 
                 context: AppContext, 
                 tree: ttk.Treeview,
                 daq_component: daq.IChannel):
        super().__init__(context, tree, daq_component)
        self.type = "Channel"
        self.icon_name = "channel"

    