from components.folder_tree_element import *
from components.property_object_view import PropertyObjectView
from components.collapsible_frame import CollapsibleFrame

class DeviceTreeElement(FolderTreeElement):
    def __init__(self,
                 context: AppContext,
                 tree: ttk.Treeview,
                 daq_component: daq.IDevice):
        super().__init__(context, tree, daq_component)
        self.type = "Device"
        self.icon_name = "device"

    def on_selected(self, main_content: tk.Frame) -> None:
        """Display device properties including info section"""
        # Create accordion group - list to track all collapsible sections
        accordion_group = []

        # Properties section
        properties_section = CollapsibleFrame(main_content, "Properties", self.context, start_collapsed=False, accordion_group=accordion_group)
        properties_section.pack(fill="both", expand=False)

        self.property_view = PropertyObjectView(properties_section.content, self.context, self.daq_component)
        self.property_view.pack(fill="both", expand=True, padx=5)

        # Device Info section
        try:
            device_info = self.daq_component.info
            if device_info:
                info_section = CollapsibleFrame(main_content, "Device Info", self.context, start_collapsed=True, accordion_group=accordion_group)
                info_section.pack(fill="both", expand=False)

                self.info_view = PropertyObjectView(info_section.content, self.context, device_info, True, "DeviceInfo")
                self.info_view.pack(fill="both", expand=True, padx=5)
        except Exception as e:
            print(f"Failed to add device info: {e}")

    def add_device(self):
        ''' Add a new device to the tree '''
        pass

    def add_function_block(self):
        ''' Add a new function block to the tree '''
        pass

    def on_create_right_click_menu(self, main_content: tk.Frame) -> None:
        popup = super().on_create_right_click_menu(main_content)
        popup.add_command(label='Remove', command=lambda: self.parent.remove_child(self))
        popup.add_command(label='Add device', command=lambda: self.add_device(self))
        popup.add_command(label='Add Function block', command=lambda: self.add_function_block(self))
        return popup

    