import tkinter as tk
from tkinter import ttk
from typing import Dict, Optional

from app_context import AppContext
import opendaq

from components.component_tree_element import ComponentTreeElement
from components.device_tree_element import DeviceTreeElement


def CreateTreeElement(context: AppContext, 
                      tree: ttk.Treeview, 
                      daq_component: opendaq.IComponent):
    
    if opendaq.IDevice.can_cast_from(daq_component):
        device = opendaq.IDevice.cast_from(daq_component)
        return DeviceTreeElement(context, tree, device)
    
    return ComponentTreeElement(context, tree, daq_component)
    