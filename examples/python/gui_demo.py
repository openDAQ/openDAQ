#!/usr/bin/env python

import argparse
import ipaddress
import tkinter as tk
from tkinter import ttk, simpledialog
from tkinter.filedialog import asksaveasfile, askopenfile
import tkinter.font as tkfont
import uuid

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

class DeviceInfoLocal:
    def __init__(self, ip):
        self.name = ip
        self.connection_string = 'daq.opcua://{}'.format(ip)
        self.serial_number = 'no-serial-number'

def showSelection(title, current_value, values: daq.IList):
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
        button = tk.Button(top, text=sel_text + value, command=make_closure(idx))
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

    top.wait_visibility()
    top.grab_set()
    top.wait_window(top)
    return result

class App(tk.Tk):
    def __init__(self, args):
        super().__init__()

        self.ui_scaling_factor = int(args.scale)
        self.include_reference_devices = bool(args.demo)
        try:
            ipaddress.ip_address(args.ip)
            self.ip = args.ip
        except ValueError:
            self.ip = None

        self.title('openDAQ demo')
        self.geometry('{}x{}'.format(1400*self.ui_scaling_factor, 1000*self.ui_scaling_factor))

        main_frame_top = tk.Frame(self)
        main_frame_top.pack(fill=tk.constants.X)
        main_frame_top.configure(background='white')

        search_button = tk.Button(main_frame_top, text='Scan devices', command=self.open_device_selection_window)
        search_button.pack(side=tk.constants.LEFT, padx=5, pady=5, fill=tk.constants.X)

        add_function_block_button = tk.Button(main_frame_top, text='Add function block', command=self.open_add_function_block_window)
        add_function_block_button.pack(side=tk.constants.LEFT, padx=5, pady=5, fill=tk.constants.X)

        save_config_button = tk.Button(main_frame_top, text='Save configuration', command=self.save_config)
        save_config_button.pack(side=tk.constants.LEFT, padx=5, pady=5, fill=tk.constants.X)

        load_config_button = tk.Button(main_frame_top, text='Load configuration', command=self.load_config)
        load_config_button.pack(side=tk.constants.LEFT, padx=5, pady=5, fill=tk.constants.X)

        main_frame_bottom = tk.Frame(self)
        main_frame_bottom.pack(fill=tk.constants.BOTH, expand=True)

        main_frame_navigator = tk.PanedWindow(main_frame_bottom, orient=tk.constants.HORIZONTAL)
        main_frame_navigator.pack_propagate(0)

        frame_navigator_for_properties = tk.PanedWindow(main_frame_navigator, orient=tk.constants.VERTICAL)
        frame_navigator_for_properties.pack_propagate(0)

        self.create_tree_widget(main_frame_navigator)

        main_frame_navigator.add(frame_navigator_for_properties)
        self.create_property_widget(frame_navigator_for_properties)
        self.create_input_ports_widget(frame_navigator_for_properties)
        self.create_output_signals_widget(frame_navigator_for_properties)

        main_frame_navigator.pack(side=tk.LEFT, expand=1, fill=tk.BOTH)

        # High DPI workaround for now
        ttk.Style().configure('Treeview', rowheight=40*self.ui_scaling_factor)

        default_font = tkfont.nametofont("TkDefaultFont")
        default_font.configure(size=9*self.ui_scaling_factor)

        self.nodes = {}
        self.output_signal_nodes = {}
        self.selected_node = None
        self.signal_is_rendered = {}

        self.init_opendaq()

    #
    # INIT OPENDAQ
    #

    def init_opendaq(self):
        # init device
        instance = daq.Instance()

        for function_block_id in instance.available_function_block_types.keys():
            if function_block_id == 'ref_fb_module_renderer':
                renderer = instance.add_function_block(function_block_id)

        self.instance = instance
        self.opendaq_renderer = renderer

        self.all_devices = {}
        self.scan_devices()
        if self.ip != None:
            self.add_first_available_device() # also calls self.update_tree_widget()
        self.update_tree_widget()

    def scan_devices(self):
        def add_device(device_info):
            conn = device_info.connection_string
            # ignore reference devices unless explicitly requested
            if not self.include_reference_devices and 'daqref' in device_info.connection_string:
                return
            # only add device to the list if it isn't there already
            if not conn in self.all_devices:
                self.all_devices[conn] = {'device_info': device_info, 'enabled': False, 'device': None}

        if self.ip != None:
            add_device(DeviceInfoLocal(self.ip))
        for device_info in self.instance.available_devices:
            add_device(device_info)

    #
    # OPENDAQ: Functionality
    #

    def add_first_available_device(self):
        for connection_string in self.all_devices:
            device = self.all_devices[connection_string]
            if not device['enabled']:
                if device['device'] == None:
                    device['device'] = self.instance.add_device(connection_string)
                device['enabled'] = True
                device['device_info'].name = device['device'].local_id
                device['device_info'].serial_number = device['device'].info.serial_number
                break
        self.update_tree_widget()

    #
    # INIT WIDGETS
    #

    #def create_menu(self):
    #    menubar = tk.Menu(self)
    #    self.config(menu=menubar)
    #    self.menubar = menubar
    #
    #    file_menu = tk.Menu(menubar, tearoff=False)
    #    file_menu.add_command(label='Add or remove devices', command=self.open_device_selection_window)
    #    file_menu.add_command(label='Add device', command=self.add_first_available_device)
    #    file_menu.add_command(label='Exit', command=self.destroy)
    #    menubar.add_cascade(label="File", menu=file_menu, underline=0)

    def create_tree_widget(self, parent_frame):
        # define columns
        tree = ttk.Treeview(parent_frame, columns=('name', 'hash'), displaycolumns=('name'), show='tree', selectmode='browse')

        # layout
        tree.column('#0', width=350*self.ui_scaling_factor)
        # hide the column with unique id
        tree.column('#1', width=0, minwidth=0, stretch=False)
        #tree.grid(row=0, column=0, rowspan=2, sticky=tk.NSEW)

        # bind selection
        tree.bind('<<TreeviewSelect>>', self.tree_item_selected)
        # bind double-click to add a channel to the renderer
        #tree.bind('<Double-1>', self.toggle_channel)

        # add a scrollbar
        #scrollbar = ttk.Scrollbar(self, orient=tk.VERTICAL, command=tree.yview)
        #tree.configure(yscroll=scrollbar.set)
        #scrollbar.grid(row=0, column=1, rowspan=2, sticky=tk.NS)

        parent_frame.add(tree)
        self.tree = tree

    def create_property_widget(self, parent_frame):
        # define columns
        tree = ttk.Treeview(parent_frame, columns=('value', 'unit', 'access'), show='tree headings')
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
        tree.grid(row=0, column=2, sticky=tk.NSEW)
        style = ttk.Style()
        style.configure("Treeview.Heading", font='Arial 10 bold')
        # bind double-click to editing
        tree.bind('<Double-1>', self.set_tree_cell_value)

        parent_frame.add(ttk.Label(parent_frame, text='Properties'))
        parent_frame.add(tree, height=480)
        self.property_tree = tree

    def create_input_ports_widget(self, parent_frame):
        # define columns
        tree = ttk.Treeview(parent_frame, columns=('signal', 'disconnect'), show='tree headings')
        # define headings
        tree.heading('#0', text='Port name')
        tree.heading('signal', text='Connected signal')
        tree.heading('disconnect', text='Disconnect')
        # layout
        tree.column('#0', anchor=tk.CENTER)
        tree.column('#1', anchor=tk.CENTER)
        tree.column('#2', anchor=tk.CENTER, width=250, stretch=False)
        #tree.grid(row=0, column=2, sticky=tk.NSEW)
        style = ttk.Style()
        style.configure("Treeview.Heading", font='Arial 10 bold')
        # bind double-click to editing
        tree.bind('<Double-1>', self.open_connect_input_port_window)

        parent_frame.add(ttk.Label(parent_frame, text='Input ports'))
        parent_frame.add(tree, height=180)
        self.input_ports_widget = tree

    def create_output_signals_widget(self, parent_frame):
        # define columns
        tree = ttk.Treeview(parent_frame, columns=('description', 'active', 'unique_id'), displaycolumns=('description', 'active'), show='tree headings')
        # define headings
        tree.heading('#0', text='Name')
        tree.heading('description', text='Description')
        tree.heading('active', text='Active')
        # layout
        tree.column('#0', anchor=tk.CENTER)
        tree.column('#1', anchor=tk.CENTER)
        tree.column('#2', anchor=tk.CENTER, width=200, stretch=False)
        #tree.grid(row=0, column=2, sticky=tk.NSEW)
        style = ttk.Style()
        style.configure("Treeview.Heading", font='Arial 10 bold')
        # bind double-click to editing
        tree.bind('<Double-1>', self.toggle_signal)

        parent_frame.add(ttk.Label(parent_frame, text='Output signals'))
        parent_frame.add(tree, height=180)
        self.output_signals_widget = tree

    #
    # WIDGETS: Interaction
    #

    # (Property tree)

    def clear_property_tree(self):
        self.property_tree.delete(*self.property_tree.get_children())
        self.input_ports_widget.delete(*self.input_ports_widget.get_children())
        self.output_signals_widget.delete(*self.output_signals_widget.get_children())
        self.output_signal_nodes.clear()

    def list_properties(self, node):
        def printed_value(value_type, value):
            if value_type == daq.CoreType.ctBool:
                return yes_no[value]
            else:
                return value

        properties_info = node.visible_properties
        for property_info in properties_info:
            show_read_write = 'R/W'
            if property_info.read_only:
                show_read_write = 'R'

            if property_info.selection_values is not None:
                property_value = printed_value(property_info.item_type, node.get_property_selection_value(property_info.name))
            elif property_info.value_type == daq.CoreType.ctProc:
                property_value = "Method"
                show_read_write = 'X'
            elif property_info.value_type == daq.CoreType.ctFunc:
                property_value = "Method"
                show_read_write = 'X'
            else:
                property_value = printed_value(property_info.item_type, node.get_property_value(property_info.name))
            property_unit_symbol = ''
            if property_info.unit:
                property_unit_symbol = property_info.unit.symbol or ''
            self.property_tree.insert('', tk.END, text=property_info.name, values=(property_value, property_unit_symbol, show_read_write))

        if (type(node) is daq.IFunctionBlock):
            input_ports = node.input_ports
            for input_port in input_ports:
                signal = input_port.signal
                if signal != None:
                    comp = daq.IComponent.cast_from(signal)
                    name = comp.name
                    disconnect = '[Disconnect]'
                else:
                    name = '(None)'
                    disconnect = ''
                self.input_ports_widget.insert('', tk.END, iid=input_port.local_id, text=input_port.name, values=(name, disconnect))

        if (type(node) is daq.IFunctionBlock) or (type(node) is daq.IChannel) or (type(node) is daq.IDevice):
            for output_signal in node.signals:
                if output_signal.domain_signal == None:
                    continue
                unique_id = output_signal.global_id                
                comp = daq.IComponent.cast_from(output_signal)
                description = output_signal.description
                is_active = output_signal.active
                self.output_signal_nodes[unique_id] = output_signal
                self.output_signals_widget.insert('', tk.END, text=comp.name, values=(description, yes_no[is_active], unique_id))

    def update_properties(self):
        self.clear_property_tree()
        self.list_properties(self.selected_node)

    def tree_item_selected(self, event):
        self.clear_property_tree()
        selected_item = self.tree.selection()[0]
        item = self.tree.item(selected_item)

        node_unique_id = item['values'][0]
        if not node_unique_id in self.nodes:
            self.clear_property_tree()
            return
        node = self.nodes[node_unique_id]
        self.selected_node = node
        if node != None and not (type(node) is daq.ISignalConfig):
            self.list_properties(node)

    def set_tree_cell_value(self, event):
        '''Fire a dialog to change the selected value after a double-click'''

        selected_items = self.property_tree.selection()
        if len(selected_items) < 1:
            return
        selected_item = selected_items[0]
        item = self.property_tree.item(selected_item)
        property_name = item['text']
        if self.selected_node == None:
            return
        property_info = self.selected_node.get_property(property_name)
        property_value = self.selected_node.get_property_value(property_name)
        if (property_info == None):
            return

        if property_info.value_type == daq.CoreType.ctFunc:
            f = daq.IFunction.cast_from(property_value)
            f() # only functions without parameters
            return

        if property_info.value_type == daq.CoreType.ctProc:
            p = daq.IProcedure.cast_from(property_value)
            p() # only functions without parameters
            return

        if (property_info == None or property_info.read_only):
            return

        prompt = 'Enter the new value for {}:'.format(property_name)

        match property_info.value_type:
            case daq.CoreType.ctBool:
                property_value = not property_value
            case daq.CoreType.ctInt:
                # TODO
                #min_value = property_info.min_value
                #max_value = property_info.max_value
                #property_value = simpledialog.askinteger(property_name, prompt=prompt, initialvalue=property_value, minvalue=min_value, maxvalue=max_value)        
                if property_info.selection_values is not None:
                    property_value = showSelection(prompt, property_value, property_info.selection_values)
                else:
                    property_value = simpledialog.askinteger(property_name, prompt=prompt, initialvalue=property_value)
            case daq.CoreType.ctFloat:
                # TODO
                #min_value = property_info.min_value
                #max_value = property_info.max_value
                #property_value = simpledialog.askfloat(property_name, prompt=prompt, initialvalue=property_value, minvalue=min_value, maxvalue=max_value)
                property_value = simpledialog.askfloat(property_name, prompt=prompt, initialvalue=property_value)
            case daq.CoreType.ctString:
                property_value = simpledialog.askstring(property_name, prompt=prompt, initialvalue=property_value)
            case _:
                return
        if property_value == None:
            return

        self.selected_node.set_property_value(property_name, property_value)
        self.update_properties()

    # (Signals)

    def toggle_signal(self, event):
        '''Add or remove a channel from the renderer'''

        # connects a signal to the renderer
        def add_to_renderer(signal, unique_id):
            input_ports_from_renderer = self.opendaq_renderer.input_ports
            index = len(input_ports_from_renderer) - 1
            input_ports_from_renderer[index].connect(signal)
            self.signal_is_rendered[unique_id] = index

        def remove_from_renderer(signal, unique_id):
            self.signal_is_rendered.pop(unique_id)
            input_ports_from_renderer = self.opendaq_renderer.input_ports
            for input_port in input_ports_from_renderer:
                connected_signal = input_port.signal
                if connected_signal == None:
                    continue
                if connected_signal.global_id == signal.global_id:
                    input_port.disconnect()

        selected_item = self.output_signals_widget.selection()[0]
        item = self.output_signals_widget.item(selected_item)

        unique_id = item['values'][2]
        # this should not happen, but just in case we aren't able to locate the signal, simply return
        if not unique_id in self.output_signal_nodes:
            return
        signal = self.output_signal_nodes[unique_id]

        #is_rendered = len(signal.connections) > 0
        is_rendered = unique_id in self.signal_is_rendered

        # toggle activity of a channel
        if not is_rendered:
            add_to_renderer(signal, unique_id)
        else:
            remove_from_renderer(signal, unique_id)

    # (Tree)
    def add_component_to_tree(self, parent_node_id, component):
        component_node_id = id(component)
        component_name = component.name

        if (daq.IFolder.can_cast_from(component)):
            self.tree.insert(parent_node_id, tk.END, iid=component_node_id, text=component_name, open=True, values=(component_node_id))
            self.nodes[component_node_id] = component
            for item in daq.IFolder.cast_from(component).items:
                self.add_component_to_tree(component_node_id, item)
        else:
            self.tree.insert(parent_node_id, tk.END, iid=component_node_id, text=component_name, open=True, values=(component_node_id))
            self.nodes[component_node_id] = component
        
                

    def add_channel_to_tree(self, parent_node_id, channel):
        channel_node_id = id(channel)
        self.add_function_block_to_tree(parent_node_id, channel_node_id, channel)

    def add_device_to_tree(self, parent_node_id, device, connection_string=''):
        name = device.info.name
        serial_number = device.info.serial_number
        if 'daq.opcua://' in connection_string:
            device_ip = connection_string[5:].replace('/', '')
            name = '{} [{}]'.format(name, device_ip)

        # TODO: for now we generate a random serial number in case one doesn't exist yet
        if serial_number == '':
            serial_number = str(uuid.uuid4())
        node_id = serial_number
        # store the device under its serial number, so that we can retrieve it later
        self.nodes[serial_number] = device
        self.tree.insert(parent_node_id, tk.END, iid=node_id, text=name, open=True, values=(serial_number))

        # add all the channels belonging to the device as well
        channels = device.channels
        if len(channels) > 0:
            channels_node_id = node_id + '_channels'
            self.tree.insert(node_id, tk.END, iid=channels_node_id, text='Channels', open=True, values=(channels_node_id))
            for channel in channels:
                self.add_channel_to_tree(channels_node_id, channel)

        # add subdevices
        child_devices = device.devices
        if len(child_devices) > 0:
            devices_node_id = node_id + '_devices'
            self.tree.insert(node_id, tk.END, iid=devices_node_id, text='Devices', open=True, values=(devices_node_id))
            for child_device in child_devices:
                self.add_device_to_tree(devices_node_id, child_device)

        # add function blocks
        function_blocks = device.function_blocks
        if len(function_blocks) > 0:
            function_blocks_node_id = node_id + '_fbs'
            self.tree.insert(node_id, tk.END, iid=function_blocks_node_id, text='Function Blocks', open=True, values=(function_blocks_node_id))
            for function_block in function_blocks:
                fb_unique_id = node_id + '_' + function_block.function_block_type.id
                self.add_function_block_to_tree(function_blocks_node_id, fb_unique_id, function_block) 

        # add custom components
        custom_components = device.custom_components
        if len(custom_components) > 0:
            components_node_id = node_id + '_components'
            self.tree.insert(node_id, tk.END, iid=components_node_id, text='Components', open=True, values=(components_node_id))
            for custom_component in custom_components:
                self.add_component_to_tree(components_node_id, custom_component)

    def add_function_block_to_tree(self, parent_id, unique_id, node):
        self.nodes[unique_id] = node
        name = node.function_block_type.name
        self.tree.insert(parent_id, tk.END, iid=unique_id, text=name, open=True, values=(unique_id))

        for function_block in node.function_blocks:
            node_id = str(unique_id) + '_' + function_block.function_block_type.id
            self.add_function_block_to_tree(unique_id, node_id, function_block)

    def update_tree_widget(self):
        # make sure the tree is empty
        self.tree.delete(*self.tree.get_children())
        # function blocks
        functions_node_id = 'root_function_blocks'
        self.tree.insert('', tk.END, iid=functions_node_id, text='Function blocks', open=True, values=(functions_node_id))
        
        function_blocks = self.instance.function_blocks
        for function_block in function_blocks:
            fb_unique_id = functions_node_id + '_' + function_block.global_id
            self.add_function_block_to_tree(functions_node_id, fb_unique_id, function_block) 

        
        #self.add_function_block_to_tree(functions_node_id, 'renderer', self.opendaq_renderer)
        # devices
        self.tree.insert('', tk.END, iid='root_devices', text='Devices', open=True, values=('title_devices'))
        for connection_string in self.all_devices:
            device = self.all_devices[connection_string]
            if device['enabled'] and device['device'] != None:
                self.add_device_to_tree('root_devices', device['device'], device['device_info'].connection_string)

    # DEVICE SELECTION (TODO)

    def open_device_selection_window(self):
        window = tk.Toplevel(self)
        window.title("Add or remove devices")
        window.geometry('{}x{}'.format(600*self.ui_scaling_factor, 400*self.ui_scaling_factor))

        tree = ttk.Treeview(window, columns=('used', 'name', 'conn'), displaycolumns=('used', 'name', 'conn'), show='tree headings', selectmode='browse')
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
        tree.bind('<Double-1>', self.toggle_device)

        tree.grid(row=0, column=0, sticky=tk.NSEW)

        self.scan_devices()
        for conn in self.all_devices:
            device_info = self.all_devices[conn]['device_info']
            name = device_info.name
            used = self.all_devices[conn]['enabled']
            tree.insert('', tk.END, iid=conn, values=(yes_no[used], name, conn))

    def open_device_selection_window(self):
        window = tk.Toplevel(self)
        window.title("Add or remove devices")
        window.geometry('{}x{}'.format(600*self.ui_scaling_factor, 400*self.ui_scaling_factor))

        tree = ttk.Treeview(window, columns=('used', 'name', 'conn'), displaycolumns=('used', 'name', 'conn'), show='tree headings', selectmode='browse')
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
        tree.bind('<Double-1>', self.toggle_device)

        tree.grid(row=0, column=0, sticky=tk.NSEW)

        self.scan_devices()
        for conn in self.all_devices:
            device_info = self.all_devices[conn]['device_info']
            name = device_info.name
            used = self.all_devices[conn]['enabled']
            tree.insert('', tk.END, iid=conn, values=(yes_no[used], name, conn))

    def open_add_function_block_window(self):
        window = tk.Toplevel(self)
        window.title("Add function block")
        window.geometry('{}x{}'.format(600*self.ui_scaling_factor, 400*self.ui_scaling_factor))

        tree = ttk.Treeview(window, columns=('id', 'name'), displaycolumns=('id', 'name'), show='tree headings', selectmode='browse')
        
        self.add_fb_tree = tree

        # define headings
        tree.heading('id', text='TypeId')
        tree.heading('name', text='Name')

        # layout
        tree.column('#0', anchor=tk.CENTER, width=0,    stretch=False)
        tree.column('#1', anchor=tk.CENTER, width=300*self.ui_scaling_factor,  stretch=False)
        tree.column('#2', anchor=tk.CENTER, width=300*self.ui_scaling_factor,  stretch=True)

        # bind double-click to editing
        tree.bind('<Double-1>', self.add_function_block)

        tree.grid(row=0, column=0, sticky=tk.NSEW)

        available_function_block_types = self.instance.available_function_block_types
        for function_block_id in available_function_block_types.keys():
            tree.insert('', tk.END, iid=function_block_id, values=(function_block_id, daq.IFunctionBlockType.cast_from(available_function_block_types[function_block_id]).name))
            
    def add_function_block(self, event):
        selected_items = self.add_fb_tree.selection()
        if len(selected_items) < 1:
            return
            
        selected_item = selected_items[0]
        item = self.add_fb_tree.item(selected_item)

        function_block_id = item['values'][0]
        self.instance.add_function_block(function_block_id)

        self.update_tree_widget()
        
    def open_connect_input_port_window(self, event):
        selected_items = self.input_ports_widget.selection()
        if len(selected_items) < 1:
            return
       
        selected_item = selected_items[0]
        fb = daq.IFunctionBlock.cast_from(self.selected_node)     
        input_ports = daq.IFolder.cast_from(fb.get_item("ip"))
        port = daq.IInputPort.cast_from(input_ports.get_item(selected_item))
        self.ip = port

        window = tk.Toplevel(self)
        window.title("Connect signal to input port")
        window.geometry('{}x{}'.format(800*self.ui_scaling_factor, 400*self.ui_scaling_factor))

        tree = ttk.Treeview(window, columns=('id', 'name'), displaycolumns=('id', 'name'), show='tree headings', selectmode='browse')
        
        self.ip_tree = tree
        
        # define headings
        tree.heading('id', text='TypeId')
        tree.heading('name', text='Name')

        # layout
        tree.column('#0', anchor=tk.CENTER, width=0,    stretch=False)
        tree.column('#1', anchor=tk.CENTER, width=500*self.ui_scaling_factor,  stretch=False)
        tree.column('#2', anchor=tk.CENTER, width=300*self.ui_scaling_factor,  stretch=True)

        # bind double-click to editing
        tree.bind('<Double-1>', self.connect_signal_to_input_port)

        tree.grid(row=0, column=0, sticky=tk.NSEW)

        signals = self.instance.signals_recursive        
        for signal in signals:
            comp = daq.IComponent.cast_from(signal)
            if signal.domain_signal is not None:
                tree.insert('', tk.END, iid=signal.global_id, values=(signal.global_id, comp.name))
            
    def connect_signal_to_input_port(self, event):
        selected_item = self.ip_tree.selection()[0]
        item = self.ip_tree.item(selected_item)
        
        global_id = item['values'][0]
        signals = self.instance.signals_recursive        
        for signal in signals:
            if signal.global_id == global_id:
                self.ip.connect(signal)
        
        self.update_properties()
        self.ip = None
        
    def save_config(self):
        file = asksaveasfile(initialfile = 'config.json', title = "Save configuration", defaultextension=".json",filetypes=[("All Files","*.*"),("Json","*.json")])        
        config_string = self.instance.save_configuration()
        a = file.write(config_string)
        file.close()

    def load_config(self):
        file = askopenfile(initialfile = 'config.json', title = "Load configuration", defaultextension=".json",filetypes=[("All Files","*.*"),("Json","*.json")])        
        config_string = file.read();
        file.close()        
        self.instance.load_configuration(config_string)            
        self.configuration_updated()

    def update_device_tree(self):
        self.device_tree.delete(*self.device_tree.get_children())

        for connection_string in self.all_devices:
            device = self.all_devices[connection_string]
            device_info = device['device_info']
            used = device['enabled']
            name = device_info.name
            self.device_tree.insert('', tk.END, iid=connection_string, values=(yes_no[used], name, connection_string))

    def toggle_device(self, event):
        selected_items = self.device_tree.selection()
        if len(selected_items) < 1:
            return
            
        selected_item = selected_items[0]
        item = self.device_tree.item(selected_item)

        conn = item['values'][2]
        if not conn in self.all_devices:
            print("Something is wrong, device not found")
            return

        will_be_enabled = not self.all_devices[conn]['enabled']
        self.all_devices[conn]['enabled'] = True #will_be_enabled
        if will_be_enabled:
            self.all_devices[conn]['device'] = self.instance.add_device(conn)
        #else:
        #    self.all_devices[conn]['device'] = None

        self.update_device_tree()
        self.update_tree_widget()
        
    def configuration_updated(self):
        self.opendaq_renderer = None

        for function_block in self.instance.function_blocks:
            if function_block.function_block_type.id == 'ref_fb_module_renderer':
                self.opendaq_renderer = function_block

        self.update_tree_widget()
        

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Display openDAQ device configuration and plot values')
    parser.add_argument('--scale', help='UI scaling factor', type=int, default=1.0)
    parser.add_argument('--ip', help='IP address', type=str, default='')
    parser.add_argument('--demo', help='Include internal demo/reference devices', action='store_true')

    app = App(parser.parse_args())
    app.mainloop()
