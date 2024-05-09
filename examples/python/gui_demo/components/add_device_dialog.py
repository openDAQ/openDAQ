import opendaq as daq
import tkinter as tk
from tkinter import ttk

from ..utils import *
from ..app_context import *
from ..event_port import *
from .diaolog import Dialog


class AddDeviceDialog(Dialog):
    def __init__(self, parent, context, node, **kwargs):
        Dialog.__init__(self, parent, 'Add device', context, **kwargs)
        self.node = node
        # send events to parent window
        self.event_port = EventPort(self.parent)

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

        # device

        right_side_frame = ttk.Frame(self)
        device_tree_frame = ttk.Frame(right_side_frame)
        device_tree = ttk.Treeview(device_tree_frame, columns=('used', 'name', 'conn'), displaycolumns=(
            'used', 'name', 'conn'), show='tree headings', selectmode='browse')

        device_scroll_bar = ttk.Scrollbar(
            device_tree_frame, orient="vertical", command=device_tree.yview)
        device_tree.configure(yscrollcommand=device_scroll_bar.set)
        device_scroll_bar.pack(side="right", fill="y")

        self.device_tree = device_tree
        self.parent_device_tree = parent_device_tree

        device_tree.heading('used', text='Used')
        device_tree.heading('name', text='Name')
        device_tree.heading('conn', text='Connection string')

        device_tree.column('#0', anchor=tk.CENTER, width=0,   stretch=False)
        device_tree.column('#1', anchor=tk.CENTER, width=50,  stretch=False)
        device_tree.column('#2', anchor=tk.CENTER, width=200, stretch=True)
        device_tree.column('#3', anchor=tk.CENTER, width=350, stretch=True)

        device_tree.bind(
            '<Double-1>', self.handle_device_tree_double_click)
        device_tree.pack(fill="both", expand=True)
        device_tree_frame.pack(fill="both", expand=True)

        add_device_frame = tk.Frame(right_side_frame)
        tk.Label(add_device_frame, text='Connection string:').pack(side=tk.LEFT)
        self.conn_string_entry = tk.Entry(add_device_frame)
        self.conn_string_entry.bind('<Return>', self.handle_entry_enter)
        self.conn_string_entry.pack(
            side=tk.LEFT, expand=True, fill=tk.X, padx=5)
        tk.Button(add_device_frame, text='Add',
                  command=self.handle_add_device).pack(side=tk.RIGHT)
        add_device_frame.pack(side=tk.BOTTOM, fill=tk.X,
                              padx=(5, 0), pady=(5, 0))

        right_side_frame.grid(row=0, column=1)
        right_side_frame.grid_configure(sticky='nsew')

        self.grid_rowconfigure(0, weight=1)
        self.columnconfigure(0, weight=1)
        self.columnconfigure(1, weight=2)

        self.dialog_parent_device = None

        self.initial_update_func = lambda: self.update_parent_devices(
            self.parent_device_tree, '', self.context.instance)
        self.after(1, lambda: self.parent_device_tree.selection_set(
            self.context.instance.global_id))

    def handle_parent_device_selected(self, event):
        selected_item = treeview_get_first_selection(
            self.parent_device_tree)
        if selected_item is None:
            return
        parent_device = find_component(selected_item, self.context.instance)
        if parent_device is not None and daq.IDevice.can_cast_from(parent_device):
            parent_device = daq.IDevice.cast_from(parent_device)
            self.dialog_parent_device = parent_device
            self.update_child_devices(
                self.device_tree, parent_device)

    def handle_device_tree_double_click(self, event):
        nearest_device = self.dialog_parent_device
        if nearest_device is None:
            return
        selected_item = treeview_get_first_selection(
            self.device_tree)
        if selected_item is None:
            return
        item = self.device_tree.item(selected_item)

        conn = item['values'][2]
        if conn not in self.context.enabled_devices:
            device = self.context.add_device(
                DeviceInfoLocal(conn), nearest_device)
            if device:
                self.update_parent_devices(
                    self.parent_device_tree, '', self.context.instance)
                self.parent_device_tree.selection_set(device.global_id)
                self.event_port.emit()

    def handle_add_device(self):
        connection_string = self.conn_string_entry.get()
        if connection_string and self.dialog_parent_device is not None:
            DeviceInfoLocal(connection_string)
            device = self.context.add_device(DeviceInfoLocal(
                connection_string), self.dialog_parent_device)
            if device:
                self.update_parent_devices(
                    self.parent_device_tree, '', self.context.instance)
                self.parent_device_tree.selection_set(device.global_id)
                self.event_port.emit()

    def handle_entry_enter(self, event):
        self.handle_add_device()

    def update_child_devices(self, tree, parent_device):
        tree.delete(*tree.get_children())

        if parent_device is None:
            return

        for device_info in parent_device.available_devices:
            name = device_info.name
            conn = device_info.connection_string
            used = conn in self.context.enabled_devices

            tree.insert('', tk.END, iid=conn, values=(
                yes_no[used], name, conn))

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
