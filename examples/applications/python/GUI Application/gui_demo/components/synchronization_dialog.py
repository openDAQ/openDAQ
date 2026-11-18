import tkinter as tk
from tkinter import ttk
import opendaq as daq


from ..app_context import AppContext
from .dialog import Dialog
from .generic_properties_treeview import PropertiesTreeview


class SynchronizationDialog(Dialog):

    def __init__(self, parent, parent_node: daq.IComponent, node, context: AppContext, **kwargs):
        super().__init__(parent, 'Synchronization', context, **kwargs)
        self.context = context
        self._parent_node = parent_node
        self._component_core_event_handler = daq.QueuedEventHandler(self._on_component_core_event)
        self._parent_node.on_component_core_event + self._component_core_event_handler
        self.geometry(f'{int(600 * context.dpi_factor)}x{int(800 * context.dpi_factor)}')

        ttk.Label(self, text='Properties').pack(anchor=tk.W, pady=5)

        self._view = PropertiesTreeview(self, node, context)

        self.bind('<Destroy>', self._on_destroy)

    def _on_destroy(self, event):
        if self._component_core_event_handler is not None:
            try:
                self._parent_node.on_component_core_event - self._component_core_event_handler
            except Exception:
                pass
            self._component_core_event_handler = None

    def _on_component_core_event(self, sender, args: daq.IEventArgs):
        if args.event_name in ("PropertyValueChanged", "PropertyAdded", "PropertyRemoved"):
            self._view.refresh()

