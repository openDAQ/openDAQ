import tkinter as tk
from tkinter import ttk
import opendaq as daq


from ..app_context import AppContext
from .properties_view import PropertiesView


class SynchronizationView(ttk.Frame):

    def __init__(self, parent, device, context: AppContext, **kwargs):
        super().__init__(parent, **kwargs)
        self.configure(relief=tk.FLAT, border=0, padding=(0, 5, 10, 0))
        self._device = daq.IDevice.cast_from(device)

        header_frame = ttk.Frame(self)
        header_frame.pack(fill=tk.X)
        ttk.Label(header_frame, image=context.icons['link']).pack(side=tk.LEFT, padx=(7, 0))
        ttk.Label(header_frame, text='Synchronization').pack(side=tk.LEFT)

        self._properties = PropertiesView(self, daq.IPropertyObject.cast_from(self._device.synchronization), context)
        self._properties.pack(fill=tk.BOTH, expand=True)

        # The synchronization is a property of the device, so its changes arrive as the device's core events
        self._component_core_event_handler = daq.QueuedEventHandler(self._on_component_core_event)
        self._device.on_component_core_event + self._component_core_event_handler
        self.bind('<Destroy>', self._on_destroy)

    def _on_destroy(self, event):
        if event.widget is not self or self._component_core_event_handler is None:
            return
        try:
            self._device.on_component_core_event - self._component_core_event_handler
        except Exception:
            pass
        self._component_core_event_handler = None

    def _on_component_core_event(self, sender, args: daq.IEventArgs):
        if args.event_name in ("PropertyValueChanged", "PropertyAdded", "PropertyRemoved"):
            self._properties.refresh()
