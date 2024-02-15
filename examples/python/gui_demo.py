#!/usr/bin/env python

import argparse
import tkinter as tk
from tkinter import ttk, simpledialog
from tkinter.filedialog import asksaveasfile, askopenfile
import tkinter.font as tkfont
from functools import cmp_to_key
import os

import opendaq as daq

try:
    from ctypes import windll
    windll.shcore.SetProcessDpiAwareness(1)
except:
    pass

yes_no = {
    False: 'No',
    True:  'Yes',
}

yes_no_inv = {
    'No':  False,
    'Yes': True,
}


class DeviceInfoLocal:
    def __init__(self, conn_string):
        self.name = conn_string
        self.connection_string = conn_string
        self.serial_number = 'no-serial-number'


def show_modal(window):
    window.wait_visibility()
    window.grab_set()
    window.wait_window(window)


def show_selection(title, current_value, values: daq.IList):
    global result
    result = current_value
    top = tk.Toplevel()
    top.resizable(False, False)

    tk.Label(top, text=title).pack()

    def select_value(v):
        global result
        top.destroy()
        result = v

    def make_closure(v):
        return lambda: select_value(v)

    def fill_buttons(idx, value):
        if current_value == idx:
            sel_text = "* "
        else:
            sel_text = ""
        button = tk.Button(top, text=sel_text + value,
                           command=make_closure(idx))
        button.pack(expand=True, fill=tk.BOTH)

    if daq.IDict.can_cast_from(values):
        for idx, value in daq.IDict.cast_from(values).items():
            fill_buttons(idx, value)
    else:
        i = 0
        for value in daq.IList.cast_from(values):
            fill_buttons(i, value)
            i = i + 1

    # center window on screen
    ww = top.winfo_reqwidth()
    wh = top.winfo_reqheight()
    pr = int(top.winfo_screenwidth() / 2 - ww / 2)
    pd = int(top.winfo_screenheight() / 2 - wh / 2)
    top.geometry("+{}+{}".format(pr, pd))

    show_modal(top)
    return result


