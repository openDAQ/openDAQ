import opendaq as daq
import tkinter as tk
from tkinter import ttk

from ..utils import *
from ..app_context import *
from ..event_port import *
from .diaolog import Dialog


class AddFunctionBlockDialog(Dialog):
    def __init__(self, parent, context, node, **kwargs):
        Dialog.__init__(self, parent, 'Add device', context, **kwargs)
        self.node = node
        # send events to parent window
        self.event_port = EventPort(self.parent)
        self.parent_device = None

        self.geometry('{}x{}'.format(
            900*self.context.ui_scaling_factor, 400*self.context.ui_scaling_factor))

        # parent

        parent_device_tree_frame = ttk.Frame(self)
        parent_device_tree = ttk.Treeview(parent_device_tree_frame)

        parent_device_scroll_bar = ttk.Scrollbar(
            parent_device_tree_frame, orient="vertical", command=parent_device_tree.yview)
        parent_device_tree.configure(
            yscrollcommand=parent_device_scroll_bar.set)
        parent_device_scroll_bar.pack(side="right", fill="y")

        parent_device_tree.heading('#0', text='Parent device')

        parent_device_tree.column(
            '#0', anchor=tk.W, width=200, stretch=True)

        parent_device_tree.bind('<<TreeviewSelect>>',
                                self.handle_parent_device_selected)
        parent_device_tree.pack(fill="both", expand=True)

        parent_device_tree_frame.grid(row=0, column=0)
        parent_device_tree_frame.grid_configure(sticky='nsew')

        # child

        tree_frame = ttk.Frame(self)
        tree = ttk.Treeview(tree_frame, columns=('id', 'name'), displaycolumns=(
            'id', 'name'), show='tree headings', selectmode='browse')
        scroll_bar = ttk.Scrollbar(
            tree_frame, orient="vertical", command=tree.yview)
        tree.configure(yscrollcommand=scroll_bar.set)
        scroll_bar.pack(side="right", fill="y")

        # define headings
        tree.heading('id', text='TypeId')
        tree.heading('name', text='Name')

        # layout
        tree.column('#0', anchor=tk.CENTER, width=0,    stretch=False)
        tree.column('#1', anchor=tk.CENTER, width=300 *
                    self.context.ui_scaling_factor,  stretch=False)
        tree.column('#2', anchor=tk.CENTER, width=300 *
                    self.context.ui_scaling_factor,  stretch=True)

        # bind double-click to editing
        tree.bind('<Double-1>', self.handle_fb_tree_double_click)

        tree.pack(fill="both", expand=True)

        tree_frame.grid(row=0, column=1)
        tree_frame.grid_configure(sticky='nsew')

        self.device_tree = parent_device_tree
        self.fb_tree = tree

        self.grid_rowconfigure(0, weight=1)
        self.columnconfigure(0, weight=1)
        self.columnconfigure(1, weight=2)

        self.initial_update_func = lambda: self.update_parent_devices(
            self.device_tree, '', self.context.instance)
        self.after(1, lambda: self.device_tree.selection_set(
            self.context.instance.global_id))

    def update_parent_devices(self, tree, parent_id, component):
        tree.delete(*tree.get_children())

        def traverse_devices_recursive(tree, parent_id, component):
            if component is None:
                return

            if daq.IDevice.can_cast_from(component):
                device = daq.IDevice.cast_from(component)
                tree.insert(parent_id, tk.END, text=device.name,
                            iid=device.global_id, open=True)
                parent_id = device.global_id

            if daq.IFolder.can_cast_from(component):
                folder = daq.IFolder.cast_from(component)
                for item in folder.items:
                    traverse_devices_recursive(tree, parent_id, item)

        traverse_devices_recursive(tree, parent_id, component)

    def update_function_blocks(self):
        self.fb_tree.delete(*self.fb_tree.get_children())

        available_function_block_types = self.parent_device.available_function_block_types
        for function_block_id in available_function_block_types:
            self.fb_tree.insert('', tk.END, iid=function_block_id, values=(
                function_block_id, daq.IFunctionBlockType.cast_from(available_function_block_types[function_block_id]).name))

    def handle_parent_device_selected(self, event):
        selected_item = treeview_get_first_selection(
            self.device_tree)
        if selected_item is None:
            return

        parent_device = find_component(selected_item, self.context.instance)
        if parent_device is not None and daq.IDevice.can_cast_from(parent_device):
            parent_device = daq.IDevice.cast_from(parent_device)
            self.parent_device = parent_device
            self.update_function_blocks()

    def handle_fb_tree_double_click(self, event):
        selected_item = treeview_get_first_selection(self.fb_tree)
        if selected_item is None:
            return

        item = self.fb_tree.item(selected_item)

        function_block_id = item['values'][0]
        self.parent_device.add_function_block(function_block_id)

        self.event_port.emit()
