import tkinter as tk
from tkinter import ttk
from typing import Dict, Optional

from components.component_tree_element import ComponentTreeElement
from app_context import AppContext

from opendaq import IDevice


class DeviceTreeElement(ComponentTreeElement):
    def __init__(self, 
                 context: AppContext, 
                 tree: ttk.Treeview,
                 device: IDevice):
        super().__init__(context, tree, device)
        self.type = "Device"
        self.icon_name = "device"

    