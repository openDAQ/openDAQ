from components.component_tree_element import *


class DeviceTreeElement(ComponentTreeElement):
    def __init__(self, 
                 context: AppContext, 
                 tree: ttk.Treeview,
                 device: daq.IDevice):
        super().__init__(context, tree, device)
        self.type = "Device"
        self.icon_name = "device"

    def init(self, parent: Optional['BaseTreeElement'] = None):
        super().init(parent)
        for device in self.daq_component.devices:
            self.add_child(DeviceTreeElement(self.context, self.tree, device))

    