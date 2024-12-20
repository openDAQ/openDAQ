import tkinter as tk
from tkinter import ttk

import opendaq as daq

from .. import utils
from ..app_context import AppContext
from ..event_port import EventPort
from .dialog import Dialog


class AddFunctionBlockDialog(Dialog):
    def __init__(self, parent, context, selected_component = None, **kwargs):
        Dialog.__init__(self, parent, 'Add function block', context, **kwargs)
        # send events to parent window
        self.event_port = EventPort(self.parent)
        self.parent_component = selected_component

        self.geometry('{}x{}'.format(
            900 * self.context.ui_scaling_factor, 400 * self.context.ui_scaling_factor))

        # parent

        parent_device_tree_frame = ttk.Frame(self)
        parent_device_tree = ttk.Treeview(parent_device_tree_frame)

        parent_device_scroll_bar = ttk.Scrollbar(
            parent_device_tree_frame, orient=tk.VERTICAL, command=parent_device_tree.yview)
        parent_device_tree.configure(
            yscrollcommand=parent_device_scroll_bar.set)
        parent_device_scroll_bar.pack(side=tk.RIGHT, fill=tk.Y)

        parent_device_tree.heading('#0', text='Parent device', anchor=tk.W)

        parent_device_tree.column(
            '#0', anchor=tk.W, minwidth=200, stretch=True)

        parent_device_tree.bind('<<TreeviewSelect>>',
                                self.handle_parent_device_selected)
        parent_device_tree.pack(fill=tk.BOTH, expand=True)

        parent_device_tree_frame.grid(row=0, column=0, sticky=tk.NSEW)

        # child

        tree_frame = ttk.Frame(self)
        tree = ttk.Treeview(tree_frame, columns=('id', 'name'), displaycolumns=(
            'id', 'name'), show='tree headings', selectmode=tk.BROWSE)
        scroll_bar = ttk.Scrollbar(
            tree_frame, orient=tk.VERTICAL, command=tree.yview)
        tree.configure(yscrollcommand=scroll_bar.set)
        scroll_bar.pack(side=tk.RIGHT, fill=tk.Y)

        # define headings
        tree.heading('id', text='TypeId', anchor=tk.W)
        tree.heading('name', text='Name', anchor=tk.W)

        # layout
        tree.column('#0', width=0, stretch=tk.NO)
        tree.column('id', anchor=tk.W, minwidth=200, width=300 *
                    self.context.ui_scaling_factor, stretch=tk.NO)
        tree.column('name', anchor=tk.W, minwidth=200, width=300 *
                    self.context.ui_scaling_factor)

        # bind double-click to editing
        tree.bind('<Double-1>', self.handle_fb_tree_double_click)

        tree.pack(fill=tk.BOTH, expand=True)

        tree_frame.grid(row=0, column=1, sticky=tk.NSEW)

        self.device_tree = parent_device_tree
        self.fb_tree = tree

        self.grid_rowconfigure(0, weight=1)
        self.grid_columnconfigure(0, weight=1)
        self.grid_columnconfigure(1, weight=2)
        self.grid_columnconfigure((0, 1), uniform='uniform')

        self.initial_update_func = lambda: self.initial_update()

    def initial_update(self):
        self.update_dialog()

    def update_dialog(self):
        self.update_parent_devices(
            self.device_tree, '', self.context.instance)
        parent_to_select = self.parent_component.global_id if self.parent_component is not None else self.context.instance.global_id
        self.select_parent_device(parent_to_select)

    def select_parent_device(self, device_id: str):
        if self.device_tree.exists(device_id):
            self.device_tree.selection_set(device_id)

    def update_parent_devices(self, tree, parent_id, component):
        tree.delete(*tree.get_children())

        def traverse_devices_recursive(tree, parent_id, component):
            if component is None:
                return

            if daq.IDevice.can_cast_from(component):
                device = daq.IDevice.cast_from(component)
                tree.insert(parent_id, tk.END, text=device.name,
                            iid=device.global_id, open=tk.TRUE)
                parent_id = device.global_id

            if daq.IFunctionBlock.can_cast_from(component):
                function_block = daq.IFunctionBlock.cast_from(component)
                if function_block.available_function_block_types:
                    tree.insert(parent_id, tk.END, text=function_block.name,
                                iid=function_block.global_id, open=tk.TRUE)
                    parent_id = function_block.global_id

            if daq.IFolder.can_cast_from(component):
                folder = daq.IFolder.cast_from(component)
                for item in folder.items:
                    traverse_devices_recursive(tree, parent_id, item)

        traverse_devices_recursive(tree, parent_id, component)

    def update_function_blocks(self):
        self.fb_tree.delete(*self.fb_tree.get_children())

        available_function_block_types = self.parent_component.available_function_block_types
        for function_block_id in available_function_block_types:
            self.fb_tree.insert('', tk.END, iid=function_block_id, values=(
                function_block_id,
                daq.IFunctionBlockType.cast_from(available_function_block_types[function_block_id]).name))

    def handle_parent_device_selected(self, event):
        selected_item = utils.treeview_get_first_selection(
            self.device_tree)
        if selected_item is None:
            return

        parent_component = utils.find_component(
            selected_item, self.context.instance)
        if parent_component is not None:
            if daq.IDevice.can_cast_from(parent_component) or daq.IFunctionBlock.can_cast_from(parent_component):
                self.parent_component = daq.IDevice.cast_from(parent_component) if daq.IDevice.can_cast_from(
                    parent_component) else daq.IFunctionBlock.cast_from(parent_component)
                self.update_function_blocks()

    def handle_fb_tree_double_click(self, event):
        selected_item = utils.treeview_get_first_selection(self.fb_tree)
        if selected_item is None:
            return

        item = self.fb_tree.item(selected_item)

        function_block_id = item['values'][0]
        self.parent_component.add_function_block(function_block_id)

        self.event_port.emit()
        self.update_dialog()