class App(tk.Tk):

    # SECTION - INIT

    def __init__(self, args):
        super().__init__()

        self.nodes = {}
        self.selected_node = None

        self.ui_scaling_factor = int(args.scale)
        self.include_reference_devices = bool(args.demo)
        try:
            if args.connection_string != '':
                self.connection_string = args.connection_string
            else:
                self.connection_string = None
        except ValueError:
            self.connection_string = None

        self.title('openDAQ demo')
        self.geometry('{}x{}'.format(
            1400*self.ui_scaling_factor, 1000*self.ui_scaling_factor))

        main_frame_top = tk.Frame(self)
        main_frame_top.pack(fill=tk.constants.X)
        main_frame_top.configure(background='white')

        search_button = tk.Button(
            main_frame_top, text='Add device', command=self.handle_add_device_button_clicked)
        search_button.pack(side=tk.constants.LEFT, padx=5,
                           pady=5, fill=tk.constants.X)

        add_function_block_button = tk.Button(
            main_frame_top, text='Add function block', command=self.handle_add_function_block_button_clicked)
        add_function_block_button.pack(
            side=tk.constants.LEFT, padx=5, pady=5, fill=tk.constants.X)

        save_config_button = tk.Button(
            main_frame_top, text='Save configuration', command=self.handle_save_config_button_clicked)
        save_config_button.pack(side=tk.constants.LEFT,
                                padx=5, pady=5, fill=tk.constants.X)

        load_config_button = tk.Button(
            main_frame_top, text='Load configuration', command=self.handle_load_config_button_clicked)
        load_config_button.pack(side=tk.constants.LEFT,
                                padx=5, pady=5, fill=tk.constants.X)

        refresh_button = tk.Button(
            main_frame_top, text='Refresh', command=self.handle_refresh_button_clicked)
        refresh_button.pack(side=tk.constants.LEFT,
                            padx=5, pady=5, fill=tk.constants.X)
        
        self.remove_button = tk.Button(
            main_frame_top, text='Remove', command=self.handle_remove_button_clicked, state='disabled')
        self.remove_button.pack(side=tk.constants.RIGHT,
                            padx=5, pady=5, fill=tk.constants.X)
        
        self.connect_button = tk.Button(
            main_frame_top, text='Connect', command=self.handle_connect_button_clicked, state='disabled')
        self.connect_button.pack(side=tk.constants.RIGHT,
                            padx=5, pady=5, fill=tk.constants.X)

        main_frame_bottom = tk.Frame(self)
        main_frame_bottom.pack(fill=tk.constants.BOTH, expand=True)

        main_frame_navigator = tk.PanedWindow(
            main_frame_bottom, orient=tk.constants.HORIZONTAL)
        main_frame_navigator.pack_propagate(0)

        frame_navigator_for_properties = tk.PanedWindow(
            main_frame_navigator, orient=tk.constants.VERTICAL)
        frame_navigator_for_properties.pack_propagate(0)

        self.tree_widget_create(main_frame_navigator)

        main_frame_navigator.add(frame_navigator_for_properties)
        self.properties_widget_create(frame_navigator_for_properties)
        self.attributes_widget_create(frame_navigator_for_properties)

        main_frame_navigator.pack(side=tk.LEFT, expand=1, fill=tk.BOTH)

        # High DPI workaround for now
        ttk.Style().configure('Treeview', rowheight=40*self.ui_scaling_factor)

        default_font = tkfont.nametofont("TkDefaultFont")
        default_font.configure(size=9*self.ui_scaling_factor)

        self.init_opendaq()

    def init_opendaq(self):
        # init device
        instance = daq.Instance()

        self.instance = instance

        self.all_devices = {}

        self.all_devices[self.instance.global_id] = dict()

        # TODO: remove this
        obj = daq.PropertyObject()
        obj.add_property(daq.IntPropertyBuilder(
            daq.String('test'), daq.Integer(1)).build())
        self.instance.add_property(
            daq.ObjectProperty(daq.String('object'), obj))

        #add the first device if connection string is provided once on start
        if self.connection_string != None:
            self.add_first_available_device()  # also calls self.update_tree_widget()

        self.tree_update()

    def scan_devices(self, parent_device=None):
        parent_device = parent_device if parent_device else self.instance

        def add_device(device_info):
            conn = device_info.connection_string
            # ignore reference devices unless explicitly requested
            if not self.include_reference_devices and 'daqref' in device_info.connection_string:
                return
            # only add device to the list if it isn't there already
            if not conn in self.all_devices[parent_device.global_id]:
                self.all_devices[parent_device.global_id][conn] = {
                    'device_info': device_info, 'enabled': False, 'device': None}

        for device_info in parent_device.available_devices:
            add_device(device_info)

    def add_first_available_device(self):
        device_info = DeviceInfoLocal(self.connection_string)

        device_state = { 'device_info': device_info, 'enabled': False, 'device': None }

        self.all_devices[self.instance.global_id][device_info.connection_string] = device_state

        try:
            device = self.instance.add_device(device_info.connection_string)
            if device:
                device_info.name = device.local_id
                device_info.serial_number = device.info.serial_number
                device_state['device'] = device
                device_state['enabled'] = True

                # multiple keys for the same device's state
                self.all_devices[self.instance.global_id][device_info.connection_string] = device_state
                self.all_devices[self.instance.global_id][device.global_id] = device_state
        except RuntimeError as e:
            print(f'Error adding device {device_info.connection_string}: {e}')
        self.tree_update()

    #!SECTION - INIT
    # SECTION - Tree view

    def tree_widget_create(self, parent_frame):
        frame = ttk.Frame(parent_frame)

        # define columns
        tree = ttk.Treeview(frame, columns=('name', 'hash'), displaycolumns=(
            'name'), show='tree', selectmode='browse')
        tree.pack(fill="both", expand=True, side="left")

        # layout
        tree.column('#0', width=350*self.ui_scaling_factor)
        # hide the column with unique id
        tree.column('#1', width=0, minwidth=0, stretch=False)
        # tree.grid(row=0, column=0, rowspan=2, sticky=tk.NSEW)

        # bind selection
        tree.bind('<<TreeviewSelect>>', self.handle_tree_select)
        tree.bind("<ButtonRelease-3>", self.handle_tree_right_button_release)
        tree.bind("<Button-3>", self.handle_tree_right_button)

        # add a scrollbar
        scroll_bar = ttk.Scrollbar(
            frame, orient=tk.VERTICAL, command=tree.yview)
        tree.configure(yscroll=scroll_bar.set)
        scroll_bar.pack(fill="y", side="right")

        parent_frame.add(frame)
        self.tree = tree

        popup = tk.Menu(tree, tearoff=0)
        popup.add_command(label="Remove")
        # popup.add_command(label="Rename")
        popup.add_command(label="Connect")
        self.tree_popup = popup

    def tree_update(self, new_selected_node=None):
        # make sure the tree is empty
        self.tree.delete(*self.tree.get_children())

        self.selected_node = new_selected_node

        self.tree_traverse_components_recursive(self.instance)
        self.selected_node = self.tree_restore_selection(
            self.selected_node)  # reset in case the selected node outdates
        self.properties_update()
        self.attributes_update()
        self.tree_update_conext_buttons_state()

    def tree_traverse_components_recursive(self, component):
        if component is None:
            return
        
        folder = daq.IFolder.cast_from(component) if component and daq.IFolder.can_cast_from(component) else None
        
        #add item to tree if it is not a folder or non-empty folder
        if folder is None or folder.items:
            self.tree_add_component(
                '' if component.parent is None else component.parent.global_id, component)
            
        if folder is not None:
            for item in folder.items:
                self.tree_traverse_components_recursive(item)

    def tree_add_component(self, parent_node_id, component):
        component_node_id = component.global_id
        component_name = component.name

        if daq.IDevice.can_cast_from(component):
            device = daq.IDevice.cast_from(component)
            component_name = device.info.name
        elif daq.IFunctionBlock.can_cast_from(component):
            function_block = daq.IFunctionBlock.cast_from(component)
            component_name = function_block.function_block_type.name
        elif daq.IFolder.can_cast_from(component):
            if component_name == 'Sig':
                component_name = 'Signals'
            elif component_name == 'FB':
                component_name = 'Function blocks'
            elif component_name == 'Dev':
                component_name = 'Devices'
            elif component_name == 'IP':
                component_name = 'Input ports'
            elif component_name == 'IO':
                component_name = 'Inputs/Outputs'

        self.tree.insert(parent_node_id, tk.END, iid=component_node_id,
                         text=component_name, open=True, values=(component_node_id))
        self.nodes[component_node_id] = component

    def tree_restore_selection(self, old_node=None):
        iid = '' if old_node is None else old_node.global_id
        node = self.find_component(iid)

        # don't drop the focus if root node is selected
        if isinstance(old_node, daq.IInstance):
            node = old_node

        if node and self.tree.exists(node.global_id):
            self.tree.selection_set(iid)
            self.tree.focus(iid)
        else:
            node = None
        return node
    
    def tree_update_conext_buttons_state(self):
        node = self.selected_node
        node_is_function_block = node is not None and daq.IFunctionBlock.can_cast_from(node)
        node_is_device = node is not None and daq.IDevice.can_cast_from(node)
        node_is_input_port = node is not None and daq.IInputPort.can_cast_from(node)

        self.connect_button.config(state='normal' if node_is_input_port else 'disabled')
        self.remove_button.config(state='normal' if node_is_function_block or node_is_device and node != self.instance else 'disabled')

    def tree_get_nearest_parent_in_tree(self, parent):
        while parent is not None:
            if self.tree.exists(parent.global_id):
                return parent
            parent = parent.parent
        return None

    #!SECTION - Tree view
    # SECTION - Properties view

    def properties_widget_create(self, parent_frame):
        frame = tk.Frame()
        # define columns
        tree = ttk.Treeview(frame, columns=(
            'value', 'unit', 'access'), show='tree headings')

        scroll_bar = ttk.Scrollbar(
            frame, orient="vertical", command=tree.yview)
        tree.configure(yscrollcommand=scroll_bar.set)
        scroll_bar.pack(side="right", fill="y")
        tree.pack(fill="both", expand=True)

        # define headings
        tree.heading('#0', text='Property')
        tree.heading('value', text='Value')
        tree.heading('unit', text='Unit')
        tree.heading('access', text='Access')
        # layout
        tree.column('#0', anchor=tk.CENTER)
        tree.column('#1', anchor=tk.CENTER)
        tree.column('#2', anchor=tk.CENTER, width=100, stretch=False)
        tree.column('#3', anchor=tk.CENTER, width=150, stretch=False)
        style = ttk.Style()
        style.configure("Treeview.Heading", font='Arial 10 bold')
        # bind double-click to editing
        tree.bind('<Double-1>', self.handle_property_double_click)

        parent_frame.add(ttk.Label(parent_frame, text='Properties'))
        parent_frame.add(frame, height=480)
        self.properties_tree = tree

    def properties_sort(self, list):
        def compare_strings(item1, item2):
            if (item2.name > item1.name):
                return -1
            elif (item2.name < item1.name):
                return 1
            else:
                return 0

        new_list = [item for item in list]
        sorted_list = sorted(new_list, key=cmp_to_key(compare_strings))
        return sorted_list

    def properties_list(self, node, parent=None):
        def printed_value(value_type, value):
            if value_type == daq.CoreType.ctBool:
                return yes_no[value]
            else:
                return value

        properties_info = node.visible_properties
        sorted_properties_info = self.properties_sort(properties_info)
        for property_info in sorted_properties_info:
            iid = id(property_info)
            self.nodes_by_iids[str(iid)] = node

            show_read_write = 'R/W'
            if property_info.read_only:
                show_read_write = 'R'

            if property_info.selection_values is not None:
                property_value = printed_value(
                    property_info.item_type, node.get_property_selection_value(property_info.name))
            elif property_info.value_type == daq.CoreType.ctProc:
                property_value = "Method"
                show_read_write = 'X'
            elif property_info.value_type == daq.CoreType.ctFunc:
                property_value = "Method"
                show_read_write = 'X'
            else:
                property_value = printed_value(
                    property_info.item_type, node.get_property_value(property_info.name))
            property_unit_symbol = ''
            if property_info.unit:
                property_unit_symbol = property_info.unit.symbol or ''

            self.properties_tree.insert('' if not parent else parent, tk.END, iid=iid, text=property_info.name, values=(
                property_value, property_unit_symbol, show_read_write))

            if property_info.value_type == daq.CoreType.ctObject:
                self.properties_list(
                    node.get_property_value(property_info.name), iid)

    def properties_update(self):
        self.properties_clear()
        self.nodes_by_iids = {}
        if (self.selected_node is not None):
            self.properties_list(self.selected_node)

    def properties_clear(self):
        self.properties_tree.delete(*self.properties_tree.get_children())

    #!SECTION - Properties view
    # SECTION - Attributes view

    def attributes_widget_create(self, parent_frame):
        frame = tk.Frame()
        # define columns
        tree = ttk.Treeview(frame, columns=(
            'value', 'access'), show='tree headings')

        scroll_bar = ttk.Scrollbar(
            frame, orient="vertical", command=tree.yview)
        tree.configure(yscrollcommand=scroll_bar.set)
        scroll_bar.pack(side="right", fill="y")
        tree.pack(fill="both", expand=True)

        # define headings
        tree.heading('#0', text='Name')
        tree.heading('#1', text='Value')
        tree.heading('#2', text='Locked')
        # layout
        tree.column('#0', anchor=tk.CENTER, width=80)
        tree.column('#1', anchor=tk.CENTER, stretch=True)
        tree.column('#2', anchor=tk.CENTER, width=80, stretch=False)
        # tree.grid(row=0, column=2, sticky=tk.NSEW)
        style = ttk.Style()
        style.configure("Treeview.Heading", font='Arial 10 bold')

        tree.bind("<Double-1>", self.handle_attributes_double_click)

        parent_frame.add(ttk.Label(parent_frame, text='Attributes'))
        parent_frame.add(frame, height=180)
        self.attributes_tree = tree
        self.attributes = {}

    def attributes_update(self):
        self.attributes_tree.delete(*self.attributes_tree.get_children())

        self.attributes = {}
        component = self.selected_node

        if component is None:
            return

        self.attributes['Name'] = {
            'Value': component.name, 'Locked': False, 'Attribute': 'name'}
        self.attributes['Description'] = {
            'Value': component.description, 'Locked': False, 'Attribute': 'description'}
        self.attributes['Active'] = {'Value': bool(
            component.active), 'Locked': False, 'Attribute': 'active'}
        self.attributes['Global ID'] = {
            'Value': component.global_id, 'Locked': True, 'Attribute': 'global_id'}
        self.attributes['Local ID'] = {
            'Value': component.local_id, 'Locked': True, 'Attribute': 'local_id'}
        self.attributes['Tags'] = {
            'Value': component.tags.list, 'Locked': False, 'Attribute': 'tags'}
        self.attributes['Visible'] = {'Value': bool(
            component.visible), 'Locked': False, 'Attribute': 'visible'}

        if daq.ISignal.can_cast_from(component):
            signal = daq.ISignal.cast_from(component)

            self.attributes['Public'] = {'Value': bool(
                signal.public), 'Locked': False, 'Attribute': 'public'}
            self.attributes['Domain Signal ID'] = {
                'Value': signal.domain_signal.global_id if signal.domain_signal else '', 'Locked': True, 'Attribute': '.domain_signal'}
            self.attributes['Related Signals IDs'] = {'Value': os.linesep.join(
                [s.global_id for s in signal.related_signals]), 'Locked': True, 'Attribute': 'related_signals'}
            self.attributes['Streamed'] = {'Value': bool(
                signal.streamed), 'Locked': True, 'Attribute': 'streamed'}
            self.attributes['Last Value'] = {
                'Value': signal.last_value, 'Locked': True, 'Attribute': 'last_value'}

        if daq.IInputPort.can_cast_from(component):
            input_port = daq.IInputPort.cast_from(component)

            self.attributes['Signal ID'] = {
                'Value': input_port.signal.global_id if input_port.signal else '', 'Locked': True, 'Attribute': 'signal'}
            self.attributes['Requires Signal'] = {'Value': bool(
                input_port.requires_signal), 'Locked': True, 'Attribute': 'requires_signal'}

        locked_attributes = component.locked_attributes

        for locked_attribute in locked_attributes:
            if locked_attribute not in self.attributes:
                continue
            self.attributes[locked_attribute]['Locked'] = True

        for attr in self.attributes:
            value = self.attributes[attr]['Value']
            locked = yes_no[self.attributes[attr]['Locked']]

            if type(value) is bool:
                value = yes_no[value]
            self.attributes_tree.insert(
                '', tk.END, iid=attr, text=attr, values=(value, locked))

    #!SECTION - Attributes view

    # SECTION - Handlers
        # SECTION - Button handlers

    def handle_add_device_button_clicked(self):
        nearest_device = self.get_nearest_device(self.selected_node)

        window = tk.Toplevel(self)
        window.title(f"Add device to {nearest_device.info.name}")
        window.geometry('{}x{}'.format(
            600*self.ui_scaling_factor, 400*self.ui_scaling_factor))

        tree = ttk.Treeview(window, columns=('used', 'name', 'conn'), displaycolumns=(
            'used', 'name', 'conn'), show='tree headings', selectmode='browse')
        scroll_bar = ttk.Scrollbar(
            window, orient="vertical", command=tree.yview)
        tree.configure(yscrollcommand=scroll_bar.set)
        scroll_bar.pack(side="right", fill="y")

        self.device_tree = tree

        # define headings
        tree.heading('used', text='Used')
        tree.heading('name', text='Name')
        tree.heading('conn', text='Connection string')

        # layout
        tree.column('#0', anchor=tk.CENTER, width=0,   stretch=False)
        tree.column('#1', anchor=tk.CENTER, width=80,  stretch=False)
        tree.column('#2', anchor=tk.CENTER, width=200, stretch=True)
        tree.column('#3', anchor=tk.CENTER, width=350, stretch=True)

        # bind double-click to editing
        tree.bind('<Double-1>', self.handle_devices_tree_double_click)

        tree.pack(fill="both", expand=True)

        self.scan_devices(nearest_device)
        for conn in self.all_devices[nearest_device.global_id]:
            device_info = self.all_devices[nearest_device.global_id][conn]['device_info']
            name = device_info.name
            used = self.all_devices[nearest_device.global_id][conn]['enabled']
            tree.insert('', tk.END, iid=conn, values=(
                yes_no[used], name, conn))

        show_modal(window)

    def handle_add_function_block_button_clicked(self):
        nearest_device = self.get_nearest_device(self.selected_node)

        window = tk.Toplevel(self)
        window.title(f"Add function block to {nearest_device.info.name}")
        window.geometry('{}x{}'.format(
            600*self.ui_scaling_factor, 400*self.ui_scaling_factor))

        tree = ttk.Treeview(window, columns=('id', 'name'), displaycolumns=(
            'id', 'name'), show='tree headings', selectmode='browse')
        scroll_bar = ttk.Scrollbar(
            window, orient="vertical", command=tree.yview)
        tree.configure(yscrollcommand=scroll_bar.set)
        scroll_bar.pack(side="right", fill="y")

        # define headings
        tree.heading('id', text='TypeId')
        tree.heading('name', text='Name')

        # layout
        tree.column('#0', anchor=tk.CENTER, width=0,    stretch=False)
        tree.column('#1', anchor=tk.CENTER, width=300 *
                    self.ui_scaling_factor,  stretch=False)
        tree.column('#2', anchor=tk.CENTER, width=300 *
                    self.ui_scaling_factor,  stretch=True)

        # bind double-click to editing
        tree.bind('<Double-1>',
                  lambda event: self.handle_add_function_block_tree_double_click(event, tree, window, nearest_device))

        tree.pack(fill="both", expand=True)

        available_function_block_types = nearest_device.available_function_block_types
        for function_block_id in available_function_block_types.keys():
            tree.insert('', tk.END, iid=function_block_id, values=(
                function_block_id, daq.IFunctionBlockType.cast_from(available_function_block_types[function_block_id]).name))

        show_modal(window)

    def handle_save_config_button_clicked(self):
        file = asksaveasfile(initialfile='config.json', title="Save configuration",
                             defaultextension=".json", filetypes=[("All Files", "*.*"), ("Json", "*.json")])
        if file is None:
            return
        config_string = self.instance.save_configuration()
        a = file.write(config_string)
        file.close()

    def handle_load_config_button_clicked(self):
        file = askopenfile(initialfile='config.json', title="Load configuration",
                           defaultextension=".json", filetypes=[("All Files", "*.*"), ("Json", "*.json")])
        if file is None:
            return
        config_string = file.read()
        file.close()
        self.instance.load_configuration(config_string)
        self.tree_update()

    def handle_refresh_button_clicked(self):
        self.tree_update(self.selected_node)

    def handle_connect_button_clicked(self):
        node = self.selected_node
        node_is_input_port = node is not None and daq.IInputPort.can_cast_from(node)
        if node_is_input_port:
            self.handle_tree_menu_connect_input_port(None)

    def handle_remove_button_clicked(self):
        node = self.selected_node
        node_is_function_block = node is not None and daq.IFunctionBlock.can_cast_from(node)
        node_is_device = node is not None and daq.IDevice.can_cast_from(node)
        if node_is_function_block:
            self.handle_tree_menu_remove_function_block(node)
        elif node_is_device:
            self.handle_tree_menu_remove_device(node)

        #!SECTION - Button handlers
        # SECTION - Tree view handlers

    def handle_tree_right_button(self, event):
        iid = event.widget.identify_row(event.y)
        if iid:
            event.widget.selection_set(iid)
        else:
            event.widget.selection_set()

    def handle_tree_right_button_release(self, event):
        iid = self.treeview_get_first_selection(self.tree)

        self.tree_popup.entryconfig(
            'Remove', state='disabled', command=None
        )
        self.tree_popup.entryconfig(
            'Connect', state='disabled', command=None
        )

        if iid:
            node = self.find_component(iid)
            if node:
                if daq.IFunctionBlock.can_cast_from(node):
                    self.tree_popup.entryconfig(
                        "Remove", state="normal", command=lambda: self.handle_tree_menu_remove_function_block(node))
                elif daq.IDevice.can_cast_from(node):
                    self.tree_popup.entryconfig(
                        "Remove", state="normal", command=lambda: self.handle_tree_menu_remove_device(node))
                elif daq.IInputPort.can_cast_from(node):
                    self.tree_popup.entryconfig(
                        "Connect", state="normal", command=lambda: self.handle_tree_menu_connect_input_port(event))

        try:
            self.tree_popup.tk_popup(event.x_root, event.y_root, 0)
        finally:
            self.tree_popup.grab_release()

    def handle_tree_select(self, event):
        self.properties_clear()

        selected_item = self.treeview_get_first_selection(self.tree)
        if selected_item is None:
            self.selected_node = None
            self.tree_update_conext_buttons_state()
            return
        item = self.tree.item(selected_item)

        node_unique_id = item['values'][0]
        if not node_unique_id in self.nodes:
            self.properties_clear()
            return
        node = self.nodes[node_unique_id]
        self.selected_node = node
        self.tree_update_conext_buttons_state()
        if node != None and (type(node) is daq.PropertyObject or daq.IPropertyObject.can_cast_from(node)):
            self.properties_list(node)
        self.attributes_update()

    def handle_tree_menu_remove_function_block(self, node):
        if node is None:
            return

        fbs = self.get_nearest_named_parent_folder(node.parent, 'FB')
        if fbs is None:
            return

        fbs.remove_item(node)
        self.selected_node = fbs
        self.tree_update(self.selected_node)

    def handle_tree_menu_remove_device(self, node):
        if type(node) is not daq.IDevice:
            node = daq.IDevice.cast_from(
                node) if daq.IDevice.can_cast_from(node) else None

        if not node:
            return

        if node.parent:
            parent_device = self.get_nearest_device(node.parent)

            parent_device.remove_device(node)

            #return initial state to connection mapped info
            device_info_id_mapped = self.all_devices[parent_device.global_id][node.global_id]['device_info']
            device_state_conn_mapped = self.all_devices[parent_device.global_id][device_info_id_mapped.connection_string]
            device_state_conn_mapped['enabled'] = False
            device_state_conn_mapped['device'] = None

            #removing id mapped info
            self.all_devices[parent_device.global_id].pop(node.global_id, None)

            self.selected_node = parent_device
        self.tree_update(self.selected_node)

    def handle_tree_menu_connect_input_port(self, event):
        if self.selected_node is None:
            return
        if not daq.IInputPort.can_cast_from(self.selected_node):
            return

        port = daq.IInputPort.cast_from(self.selected_node)

        window = tk.Toplevel(self)
        window.title("Connect signal to input port")
        window.geometry('{}x{}'.format(
            800*self.ui_scaling_factor, 400*self.ui_scaling_factor))

        tree = ttk.Treeview(window, columns=('name', 'id'), displaycolumns=(
            'name', 'id'), show='tree headings', selectmode='browse')
        scroll_bar = ttk.Scrollbar(
            window, orient="vertical", command=tree.yview)
        tree.configure(yscrollcommand=scroll_bar.set)
        scroll_bar.pack(side="right", fill="y")

        # define headings
        tree.heading('name', text='Name')
        tree.heading('id', text='GlobalId')

        # layout
        tree.column('#0', anchor=tk.CENTER, width=0,    stretch=False)
        tree.column('#1', anchor=tk.CENTER, width=250 *
                    self.ui_scaling_factor,  stretch=False)
        tree.column('#2', anchor=tk.CENTER, width=550 *
                    self.ui_scaling_factor,  stretch=True)

        # bind double-click to editing
        tree.bind(
            '<Double-1>', lambda event: self.handle_tree_connect_signal_to_input_port(tree, window, port))

        tree.pack(fill="both", expand=True)

        tree.insert('', tk.END, iid="__none__", values=("[Unassigned]", "N/A"))

        signals = self.instance.signals_recursive
        for signal in signals:
            if signal.domain_signal is not None:
                tree.insert('', tk.END, iid=signal.global_id,
                            values=(signal.name, signal.global_id))

        show_modal(window)

    def handle_tree_connect_signal_to_input_port(self, tree, window, port):
        global_id = self.treeview_get_first_selection(tree)
        if global_id is None:
            return

        if global_id == "__none__":
            port.disconnect()
        else:
            signals = self.instance.signals_recursive
            for signal in signals:
                if signal.global_id == global_id:
                    port.connect(signal)

        window.destroy()

        self.tree_update(self.selected_node)

        #!SECTION - Tree view handlers
        # SECTION - Property view handlers

    def handle_property_double_click(self, event):
        '''Fire a dialog to change the selected value after a double-click'''

        selected_item = self.treeview_get_first_selection(self.properties_tree)
        if selected_item is None:
            return
        item = self.properties_tree.item(selected_item)
        property_name = item['text']
        node = self.nodes_by_iids.get(selected_item)
        if not node:
            return

        property_info = node.get_property(property_name)
        property_value = node.get_property_value(property_name)
        if not property_info:
            return

        if property_info.value_type == daq.CoreType.ctFunc:
            f = daq.IFunction.cast_from(property_value)
            f()  # only functions without parameters
            return

        if property_info.value_type == daq.CoreType.ctProc:
            p = daq.IProcedure.cast_from(property_value)
            p()  # only functions without parameters
            return

        if (property_info == None or property_info.read_only):
            return

        prompt = 'Enter the new value for {}:'.format(property_name)

        if property_info.value_type == daq.CoreType.ctBool:
            property_value = not property_value
        elif property_info.value_type == daq.CoreType.ctInt:
            # TODO
            # min_value = property_info.min_value
            # max_value = property_info.max_value
            # property_value = simpledialog.askinteger(property_name, prompt=prompt, initialvalue=property_value, minvalue=min_value, maxvalue=max_value)
            if property_info.selection_values is not None:
                property_value = show_selection(
                    prompt, property_value, property_info.selection_values)
            else:
                property_value = simpledialog.askinteger(
                    property_name, prompt=prompt, initialvalue=property_value)
        elif property_info.value_type == daq.CoreType.ctFloat:
            # TODO
            # min_value = property_info.min_value
            # max_value = property_info.max_value
            # property_value = simpledialog.askfloat(property_name, prompt=prompt, initialvalue=property_value, minvalue=min_value, maxvalue=max_value)
            property_value = simpledialog.askfloat(
                property_name, prompt=prompt, initialvalue=property_value)
        elif property_info.value_type == daq.CoreType.ctString:
            property_value = simpledialog.askstring(
                property_name, prompt=prompt, initialvalue=property_value)
        else:
            return

        if property_value is None:
            return

        node.set_property_value(property_name, property_value)
        self.properties_update()

        #!SECTION - Property view handlers
        # SECTION - Attributes view handlers

    def handle_attributes_double_click(self, event):
        node = self.selected_node
        if not node:
            return

        sel = self.treeview_get_first_selection(self.attributes_tree)
        if sel not in self.attributes:
            return

        attr_dict = self.attributes[sel]

        if attr_dict['Locked']:
            return

        if daq.ISignal.can_cast_from(node):
            node = daq.ISignal.cast_from(node)
        elif daq.IInputPort.can_cast_from(node):
            node = daq.IInputPort.cast_from(node)

        new_value = None
        value = attr_dict['Value']
        attribute = attr_dict['Attribute']

        prompt = f'Enter the new value for {sel}:'

        if type(value) is bool:
            new_value = not value
            pass
        elif type(value) is int:
            new_value = simpledialog.askinteger(
                sel, prompt=prompt, initialvalue=value)
            pass
        elif type(value) is str:
            new_value = simpledialog.askstring(
                sel, prompt=prompt, initialvalue=value)
            pass

        if new_value is None or new_value == value:
            return

        setattr(node, attribute, new_value)

        print(f'Value changed for {sel}: {value} -> {new_value}')

        self.tree_update(self.selected_node)

        #!SECTION - Attributes view handlers
        # SECTION - Handlers other

    def handle_devices_tree_double_click(self, event):
        nearest_device = self.get_nearest_device(self.selected_node)
        selected_item = self.treeview_get_first_selection(self.device_tree)
        if selected_item is None:
            return
        item = self.device_tree.item(selected_item)

        conn = item['values'][2]
        if not conn in self.all_devices[nearest_device.global_id]:
            print("Something is wrong, device not found")
            return

        device_state_conn_mapped = self.all_devices[nearest_device.global_id][conn]
        will_be_enabled = not device_state_conn_mapped['enabled']
        # will_be_enabled
        if will_be_enabled:
            try:
                device = self.instance.add_device(conn)
                
                device_state_conn_mapped['device'] = device 
                device_state_conn_mapped['enabled'] = True

                device_info = device_state_conn_mapped['device_info']
                if isinstance(device_info, DeviceInfoLocal):
                    device_info.name = device.local_id
                    device_info.serial_number = device.info.serial_number

                self.all_devices[nearest_device.global_id][device.global_id] = device_state_conn_mapped

            except RuntimeError as e:
                print(f'Error adding device: {e}')
                device_state_conn_mapped['device'] = None
                device_state_conn_mapped['enabled'] = False
        self.tree_update(self.selected_node)

    def handle_add_function_block_tree_double_click(self, event, tree, window, device):
        selected_item = self.treeview_get_first_selection(tree)
        if selected_item is None:
            return

        item = tree.item(selected_item)

        function_block_id = item['values'][0]
        fb = device.add_function_block(function_block_id)

        self.tree_update(fb)

        window.destroy()

        #!SECTION - Handlers other
    #!SECTION - Handlers
    # SECTION - Other

    def find_component(self, id, parent=None, convert_id=True):
        parent = parent if parent else self.instance
        if convert_id:
            split_id = id.split('/')
            id = '/'.join(split_id[2:])
        return parent.find_component(id)

    def treeview_get_first_selection(self, treeview):
        sel = treeview.selection()
        if len(sel) == 0:
            return None
        return sel[0]

    def get_nearest_device(self, component: daq.IComponent):
        while component:
            if daq.IDevice.can_cast_from(component):
                return daq.IDevice.cast_from(component)
            component = component.parent

        return self.instance

    def get_nearest_named_parent_folder(self, component, name):
        while component:
            if daq.IFolderConfig.can_cast_from(component):
                folder = daq.IFolderConfig.cast_from(component)
                if folder.name == name:
                    return folder
            component = component.parent
        return None

    #!SECTION - Other

# ANCHOR - Entry point


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Display openDAQ device configuration and plot values')
    parser.add_argument('--scale', help='UI scaling factor',
                        type=int, default=1.0)
    parser.add_argument('--connection_string',
                        help='Connection string', type=str, default='')
    parser.add_argument(
        '--demo', help='Include internal demo/reference devices', action='store_true')

    app = App(parser.parse_args())
    app.mainloop()
