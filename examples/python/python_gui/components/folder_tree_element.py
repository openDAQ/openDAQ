from components.component_tree_element import *
from components.component_factory import create_tree_element

class FolderTreeElement(ComponentTreeElement):
    def __init__(self, 
                 context: AppContext, 
                 tree: ttk.Treeview,
                 daq_component: daq.IFolder):
        super().__init__(context, tree, daq_component)
        self.type = "Folder"
        self.icon_name = "folder"
        self.name = self.get_standard_folder_name(self.name)

    def init(self, parent: Optional['BaseTreeElement'] = None):
        super().init(parent)
        for item in self.daq_component.items:
            self.add_child(create_tree_element(self.context, self.tree, item))

    @property
    def visible(self):
        if len(self.children) == 0:
            return False
        return super().visible
    
    def get_standard_folder_name(self, component_name):
        if component_name == 'Sig':
            return 'Signals'
        if component_name == 'FB':
            return 'Function blocks'
        if component_name == 'Dev':
            return 'Devices'
        if component_name == 'IP':
            return 'Input ports'
        if component_name == 'IO':
            return 'Inputs/Outputs'
        if component_name == 'Srv':
            return 'Servers'
        return component_name

    