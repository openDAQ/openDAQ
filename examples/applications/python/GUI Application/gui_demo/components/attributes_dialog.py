import tkinter as tk
from tkinter import ttk

import opendaq as daq

from ..app_context import AppContext
from .dialog import Dialog
from .generic_attributes_treeview import AttributesTreeview
from .generic_properties_treeview import PropertiesTreeview
from .data_descriptor_treeview import DataDescriptorTreeview

class AttributesDialog(Dialog):
    def __init__(self, parent, title, node, context: AppContext, **kwargs):
        Dialog.__init__(self, parent, title, context, **kwargs)

        self.geometry(f'{int(600 * context.dpi_factor)}x{int(800 * context.dpi_factor)}')
        tree_frame = ttk.Frame(self)
        tree_frame.pack(fill=tk.BOTH, expand=True)

        AttributesTreeview(parent, tree_frame, node, context)

        if daq.ISignal.can_cast_from(node) or daq.IDevice.can_cast_from(node):
            additional_tree_frame = ttk.Frame(self)
            additional_tree_frame.pack(fill=tk.BOTH, expand=True)
            if daq.ISignal.can_cast_from(node):
                notebook = ttk.Notebook(additional_tree_frame)
                notebook.pack(fill=tk.BOTH, expand=True)

                signal = daq.ISignal.cast_from(node)

                signal_desc_frame = ttk.Frame(notebook)
                signal_desc_frame.pack(fill=tk.BOTH, expand=True)
                notebook.add(signal_desc_frame, text='Signal Descriptor')
                DataDescriptorTreeview(signal_desc_frame, signal.descriptor, context)

                if signal.domain_signal is not None:
                    signal_domain_desc_frame = ttk.Frame(notebook)
                    signal_domain_desc_frame.pack(fill=tk.BOTH, expand=True)
                    notebook.add(signal_domain_desc_frame, text='Domain Signal Descriptor')
                    DataDescriptorTreeview(signal_domain_desc_frame, signal.domain_signal.descriptor, context)


            elif daq.IDevice.can_cast_from(node):
                device = daq.IDevice.cast_from(node)

                notebook = ttk.Notebook(additional_tree_frame)
                notebook.pack(fill=tk.BOTH, expand=True)

                info_frame = ttk.Frame(notebook)
                info_frame.pack(fill=tk.BOTH, expand=True)
                notebook.add(info_frame, text='Device Info')
                PropertiesTreeview(info_frame, device.info, context)

                domain_frame = ttk.Frame(notebook)
                domain_frame.pack(fill=tk.BOTH, expand=True)
                notebook.add(domain_frame, text='Device Domain')

                try:
                    device_domain = device.domain
                except RuntimeError:
                    device_domain = None

                if device_domain is not None:
                    DataDescriptorTreeview(domain_frame, device_domain, context)
                else:
                    ttk.Label(domain_frame,
                              text='No device domain available',
                              padding=(10, 10)).pack(anchor=tk.W)