from tkinter import ttk

from app_context import AppContext
import opendaq

from components.component_tree_element import ComponentTreeElement

def create_tree_element(context: AppContext, 
                       tree: ttk.Treeview, 
                       daq_component: opendaq.IComponent) -> ComponentTreeElement:
    """Factory function to create appropriate tree element based on component type."""
    if opendaq.IDevice.can_cast_from(daq_component):
        from components.device_tree_element import DeviceTreeElement
        component = opendaq.IDevice.cast_from(daq_component)
        return DeviceTreeElement(context, tree, component)
    
    if opendaq.IChannel.can_cast_from(daq_component):
        from components.channel_tree_element import ChannelTreeElement
        component = opendaq.IChannel.cast_from(daq_component)
        return ChannelTreeElement(context, tree, component)
    
    if opendaq.IFunctionBlock.can_cast_from(daq_component):
        from components.function_block_tree_element import FunctionBlockTreeElement
        component = opendaq.IFunctionBlock.cast_from(daq_component)
        return FunctionBlockTreeElement(context, tree, component)
    
    if opendaq.IFolder.can_cast_from(daq_component):
        from components.folder_tree_element import FolderTreeElement
        component = opendaq.IFolder.cast_from(daq_component)
        return FolderTreeElement(context, tree, component)
    
    if opendaq.ISignal.can_cast_from(daq_component):
        from components.signal_tree_element import SignalTreeElement
        component = opendaq.ISignal.cast_from(daq_component)
        return SignalTreeElement(context, tree, component)
    
    if opendaq.IInputPort.can_cast_from(daq_component):
        from components.input_port_tree_element import InputPortTreeElement
        component = opendaq.IInputPort.cast_from(daq_component)
        return InputPortTreeElement(context, tree, component)
    
    return ComponentTreeElement(context, tree, daq_component) 