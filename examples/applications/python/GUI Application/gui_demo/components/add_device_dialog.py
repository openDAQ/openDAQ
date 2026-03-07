import tkinter as tk
from tkinter import ttk
import threading

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
            int(1000 * self.context.ui_scaling_factor * self.context.dpi_factor),
            int(400 * self.context.ui_scaling_factor * self.context.dpi_factor)))

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
            '#0', anchor=tk.W, minwidth=int(200 * self.context.dpi_factor), stretch=True)

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
        device_tree.column('name', anchor=tk.W, minwidth=int(200 * self.context.dpi_factor))
        device_tree.column('conn', anchor=tk.W, minwidth=int(300 * self.context.dpi_factor))

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

        def show_add_menu():
            menu = tk.Menu(add_device_frame, tearoff=0)
            menu.add_command(label='Add with config',
                             command=lambda: self.process_add_device(True))
            menu.tk_popup(add_dropdown_btn.winfo_rootx(),
                          add_dropdown_btn.winfo_rooty() + add_dropdown_btn.winfo_height())

        add_dropdown_btn = ttk.Button(add_device_frame, text='▼', width=3,
                                      command=show_add_menu)
        add_dropdown_btn.pack(side=tk.RIGHT, padx=(0, 0))
        ttk.Button(add_device_frame, text='Add',
                   command=self.handle_add_clicked).pack(side=tk.RIGHT, padx=(0, 2))

        add_device_frame.pack(side=tk.BOTTOM, fill=tk.X,
                              padx=(5, 0), pady=(5, 0))

        right_side_frame.grid(row=0, column=1)
        right_side_frame.grid_configure(sticky=tk.NSEW)

        self.grid_rowconfigure(0, weight=1)
        self.columnconfigure(0, weight=1, uniform='column')
        self.columnconfigure(1, weight=2, uniform='column')

        self.dialog_parent_device = None
        self._search_generation = 0

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

        # Fill the textbox with connection string from the device
        connection_string = self.device_tree.item(selected_item_iid, 'values')
        self.conn_string_entry.delete(0, tk.END)
        self.conn_string_entry.insert(0, connection_string[1])

    def handle_device_tree_double_click(self, event):
        self.process_add_device(False)

    def process_add_device(self, open_config_dialog: bool):
        parent_top = self.parent.winfo_toplevel()
        self.configure(cursor='watch')
        parent_top.configure(cursor='watch')
        # Defer work so Tkinter finishes the current event (repaint cursor) before blocking
        self.after(0, lambda: self._run_add_device(open_config_dialog, parent_top))

    def _run_add_device(self, open_config_dialog: bool, parent_top):
        if open_config_dialog:
            self.add_device_with_config()
        else:
            self.add_device_without_config()
        # Restore cursor if dialog is still open (error or config cancelled)
        try:
            self.configure(cursor='')
        except tk.TclError:
            pass
        try:
            parent_top.configure(cursor='')
        except tk.TclError:
            pass

    def add_device_with_config(self):
        parent_top = self.parent.winfo_toplevel()
        nearest_device = self.dialog_parent_device
        if nearest_device is None:
            utils.show_error("Configuration error", "Parent device is not selected. Cannot open config.", self)
            return

        # Device selection is handled by the connection string entry. It is updated whenever a tree-item is selected,
        # but the user may modify the connection string.
        conn_string = self.conn_string_entry.get()

        implied_protocol = self.get_implied_protocol(conn_string)
        if implied_protocol == "":
            utils.show_error(
                "Error", "Invalid connection string.", self)
            return

        add_config_dialog = None

        # Try to match entered connection string to one of the discovered devices.
        # find_available_device triggers a network call — hourglass is already set by process_add_device.
        selected_device_info = self.find_available_device(self.dialog_parent_device, conn_string)

        # Restore cursor before showing the interactive config dialog
        self.configure(cursor='')
        parent_top.configure(cursor='')

        if selected_device_info is not None and daq.IDeviceInfo.can_cast_from(selected_device_info):
            add_config_dialog = AddConfigDialog.for_discoverable(
                self, self.context, self.dialog_parent_device, selected_device_info)
        else:
            if implied_protocol == "SmartConnectionString":
                utils.show_error(
                    "Error", "Invalid connection string: 'daq://' not allowed for non-discoverable devices.", self)
                return
            add_config_dialog = AddConfigDialog.from_connection_string(
                self, self.context, self.dialog_parent_device, conn_string, implied_protocol)

        add_config_dialog.show()

        config = add_config_dialog.config
        conn_string = add_config_dialog.connection_string
        if config is None or conn_string is None:
            # Configuration was cancelled
            return

        self.add_device(conn_string, config)

    def add_device_without_config(self):
        conn_string = self.conn_string_entry.get()
        self.add_device(conn_string, None)

    def handle_right_click(self, event):
        utils.treeview_select_item(self.device_tree, event)

        menu = tk.Menu(self, tearoff=0)
        menu.add_command(label='Add',
                         command=lambda: self.process_add_device(open_config_dialog=False))
        menu.add_command(label='Add with config',
                         command=lambda: self.process_add_device(open_config_dialog=True))
        menu.add_separator()
        menu.add_command(label='Device Info',
                         command=self.handle_show_device_info)
        menu.tk_popup(event.x_root, event.y_root)

    def add_device(self, connection_string, config):
        if not connection_string or self.dialog_parent_device is None:
            return
        parent_top = self.parent.winfo_toplevel()
        self.configure(cursor='watch')
        parent_top.configure(cursor='watch')
        self.update_idletasks()
        try:
            self.context.add_device(DeviceInfoLocal(
                connection_string), self.dialog_parent_device, config)
        except Exception as e:
            self.configure(cursor='')
            parent_top.configure(cursor='')
            utils.show_error('Error adding device', f'{connection_string}: {str(e)}', self)
            return
        parent_top.configure(cursor='')
        self.event_port.emit()
        self.close()

    def handle_add_clicked(self):
        self.process_add_device(False)

    def handle_entry_enter(self, event):
        self.process_add_device(False)

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
        self._search_generation += 1
        generation = self._search_generation

        tree.delete(*tree.get_children())

        if parent_device is None:
            return

        tree.insert('', tk.END, iid='__loading__',
                    values=('Searching for devices…', ''))
        tree.configure(cursor='watch')

        def fetch():
            try:
                devices = [
                    (daq.IDeviceInfo.cast_from(d).name,
                     daq.IDeviceInfo.cast_from(d).connection_string)
                    for d in parent_device.available_devices
                ]
            except Exception:
                devices = []
            self.after(0, lambda: self._on_devices_loaded(tree, devices, generation))

        threading.Thread(target=fetch, daemon=True).start()

    def _on_devices_loaded(self, tree, devices, generation):
        if generation != self._search_generation:
            return  # a newer search has started; discard stale results
        try:
            tree.delete(*tree.get_children())
            tree.configure(cursor='')
            for name, conn in devices:
                tree.insert('', tk.END, iid=conn, values=(name, conn))
        except tk.TclError:
            pass  # dialog was closed before results arrived

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

    def get_implied_protocol(self, connection_string: str):
        sep = "://"
        if sep not in connection_string:
            return ""
        parts = connection_string.split(sep)
        if len(parts) != 2:
            return ""

        prefix = parts[0]
        if prefix == "daq":
            return "SmartConnectionString"

        # Find protocol ID matching the connection string prefix
        for protocol_id, device_type in self.dialog_parent_device.available_device_types.items():
            dt_dict = device_type.as_dictionary

            if dt_dict["Prefix"] == prefix:
                # Prefix is valid
                return protocol_id

        return ""
