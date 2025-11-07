import tkinter as tk
from tkinter import ttk

import opendaq as daq

from .. import utils
from ..app_context import AppContext, DeviceInfoLocal
from ..event_port import *
from .dialog import Dialog
from .add_config_dialog import AddConfigDialog
from .device_info_dialog import DeviceInfoDialog


class AddDeviceDialog(Dialog):
    def __init__(self, parent, context: AppContext, node, **kwargs):
        Dialog.__init__(self, parent, 'Add device', context, **kwargs)
        self.node = node
        # send events to parent window
        self.event_port = EventPort(self.parent)

        self.geometry('{}x{}'.format(
            1000 * self.context.ui_scaling_factor, 400 * self.context.ui_scaling_factor))

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

        parent_device_tree_frame.grid(row=0, column=0)
        parent_device_tree_frame.grid_configure(sticky=tk.NSEW)

        # device

        right_side_frame = ttk.Frame(self)
        device_tree_frame = ttk.Frame(right_side_frame)
        device_tree = ttk.Treeview(device_tree_frame, columns=('name', 'conn'), displaycolumns=(
            'name', 'conn'), show='tree headings', selectmode=tk.BROWSE)

        device_scroll_bar = ttk.Scrollbar(
            device_tree_frame, orient=tk.VERTICAL, command=device_tree.yview)
        device_tree.configure(yscrollcommand=device_scroll_bar.set)
        device_scroll_bar.pack(side=tk.RIGHT, fill=tk.Y)

        self.device_tree = device_tree
        self.parent_device_tree = parent_device_tree

        device_tree.heading('name', text='Name', anchor=tk.W)
        device_tree.heading('conn', text='Connection string', anchor=tk.W)

        device_tree.column('#0', width=0, stretch=False)
        device_tree.column('name', anchor=tk.W, minwidth=200)
        device_tree.column('conn', anchor=tk.W, minwidth=300)

        device_tree.bind('<Double-1>', self.handle_device_tree_double_click)
        device_tree.bind('<Button-3>', self.handle_right_click)
        device_tree.bind('<<TreeviewSelect>>', self.handle_device_selected)
        device_tree.pack(fill=tk.BOTH, expand=True)
        device_tree_frame.pack(fill=tk.BOTH, expand=True)

        add_device_frame = ttk.Frame(right_side_frame)
        ttk.Label(add_device_frame, text='Connection string:').pack(side=tk.LEFT)
        self.conn_string_entry = ttk.Entry(add_device_frame)
        self.conn_string_entry.bind('<Return>', self.handle_entry_enter)
        self.conn_string_entry.pack(side=tk.LEFT, expand=True, fill=tk.X, padx=5)

        ttk.Button(add_device_frame, text='Quick add',
                   command=self.handle_add_device).pack(side=tk.RIGHT)

        add_device_frame.pack(side=tk.BOTTOM, fill=tk.X,
                              padx=(5, 0), pady=(5, 0))

        right_side_frame.grid(row=0, column=1)
        right_side_frame.grid_configure(sticky=tk.NSEW)

        self.grid_rowconfigure(0, weight=1)
        self.columnconfigure(0, weight=1, uniform='column')
        self.columnconfigure(1, weight=2, uniform='column')

        self.dialog_parent_device = None

    def initial_update(self):
        self.update_parent_devices(
            self.parent_device_tree, '', self.context.instance)
        self.select_parent_device(
            self.context.instance.global_id)
        self.conn_string_entry.focus_set()

    def select_parent_device(self, device_id: str):
        if self.parent_device_tree.exists(device_id):
            self.parent_device_tree.selection_set(device_id)

    def handle_parent_device_selected(self, event):
        selected_item = utils.treeview_get_first_selection(
            self.parent_device_tree)
        if selected_item is None:
            return
        parent_device = utils.find_component(
            selected_item, self.context.instance)
        if parent_device is not None and daq.IDevice.can_cast_from(parent_device):
            parent_device = daq.IDevice.cast_from(parent_device)
            self.dialog_parent_device = parent_device
            self.update_child_devices(
                self.device_tree, parent_device)

    def handle_device_selected(self, event):
        selected_item_iid = utils.treeview_get_first_selection(
            self.device_tree)
        if selected_item_iid is None:
            return

        connection_string = self.device_tree.item(selected_item_iid, 'values')
        self.conn_string_entry.delete(0, tk.END)
        self.conn_string_entry.insert(0, connection_string[1])

    def handle_device_tree_double_click(self, event):
        nearest_device = self.dialog_parent_device
        if nearest_device is None:
            return
        selected_item_iid = utils.treeview_get_first_selection(
            self.device_tree)

        selected_device_info = self.find_available_device(self.dialog_parent_device, selected_item_iid)
        if selected_device_info is None or not daq.IDeviceInfo.can_cast_from(selected_device_info):
            return

        config = None
        add_config_dialog = AddConfigDialog(
            self, self.context, selected_device_info, self.dialog_parent_device)
        add_config_dialog.show()
        config = add_config_dialog.device_config
        if config is None:
            return

        self.add_device(selected_device_info.connection_string, config)

    def handle_right_click(self, event):
        utils.treeview_select_item(self.device_tree, event)

        menu = tk.Menu(self, tearoff=0)
        menu.add_command(
            label='Add with config', command=lambda: self.handle_device_tree_double_click(None))
        menu.add_command(label='Device Info',
                         command=self.handle_show_device_info)
        menu.tk_popup(event.x_root, event.y_root)

    def add_device(self, connection_string, config):
        if connection_string and self.dialog_parent_device is not None:
            try:
                device = self.context.add_device(DeviceInfoLocal(
                    connection_string), self.dialog_parent_device, config)

                self.update_parent_devices(
                    self.parent_device_tree, '', self.context.instance)
                self.select_parent_device(device.global_id)
                self.event_port.emit()
            except Exception as e:
                utils.show_error('Error adding device', f'{connection_string}: {str(e)}', self)
                return

    def handle_add_device(self):
        connection_string = self.conn_string_entry.get()
        self.add_device(connection_string, None)

    def handle_entry_enter(self, event):
        self.handle_add_device()

    def find_available_device(self, parent_device: daq.IDevice, selected_item_iid):
        if parent_device is None or selected_item_iid is None:
            return None

        found_devices = list(filter(
            lambda d: d.connection_string == selected_item_iid, parent_device.available_devices))
        return found_devices[0] if len(found_devices) > 0 else None

    def handle_show_device_info(self):
        selected_item_iid = utils.treeview_get_first_selection(
            self.device_tree)
        if selected_item_iid is None:
            return

        device_info = self.find_available_device(self.dialog_parent_device, selected_item_iid)
        if device_info is None or not daq.IDeviceInfo.can_cast_from(device_info):
            return

        DeviceInfoDialog(self, device_info, self.context).show()

    def update_child_devices(self, tree, parent_device):
        tree.delete(*tree.get_children())

        if parent_device is None:
            return

        try:
            available_devices = parent_device.available_devices
        except:
            return

        for device_info in available_devices:
            device_info = daq.IDeviceInfo.cast_from(device_info)
            name = device_info.name
            conn = device_info.connection_string

            tree.insert('', tk.END, iid=conn, values=(
                name, conn))

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
