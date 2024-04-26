#!/usr/bin/env python

import argparse
import os

import tkinter as tk
from tkinter import ttk
import tkinter.font as tkfont
from tkinter.filedialog import asksaveasfile, askopenfile

import opendaq as daq

try:
    from ctypes import windll
    windll.shcore.SetProcessDpiAwareness(1)
except:
    pass

try:
    from gui_demo.components.block_view import BlockView
    from gui_demo.components.add_device_dialog import AddDeviceDialog
    from gui_demo.components.add_function_block_dialog import AddFunctionBlockDialog
    from gui_demo.app_context import *
    from gui_demo.utils import *
    from gui_demo.app_context import *
    from gui_demo.event_port import EventPort
except:
    from opendaq.gui_demo.components.block_view import BlockView
    from opendaq.gui_demo.components.add_device_dialog import AddDeviceDialog
    from opendaq.gui_demo.components.add_function_block_dialog import AddFunctionBlockDialog
    from opendaq.gui_demo.app_context import *
    from opendaq.gui_demo.utils import *
    from opendaq.gui_demo.app_context import *
    from opendaq.gui_demo.event_port import EventPort


class App(tk.Tk):

    # MARK: -- INIT
    def __init__(self, args):
        super().__init__()

        self.context = AppContext()
        self.event_port = EventPort(self, event_callback=self.on_refresh_event)

        self.context.ui_scaling_factor = int(args.scale)
        self.context.include_reference_devices = bool(args.demo)
        try:
            if args.connection_string != '':
                self.context.connection_string = args.connection_string
            else:
                self.context.connection_string = None
        except ValueError:
            self.context.connection_string = None

        self.title('openDAQ demo')
        self.geometry('{}x{}'.format(
            1400*self.context.ui_scaling_factor, 1000*self.context.ui_scaling_factor))

        main_frame_top = tk.Frame(self)
        main_frame_top.pack(fill=tk.constants.X)
        main_frame_top.configure(background='white')

        self.menu_bar_create()

        add_device_button = tk.Button(
            main_frame_top, text='Add device', command=self.handle_add_device_button_clicked)
        add_device_button.pack(side=tk.constants.LEFT, padx=5,
                               pady=5, fill=tk.constants.X)

        add_function_block_button = tk.Button(
            main_frame_top, text='Add function block', command=self.handle_add_function_block_button_clicked)
        add_function_block_button.pack(
            side=tk.constants.LEFT, padx=5, pady=5, fill=tk.constants.X)

        refresh_button = tk.Button(
            main_frame_top, text='Refresh', command=self.handle_refresh_button_clicked)
        refresh_button.pack(side=tk.constants.LEFT,
                            padx=5, pady=5, fill=tk.constants.X)

        main_frame_bottom = tk.Frame(self)
        main_frame_bottom.pack(fill=tk.constants.BOTH, expand=True)

        main_frame_navigator = tk.PanedWindow(
            main_frame_bottom, orient=tk.constants.HORIZONTAL)
        main_frame_navigator.pack_propagate(0)

        frame_navigator_for_properties = tk.Frame(
            main_frame_navigator)
        frame_navigator_for_properties.pack_propagate(False)

        self.tree_widget_create(main_frame_navigator)

        main_frame_navigator.add(frame_navigator_for_properties)

        main_frame_navigator.pack(side=tk.LEFT, expand=1, fill=tk.BOTH)

        self.frame_navigator_for_properties = frame_navigator_for_properties

        self.right_side_panel_create(frame_navigator_for_properties)

        # High DPI workaround for now
        ttk.Style().configure('Treeview', rowheight=30*self.context.ui_scaling_factor)

        default_font = tkfont.nametofont("TkDefaultFont")
        default_font.configure(size=9*self.context.ui_scaling_factor)

        self.context.load_icons(os.path.join(
            os.path.dirname(__file__), 'gui_demo', 'icons'))

        self.init_opendaq()

    def init_opendaq(self):

        # add the first device if connection string is provided once on start
        if self.context.connection_string != None:
            # also calls self.update_tree_widget()
            self.context.add_first_available_device()

        self.tree_update()

    # MARK: - Menu bar
    def menu_bar_create(self):
        menu_bar = tk.Menu(self)
        self.config(menu=menu_bar)

        file_menu = tk.Menu(menu_bar, tearoff=0)
        menu_bar.add_cascade(label='File', menu=file_menu)
        file_menu.add_command(label='Load configuration',
                              command=self.handle_load_config_button_clicked)
        file_menu.add_command(label='Save configuration',
                              command=self.handle_save_config_button_clicked)
        file_menu.add_separator()
        file_menu.add_command(label='Exit', command=self.quit)

        view_menu = tk.Menu(menu_bar, tearoff=0)
        menu_bar.add_cascade(label='View', menu=view_menu)
        view_menu.add_checkbutton(
            label='Show hidden components', command=self.handle_view_show_hidden_components)

    def handle_view_show_hidden_components(self):
        self.context.view_hidden_components = not self.context.view_hidden_components
        self.tree_update(self.context.selected_node)

    # MARK: - Tree view
    def tree_widget_create(self, parent_frame):
        frame = ttk.Frame(parent_frame)

        # define columns
        tree = ttk.Treeview(frame, columns=('name', 'hash'), displaycolumns=(
            'name'), show='tree', selectmode='browse')
        tree.pack(fill="both", expand=True, side="left")

        # layout
        tree.column('#0', width=350*self.context.ui_scaling_factor)
        # hide the column with unique id
        tree.column('#1', width=0, minwidth=0, stretch=False)

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
        popup.add_command(label="Close")
        self.tree_popup = popup

    def tree_update(self, new_selected_node=None):
        self.tree.delete(*self.tree.get_children())

        self.context.selected_node = new_selected_node

        self.tree_traverse_components_recursive(self.context.instance)
        self.context.selected_node = self.tree_restore_selection(
            self.context.selected_node)  # reset in case the selected node outdates

    def tree_traverse_components_recursive(self, component):
        if component is None:
            return

        folder = daq.IFolder.cast_from(
            component) if component and daq.IFolder.can_cast_from(component) else None

        # add item to tree if it is not a folder or non-empty folder
        if folder is None or folder.items:
            self.tree_add_component(
                '' if component.parent is None else component.parent.global_id, component)

        if folder is not None:
            for item in folder.items:
                self.tree_traverse_components_recursive(item)

    def tree_add_component(self, parent_node_id, component: daq.IComponent):
        component_node_id = component.global_id
        component_name = component.name
        icon = icon = self.context.icons['circle']
        skip = not self.context.view_hidden_components and not component.visible

        if daq.IChannel.can_cast_from(component):
            channel = daq.IChannel.cast_from(component)
            # component_name = channel.name
            icon = self.context.icons['channel']
        elif daq.ISignal.can_cast_from(component):
            signal = daq.ISignal.cast_from(component)
            # component_name = signal.name
            icon = self.context.icons['signal']
        elif daq.IFunctionBlock.can_cast_from(component):
            function_block = daq.IFunctionBlock.cast_from(component)
            # component_name = function_block.function_block_type.name
            icon = self.context.icons['function_block']
        elif daq.IInputPort.can_cast_from(component):
            input_port = daq.IInputPort.cast_from(component)
            # component_name = input_port.name
            icon = self.context.icons['input_port']
        elif daq.IDevice.can_cast_from(component):
            device = daq.IDevice.cast_from(component)
            # component_name = device.info.name
            icon = self.context.icons['device']
        elif daq.IFolder.can_cast_from(component):
            icon = self.context.icons['folder']
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
        else:  # skipping unknown type components
            skip = True

        if not skip:
            self.tree.insert(parent_node_id, tk.END, iid=component_node_id, image=icon,
                             text=component_name, open=True, values=(component_node_id))
        self.context.nodes[component_node_id] = component

    def tree_restore_selection(self, old_node=None):
        iid = '' if old_node is None else old_node.global_id
        node = find_component(iid, self.context.instance)

        # don't drop the focus if root node is selected
        if isinstance(old_node, daq.IInstance):
            node = old_node

        if node and self.tree.exists(node.global_id):
            self.tree.selection_set(iid)
            self.tree.focus(iid)
        else:
            node = None
        return node

    def right_side_panel_create(self, parent_frame):

        def canvas_on_configure(event):
            canvas.itemconfig(sframe_id, width=event.width)

        def inner_frame_on_configure(event):
            reqwidth, reqheight = sframe.winfo_reqwidth(), sframe.winfo_reqheight()
            canvas.config(scrollregion=f'0 0 {reqwidth} {reqheight}')

        def yview_wrapper(*args):
            moveto = float(args[1])
            moveto = moveto if moveto > 0 else 0.0
            return canvas.yview('moveto', moveto)

        frame = parent_frame
        canvas = tk.Canvas(frame)
        canvas.pack(fill=tk.BOTH, side=tk.LEFT, expand=True)

        canvas.xview_moveto(0)
        canvas.yview_moveto(0)

        scrollbar = ttk.Scrollbar(
            frame, orient=tk.VERTICAL, command=yview_wrapper)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        sframe = tk.Frame(canvas)
        sframe_id = canvas.create_window(0, 0, window=sframe, anchor=tk.NW)

        self.right_side_panel = sframe

        canvas.configure(yscrollcommand=scrollbar.set)
        canvas.bind('<Configure>', canvas_on_configure)
        sframe.bind('<Configure>', inner_frame_on_configure)

    # MARK: - Add device dialog
    def add_device_dialog_show(self):
        dialog = AddDeviceDialog(self, self.context, None)
        dialog.show()

    # MARK: - Add function block dialog
    def add_function_block_dialog_show(self):
        dialog = AddFunctionBlockDialog(self, self.context, None)
        dialog.show()

    # MARK: - Button handlers
    def handle_add_device_button_clicked(self):
        self.add_device_dialog_show()

    def handle_add_function_block_button_clicked(self):
        self.add_function_block_dialog_show()

    def handle_save_config_button_clicked(self):
        file = asksaveasfile(initialfile='config.json', title="Save configuration",
                             defaultextension=".json", filetypes=[("All Files", "*.*"), ("Json", "*.json")])
        if file is None:
            return
        config_string = self.context.instance.save_configuration()
        a = file.write(config_string)
        file.close()

    def handle_load_config_button_clicked(self):
        file = askopenfile(initialfile='config.json', title="Load configuration",
                           defaultextension=".json", filetypes=[("All Files", "*.*"), ("Json", "*.json")])
        if file is None:
            return
        config_string = file.read()
        file.close()
        self.context.instance.load_configuration(config_string)
        self.tree_update()

    def handle_refresh_button_clicked(self):
        self.tree_update(self.context.selected_node)

    # MARK: - Tree view handlers
    def handle_tree_right_button(self, event):
        iid = event.widget.identify_row(event.y)
        if iid:
            event.widget.selection_set(iid)
        else:
            event.widget.selection_set()

    def handle_tree_right_button_release(self, event):
        iid = treeview_get_first_selection(self.tree)

        self.tree_popup.entryconfig(
            'Remove', state='disabled', command=None
        )
        self.tree_popup.entryconfig(
            'Close', command=self.tree_popup.grab_release)

        if iid:
            node = find_component(iid, self.context.instance)
            if node:
                if daq.IFunctionBlock.can_cast_from(node):
                    self.tree_popup.entryconfig(
                        "Remove", state="normal", command=lambda: self.handle_tree_menu_remove_function_block(node))
                elif daq.IDevice.can_cast_from(node):
                    self.tree_popup.entryconfig(
                        "Remove", state="normal", command=lambda: self.handle_tree_menu_remove_device(node))
        try:
            self.tree_popup.tk_popup(event.x_root, event.y_root, 0)
        finally:
            self.tree_popup.grab_release()

    # MARK: - Right hand side panel

    def find_fb_or_device(self, node):
        if node is None:
            return None
        elif daq.IChannel.can_cast_from(node):
            return daq.IChannel.cast_from(node)
        elif daq.IFunctionBlock.can_cast_from(node):
            return daq.IFunctionBlock.cast_from(node)
        elif daq.IDevice.can_cast_from(node):
            return daq.IDevice.cast_from(node)
        else:
            if daq.IFolderConfig.can_cast_from(node):
                folder = daq.IFolderConfig.cast_from(node)
                print(
                    f'io folder {node} named: {folder.name} items: {folder.items}')
            return self.find_fb_or_device(node.parent)

    def right_side_panel_draw_node(self, node):
        if node is None:
            return
        found = self.find_fb_or_device(node)
        if found is None:
            return
        elif type(found) in (daq.IChannel, daq.IFunctionBlock):

            upper_nodes = list()

            if daq.IChannel.can_cast_from(found):  # traversing up IO folders
                current = found.parent
                while current is not None:
                    next_parent = current.parent
                    if daq.IFolder.can_cast_from(current):
                        if next_parent is not None and daq.IFolder.can_cast_from(next_parent):
                            if not daq.IDevice.can_cast_from(next_parent) and current.local_id != 'IO':
                                upper_nodes.append(
                                    daq.IFolder.cast_from(current))
                            else:
                                break
                    current = current.parent
            # traversing function blocks
            elif daq.IFunctionBlock.can_cast_from(found):
                current = found.parent
                while current is not None:
                    if daq.IFunctionBlock.can_cast_from(current):
                        upper_nodes.append(
                            daq.IFunctionBlock.cast_from(current))
                    current = current.parent

            for upper_node in reversed(upper_nodes):
                block_view = BlockView(
                    self.right_side_panel, upper_node, self.context)
                block_view.pack(fill=tk.X)

            def draw_sub_fbs(fb, level=0):
                if fb is None:
                    return

                if daq.IFunctionBlock.can_cast_from(fb):
                    fb = daq.IFunctionBlock.cast_from(fb)
                    b = BlockView(self.right_side_panel, fb,
                                  self.context, level == 0)
                    b.pack(fill=tk.X, padx=(5*level, 0))

                if fb.has_item('FB'):
                    fb_folder = fb.get_item('FB')
                    fb_folder = daq.IFolder.cast_from(fb_folder)
                    for fb in fb_folder.items:
                        draw_sub_fbs(fb, level+1)

            draw_sub_fbs(found)

        elif type(found) is daq.IDevice:
            block_view = BlockView(self.right_side_panel, found, self.context)
            block_view.handle_expand_toggle()
            block_view.pack(fill=tk.X)

    # MARK: - Tree view handlers

    def handle_tree_select(self, event):

        selected_item = treeview_get_first_selection(self.tree)
        if selected_item is None:
            self.context.selected_node = None
            return
        item = self.tree.item(selected_item)

        node_unique_id = item['values'][0]
        if not node_unique_id in self.context.nodes:
            return
        node = self.context.nodes[node_unique_id]
        self.context.selected_node = node

        for widget in self.right_side_panel.children.values():
            widget.pack_forget()

        self.right_side_panel_draw_node(node)

    def handle_tree_menu_remove_function_block(self, node):
        if node is None:
            return
        if not daq.IFunctionBlock.can_cast_from(node):
            return

        node = daq.IFunctionBlock.cast_from(node)

        device = get_nearest_device(node.parent)
        if device is None:
            return

        device.remove_function_block(node)
        self.context.selected_node = device
        self.tree_update(self.context.selected_node)

    def handle_tree_menu_remove_device(self, node):
        if type(node) is not daq.IDevice:
            node = daq.IDevice.cast_from(
                node) if daq.IDevice.can_cast_from(node) else None

        if not node:
            return

        if node.parent:
            parent_device = get_nearest_device(node.parent)

            parent_device.remove_device(node)

            # return initial state to connection mapped info
            device_info_id_mapped = self.context.all_devices[
                parent_device.global_id][node.global_id]['device_info']
            device_state_conn_mapped = self.context.all_devices[
                parent_device.global_id][device_info_id_mapped.connection_string]
            device_state_conn_mapped['enabled'] = False
            device_state_conn_mapped['device'] = None

            # removing id mapped info
            self.context.all_devices[parent_device.global_id].pop(
                node.global_id, None)
            self.context.connected_devices[parent_device.global_id].pop(
                node.global_id, None)

            self.context.selected_node = parent_device
        self.tree_update(self.context.selected_node)

    # MARK: - Other

    def on_refresh_event(self, event):
        print('APP: refresh event received')
        self.tree_update(self.context.selected_node)


# MARK: - Entry point
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
