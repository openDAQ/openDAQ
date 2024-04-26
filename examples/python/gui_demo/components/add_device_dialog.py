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

        device_tree_frame = ttk.Frame(self)
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

        device_tree_frame.grid(row=0, column=1)
        device_tree_frame.grid_configure(sticky='nsew')

        self.grid_rowconfigure(0, weight=1)
        self.columnconfigure(0, weight=1)
        self.columnconfigure(1, weight=2)

        self.initial_update_func = lambda: self.update_parent_devices(
            self.parent_device_tree, '', self.context.instance)

    def handle_parent_device_selected(self, event):
        selected_item = treeview_get_first_selection(
            self.parent_device_tree)
        if selected_item is None:
            return
        parent_device = find_component(selected_item, self.context.instance)
        if parent_device is not None and daq.IDevice.can_cast_from(parent_device):
            parent_device = daq.IDevice.cast_from(parent_device)
            self.dialog_parent_device = parent_device
            self.context.scan_devices(parent_device)
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
        if not conn in self.context.all_devices[nearest_device.global_id]:
            print("Something is wrong, device not found")
            return

        device_state_conn_mapped = self.context.all_devices[nearest_device.global_id][conn]
        will_be_enabled = not device_state_conn_mapped['enabled']
        # will_be_enabled
        if will_be_enabled:
            try:
                device = nearest_device.add_device(conn)

                device_state_conn_mapped['device'] = device
                device_state_conn_mapped['enabled'] = True

                device_info = device_state_conn_mapped['device_info']
                if isinstance(device_info, DeviceInfoLocal):
                    device_info.name = device.local_id
                    device_info.serial_number = device.info.serial_number

                self.context.all_devices[nearest_device.global_id][device.global_id] = device_state_conn_mapped
                self.context.connected_devices[nearest_device.global_id][device.global_id] = device_state_conn_mapped

            except RuntimeError as e:
                print(f'Error adding device: {e}')
                device_state_conn_mapped['device'] = None
                device_state_conn_mapped['enabled'] = False
        self.update_parent_devices(
            self.parent_device_tree, '', self.context.instance)
        self.parent_device_tree.selection_set(nearest_device.global_id)
        self.event_port.emit()

    def update_child_devices(self, tree, parent_device: daq.IDevice):
        tree.delete(*tree.get_children())

        for conn in self.context.all_devices[parent_device.global_id]:
            # not displaying dups of already connected devices
            if conn in self.context.connected_devices[parent_device.global_id]:
                continue

            device_info = self.context.all_devices[parent_device.global_id][conn]['device_info']
            name = device_info.name
            used = self.context.all_devices[parent_device.global_id][conn]['enabled']
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
