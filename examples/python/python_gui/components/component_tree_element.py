import tkinter as tk
from tkinter import ttk
from typing import Dict, Optional

from components.base_tree_element import BaseTreeElement
from app_context import AppContext

import opendaq as daq


class ComponentTreeElement(BaseTreeElement):
    def __init__(self, 
                 context: AppContext, 
                 tree: ttk.Treeview,
                 daq_component: daq.IComponent):
        super().__init__(context, tree)
        self.daq_component = daq_component

        self.local_id = self.daq_component.local_id
        self.global_id = self.daq_component.global_id
        self.name = self.daq_component.name

        self.type = "Component"

    def init(self):
        super().init()
        self.daq_component.on_component_core_event + daq.EventHandler(self.on_core_event)

    @property
    def visible(self):
        return self.daq_component.visible

    def on_core_event(self, sender: Optional[daq.IComponent], args: daq.IEventArgs):
        if args.event_name == "AttributeChanged":
            core_args = daq.ICoreEventArgs.cast_from(args)
            attribute_name = core_args.parameters["AttributeName"]
            attribute_value = core_args.parameters[attribute_name]
            self.on_changed_attribute(attribute_name, attribute_value)

    def on_changed_attribute(self, name, value):
        if name == "Name":
            self.set_name(value)




    