import opendaq as daq
import tkinter as tk
from tkinter import ttk

from ..utils import *
from ..app_context import *
from ..event_port import *
from .diaolog import Dialog


class AddConfigDialog(tk.Toplevel):
    def __init__(self, parent, context, connection_string=None):
        super().__init__(parent)
        self.context = context
        self.connection_string = connection_string
        self.title("Add Config")
        self.geometry('{}x{}'.format(
            1200 * self.context.ui_scaling_factor, 600 * self.context.ui_scaling_factor))

        tk.Label(self, text=self.connection_string).pack(pady=10, padx=10, anchor=tk.NW, side=tk.TOP)

        tk.Button(self, text="add", command=self.save_config).pack(pady=10, padx=10, anchor=tk.SE, side=tk.BOTTOM)

        self.frame = tk.Frame(self)
        self.frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)

        self.tree = ttk.Treeview(self.frame, columns=('value',), show='tree headings')
        self.tree.heading('#0', text='Property')
        self.tree.heading('value', text='Value')
        self.tree.pack(fill=tk.BOTH, expand=True)

        self.device_config = context.instance.create_default_add_device_config()
        self.display_config_options('', self.device_config)

        self.grab_set()
        self.transient(parent)

    # initial display of properties
    def display_config_options(self, parent_node, context):
        def printed_value(value_type, value):
            if value_type == daq.CoreType.ctBool:
                return yes_no[value]
            else:
                return value

        for property in context.visible_properties:
            prop = context.get_property_value(property.name)
            if isinstance(prop, daq.IBaseObject) and daq.IPropertyObject.can_cast_from(prop):
                casted_property = daq.IPropertyObject.cast_from(prop)
                node_id = self.tree.insert(parent_node, 'end', text=property.name, open=True)
                self.display_config_options(node_id, casted_property)
            else:
                property_value = printed_value(property.item_type, context.get_property_value(property.name))
                self.tree.insert(parent_node, 'end', text=property.name, values=(property_value,))
                self.tree.bind('<Double-1>', self.edit_value)

    def save_value(self, entry, item_id, column, path):
        new_value = entry.get()
        self.tree.set(item_id, column, new_value)
        entry.destroy()
        print(path, new_value)
        self.update_property_value(path, new_value)

    # after pressing enter or if entry is out of focus it changes property value
    def update_property_value(self, path, new_value):
        def update_property(context, path, new_value, depth=0):
            for property in context.visible_properties:
                if property.name == path[depth]:
                    if depth == len(path) - 1:
                        context.set_property_value(property.name,
                                                   int(new_value))  # for now it only handles CoreType.ctInt type values, it also needs support for ctList, ctBool and ctString
                        return
                    prop = context.get_property_value(property.name)
                    if isinstance(prop, daq.IBaseObject) and daq.IPropertyObject.can_cast_from(prop):
                        casted_property = daq.IPropertyObject.cast_from(prop)
                        update_property(casted_property, path, new_value, depth + 1)

        update_property(self.device_config, path, new_value)

    def edit_value(self, event):
        item_id = self.tree.selection()[0]
        column = self.tree.identify_column(event.x)
        if column == '#1':
            x, y, width, height = self.tree.bbox(item_id, column)
            value = self.tree.set(item_id, column)
            entry = tk.Entry(self.tree)
            entry.place(x=x, y=y, width=width, height=height)
            entry.insert(0, value)
            entry.focus()
            path = self.get_item_path(item_id)
            entry.bind('<Return>', lambda e: self.save_value(entry, item_id, column, path))
            entry.bind('<FocusOut>', lambda e: self.save_value(entry, item_id, column, path))

    def save_config(self):
        self.print_config()
        try:
            self.context.instance.add_device(self.connection_string, self.device_config)
        except RuntimeError as e:
            print(f"{e}")
        self.destroy()

    # checker if config actually changes
    def print_config(self):
        def print_property_recursive(context):
            for property in context.visible_properties:
                prop = context.get_property_value(property.name)
                if isinstance(prop, daq.IBaseObject) and daq.IPropertyObject.can_cast_from(prop):
                    casted_property = daq.IPropertyObject.cast_from(prop)
                    print_property_recursive(casted_property)
                else:
                    print(f"{property.name}: {prop}")

        print_property_recursive(self.device_config)

    # my solution for having multiple properties with the same name
    def get_item_path(self, item_id):
        path = []
        while item_id:
            item_text = self.tree.item(item_id, 'text')
            path.insert(0, item_text)
            item_id = self.tree.parent(item_id)
        return path


class AddDeviceDialog(Dialog):
    def __init__(self, parent, context, node, **kwargs):
        Dialog.__init__(self, parent, 'Add device', context, **kwargs)
        self.node = node
        # send events to parent window
        self.event_port = EventPort(self.parent)

        self.geometry('{}x{}'.format(
            900 * self.context.ui_scaling_factor, 400 * self.context.ui_scaling_factor))

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

        device_tree.column('#0', anchor=tk.CENTER, width=0, stretch=False)
        device_tree.column('#1', anchor=tk.CENTER, width=50, stretch=False)
        device_tree.column('#2', anchor=tk.CENTER, width=200, stretch=True)
        device_tree.column('#3', anchor=tk.CENTER, width=350, stretch=True)

        device_tree.bind(
            '<Double-1>', self.handle_device_tree_double_click)
        device_tree.bind('<Button-3>', self.handle_right_click)
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
        tk.Button(add_device_frame, text='Add Config',
                  command=self.handle_add_config).pack(side=tk.RIGHT)  # New button
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

    def handle_add_config(self):
        if not self.device_tree.selection() is None:
            selected_item = treeview_get_first_selection(
                self.device_tree)
            if selected_item is None:
                return
            item = self.device_tree.item(selected_item)
            connection_string = item['values'][2]
        else:
            connection_string = self.conn_string_entry.get()
        add_config_dialog = AddConfigDialog(self, self.context, connection_string)
        self.wait_window(add_config_dialog)

    def handle_right_click(self, event):
        menu = tk.Menu(self, tearoff=0)
        menu.add_command(label="Copy", command=self.handle_add_config)
        menu.tk_popup(event.x_root, event.y_root)

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
