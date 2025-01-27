#!/usr/bin/env python

import argparse
import os
import enum
import sys

import tkinter as tk
from tkinter import ttk
import tkinter.font as tkfont
from tkinter.filedialog import asksaveasfile
import opendaq as daq
from tkinter import messagebox

try:
    from ctypes import windll
    windll.shcore.SetProcessDpiAwareness(1)
except Exception:
    pass

try:
    from gui_demo.components.block_view import BlockView
    from gui_demo.components.add_device_dialog import AddDeviceDialog
    from gui_demo.components.add_function_block_dialog import AddFunctionBlockDialog
    from gui_demo.components.load_instance_config_dialog import LoadInstanceConfigDialog
    from gui_demo.app_context import AppContext
    from gui_demo import utils
    from gui_demo.event_port import EventPort
except Exception as e:
    from opendaq.gui_demo.components.block_view import BlockView
    from opendaq.gui_demo.components.add_device_dialog import AddDeviceDialog
    from opendaq.gui_demo.components.add_function_block_dialog import AddFunctionBlockDialog
    from opendaq.gui_demo.components.load_instance_config_dialog import LoadInstanceConfigDialog
    from opendaq.gui_demo.app_context import AppContext
    from opendaq.gui_demo import utils
    from opendaq.gui_demo.event_port import EventPort


class DisplayType(enum.Enum):
    SYSTEM_OVERVIEW = 0
    SIGNALS = 1
    CHANNELS = 2
    FUNCTION_BLOCKS = 3
    TOPOLOGY = 4
    TOPOLOGY_CUSTOM_COMPONENTS = 5
    UNSPECIFIED = 6

    def from_tab_index(index):
        if index == 0:
            return DisplayType.SYSTEM_OVERVIEW
        elif index == 1:
            return DisplayType.SIGNALS
        elif index == 2:
            return DisplayType.CHANNELS
        elif index == 3:
            return DisplayType.FUNCTION_BLOCKS
        elif index == 4:
            return DisplayType.TOPOLOGY
        return DisplayType.UNSPECIFIED
        
class ContextParams:
    module_path: str = ''

class App(tk.Tk):

    # MARK: -- INIT
    def __init__(self, args):
        super().__init__()
         
        context_params = ContextParams()
        
        try:
            if args.module_path != '':
                context_params.module_path = args.module_path
            else:
                context_params.module_path = None
        except ValueError:
            context_params.module_path = None
            
        self.context = AppContext(context_params)
        self.context.on_needs_refresh = lambda: self.on_refresh_event(None)
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
            1500 * self.context.ui_scaling_factor, 800 * self.context.ui_scaling_factor))

        main_frame_top = ttk.Frame(self)
        main_frame_top.pack(fill=tk.X)

        self.menu_bar_create()

        add_device_button = ttk.Button(
            main_frame_top, text='Add device', command=self.handle_add_device_button_clicked)
        add_device_button.pack(side=tk.LEFT, padx=5)

        add_function_block_button = ttk.Button(
            main_frame_top, text='Add function block', command=self.handle_add_function_block_button_clicked)
        add_function_block_button.pack(side=tk.LEFT, padx=5)

        refresh_button = ttk.Button(
            main_frame_top, text='Refresh', command=self.handle_refresh_button_clicked)
        refresh_button.pack(side=tk.LEFT, padx=5)

        main_frame_bottom = ttk.Frame(self)
        main_frame_bottom.pack(fill=tk.BOTH, expand=True)

        nb = ttk.Notebook(main_frame_bottom)
        nb.add(ttk.Frame(nb), text='System Overview')
        nb.add(ttk.Frame(nb), text='Signals')
        nb.add(ttk.Frame(nb), text='Channels')
        nb.add(ttk.Frame(nb), text='Function blocks')
        nb.add(ttk.Frame(nb), text='Full Topology')
        nb.bind('<<NotebookTabChanged>>', self.on_tab_change)
        nb.pack(fill=tk.X)
        self.nb = nb

        main_frame_navigator = ttk.PanedWindow(
            main_frame_bottom, orient=tk.HORIZONTAL)
        main_frame_navigator.pack_propagate(0)

        frame_navigator_for_properties = ttk.Frame(
            main_frame_navigator)

        self.tree_widget_create(main_frame_navigator)

        main_frame_navigator.add(frame_navigator_for_properties)

        main_frame_navigator.pack(side=tk.LEFT, expand=1, fill=tk.BOTH)

        self.frame_navigator_for_properties = frame_navigator_for_properties

        self.right_side_panel_create(frame_navigator_for_properties)

        # High DPI workaround for now
        style = ttk.Style()
        style.configure('Treeview', rowheight=30 *
                        self.context.ui_scaling_factor)

        style.configure('Treeview.Heading', font='Arial 10 bold')
        style.configure('Treeview.Column', padding=(
            5 * self.context.ui_scaling_factor))
        default_font = tkfont.nametofont('TkDefaultFont')
        default_font.configure(size=9 * self.context.ui_scaling_factor)

        self.context.load_icons(os.path.join(
            os.path.dirname(__file__), 'gui_demo', 'icons'))

        self.init_opendaq()
        
        if args.config != '':
            self._load_config(args.config)

    def init_opendaq(self):

        # add the first device if connection string is provided once on start
        if self.context.connection_string is not None:
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
        self.tree_update()

    # MARK: - Tree view
    def tree_widget_create(self, parent_frame):
        frame = ttk.Frame(parent_frame)

        # define columns
        tree = ttk.Treeview(frame, columns=('name', 'hash'), displaycolumns=(
            'name'), show='tree', selectmode=tk.BROWSE)
        tree.pack(fill=tk.BOTH, expand=True, side=tk.LEFT)

        # layout
        tree.column('#0', width=350 * self.context.ui_scaling_factor)
        # hide the column with unique id
        tree.column('#1', width=0, minwidth=0, stretch=False)

        # bind selection
        tree.bind('<<TreeviewSelect>>', self.handle_tree_select)
        tree.bind('<ButtonRelease-3>', self.handle_tree_right_button_release)
        tree.bind('<Button-3>', self.handle_tree_right_button)

        # add a scrollbar
        scroll_bar = ttk.Scrollbar(
            frame, orient=tk.VERTICAL, command=tree.yview)
        tree.configure(yscroll=scroll_bar.set)
        scroll_bar.pack(fill=tk.Y, side=tk.RIGHT)

        parent_frame.add(frame)
        self.tree = tree

        popup = tk.Menu(tree, tearoff=0)
        popup.add_command(label='Remove')
        popup.add_command(label='Begin update',
                          command=self.handle_begin_update)
        popup.add_command(label='End update', command=self.handle_end_update)
        popup.add_command(label='Lock', command=self.handle_lock)
        popup.add_command(label='Unlock', command=self.handle_unlock)
        popup.add_command(label='Add Function block', command=self.handle_add_function_block_button_clicked)
        self.tree_popup = popup

    def tree_update(self, new_selected_node=None):
        self.tree.delete(*self.tree.get_children())
        self.right_side_panel_clear()

        self.context.selected_node = new_selected_node

        self.tree_traverse_components_recursive(
            self.context.instance, self.current_tab())
        self.tree_restore_selection(
            self.context.selected_node)  # reset in case the selected node outdates
        self.set_node_update_status()
        self.set_node_lock_status()

    def tree_traverse_components_recursive(
            self, component, display_type=DisplayType.UNSPECIFIED):
        if component is None:
            return

        self.context.nodes[component.global_id] = component

        folder = daq.IFolder.cast_from(
            component) if component and daq.IFolder.can_cast_from(component) else None
        device = daq.IDevice.cast_from(
            component) if component and daq.IDevice.can_cast_from(component) else None
        items = folder.get_items(daq.AnySearchFilter(
        ) if self.context.view_hidden_components else None) if folder else []

        # tree view only in topology mode + parent exists
        parent_id = '' if display_type not in (
            DisplayType.UNSPECIFIED, DisplayType.TOPOLOGY, DisplayType.SYSTEM_OVERVIEW, DisplayType.TOPOLOGY_CUSTOM_COMPONENTS, None) or component.parent is None else component.parent.global_id

        if folder is None or items or display_type == DisplayType.TOPOLOGY_CUSTOM_COMPONENTS:
            if display_type in (DisplayType.UNSPECIFIED, DisplayType.TOPOLOGY,
                                DisplayType.TOPOLOGY_CUSTOM_COMPONENTS, None):
                self.tree_add_component(
                    parent_id, component, display_type == DisplayType.TOPOLOGY_CUSTOM_COMPONENTS)
            elif display_type == DisplayType.SYSTEM_OVERVIEW:
                if not (daq.IInputPort.can_cast_from(component) or daq.ISignal.can_cast_from(
                        component) or daq.IServer.can_cast_from(component)):
                    if not (daq.IFolder.can_cast_from(component)
                            and component.name in ('IP', 'Sig', 'Srv')):
                        self.tree_add_component(parent_id, component)
            elif display_type == DisplayType.SIGNALS and daq.ISignal.can_cast_from(component):
                self.tree_add_component(
                    parent_id, daq.ISignal.cast_from(component))
            elif display_type == DisplayType.CHANNELS:
                if daq.IChannel.can_cast_from(component):
                    self.tree_add_component(
                        parent_id, daq.IChannel.cast_from(component))
            elif display_type == DisplayType.FUNCTION_BLOCKS:
                if daq.IFunctionBlock.can_cast_from(
                        component) and not daq.IChannel.can_cast_from(component):
                    self.tree_add_component(
                        parent_id, daq.IFunctionBlock.cast_from(component))

        if folder is not None:
            if not (daq.IFunctionBlock.can_cast_from(component)
                    and display_type == DisplayType.FUNCTION_BLOCKS) and (self.context.view_hidden_components or folder.visible):
                for item in items:
                    self.tree_traverse_components_recursive(
                        item, display_type=display_type)

        if device is not None and display_type == DisplayType.TOPOLOGY:
            custom_components = device.custom_components
            for item in custom_components:
                if item.visible or self.context.view_hidden_components:
                    self.context.custom_component_ids.add(item.global_id)
                    self.tree_traverse_components_recursive(
                        item, display_type=DisplayType.TOPOLOGY_CUSTOM_COMPONENTS)

    def tree_add_component(self, parent_node_id,
                           component, show_unknown=False):
        component_node_id = component.global_id
        component_name = component.name
        icon = self.context.icons['circle']
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
            if not utils.is_device_connected(device):
                component_name = f'{component_name} [disconnected]'
            icon = self.context.icons['device']
        elif daq.IFolder.can_cast_from(component):
            icon = self.context.icons['folder']
            component_name = self.get_standard_folder_name(component_name)
        elif daq.ISyncComponent.can_cast_from(component):
            icon = self.context.icons['link']
            component_name = self.get_standard_folder_name(component_name)
        else:  # skipping unknown type components
            skip = not show_unknown

        if not skip:
            self.tree.insert(parent_node_id, tk.END, iid=component_node_id, image=icon,
                             text=component_name, open=True, values=(component_node_id,))

    def get_standard_folder_name(self, component):
        if component == 'Sig':
            component = 'Signals'
        elif component == 'FB':
            component = 'Function blocks'
        elif component == 'Dev':
            component = 'Devices'
        elif component == 'IP':
            component = 'Input ports'
        elif component == 'IO':
            component = 'Inputs/Outputs'
        elif component == 'Srv':
            component = 'Servers'
        return component

    def tree_restore_selection(self, old_node=None):
        desired_iid = old_node.global_id if old_node else ''
        current_iid = utils.treeview_get_first_selection(self.tree) or ''

        node = utils.find_component(desired_iid, self.context.instance)

        # if component is alive and in treeview
        if node and self.tree.exists(desired_iid):
            if desired_iid != current_iid:  # if component is not already selected
                self.tree.selection_set(desired_iid)
                self.tree.focus(desired_iid)
        elif old_node and old_node.parent:  # try to select parent
            self.tree_restore_selection(old_node.parent)
        else:  # fallback
            self.tree.selection_set('')

    def right_side_panel_create(self, parent_frame):

        def canvas_on_configure(event):
            canvas.itemconfig(sframe_id, width=event.width)

        def inner_frame_on_configure(event):
            reqwidth, reqheight = sframe.winfo_reqwidth(), sframe.winfo_reqheight()
            canvas.config(scrollregion=f'0 0 {reqwidth} {reqheight}')

        def yview_wrapper(*args):
            moveto = float(args[1])
            moveto = moveto if moveto > 0 else 0.0
            return canvas.yview(tk.MOVETO, moveto)

        frame = parent_frame
        canvas = tk.Canvas(frame)
        canvas.pack(fill=tk.BOTH, side=tk.LEFT, expand=True)

        canvas.xview_moveto(0)
        canvas.yview_moveto(0)

        scrollbar = ttk.Scrollbar(
            frame, orient=tk.VERTICAL, command=yview_wrapper)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        sframe = ttk.Frame(canvas)
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
    def add_function_block_dialog_show(self, component=None):
        dialog = AddFunctionBlockDialog(self, self.context, component)
        dialog.show()

    # MARK: - Button handlers
    def handle_add_device_button_clicked(self):
        self.add_device_dialog_show()

    def handle_add_function_block_button_clicked(self):
        self.add_function_block_dialog_show()

    def handle_save_config_button_clicked(self):
        file = asksaveasfile(initialfile='config.json', title='Save configuration',
                             defaultextension='.json', filetypes=[('All Files', '*.*'), ('Json', '*.json')])
        if file is None:
            return
        config_string = self.context.instance.save_configuration()
        a = file.write(config_string)
        file.close()

    def handle_load_config_button_clicked(self):
        dialog = LoadInstanceConfigDialog(self, self.context)
        dialog.show()
        self.tree_update()

    def handle_refresh_button_clicked(self):
        self.tree_update(self.context.selected_node)

    # MARK: - Tree view handlers
    def handle_tree_right_button(self, event):
        iid = event.widget.identify_row(event.y)
        if iid:
            selected_iid = utils.treeview_get_first_selection(
                event.widget) or ''
            if iid != selected_iid:
                event.widget.selection_set(iid)
        else:
            event.widget.selection_set()

    def handle_tree_right_button_release(self, event):
        iid = utils.treeview_get_first_selection(self.tree)

        self.tree_popup.entryconfig('Lock', state=tk.DISABLED)
        self.tree_popup.entryconfig('Unlock', state=tk.DISABLED)

        self.tree_popup.entryconfig(
            'Remove', state=tk.DISABLED, command=None
        )
        self.tree_popup.entryconfig(
            'Add Function block', state=tk.DISABLED)

        if iid:
            node = utils.find_component(iid, self.context.instance)
            if node:
                if daq.IFunctionBlock.can_cast_from(node):
                    node = daq.IFunctionBlock.cast_from(node)
                    if node.available_function_block_types:
                        self.tree_popup.entryconfig(
                            'Add Function block', state=tk.NORMAL, command=lambda: self.add_function_block_dialog_show(node))
                    if not daq.IChannel.can_cast_from(node):
                        self.tree_popup.entryconfig(
                            'Remove', state=tk.NORMAL, command=lambda: self.handle_tree_menu_remove_function_block(node))
                elif daq.IDevice.can_cast_from(node):
                    node = daq.IDevice.cast_from(node)
                    if node.available_function_block_types:
                        self.tree_popup.entryconfig(
                            'Add Function block', state=tk.NORMAL, command=lambda: self.add_function_block_dialog_show(node))

                    self.tree_popup.entryconfig('Lock', state=tk.ACTIVE)
                    self.tree_popup.entryconfig('Unlock', state=tk.ACTIVE)

                    if node.global_id != self.context.instance.global_id:
                        self.tree_popup.entryconfig(
                            'Remove', state=tk.NORMAL, command=lambda: self.handle_tree_menu_remove_device(node))
        try:
            self.tree_popup.tk_popup(event.x_root, event.y_root, 0)
        finally:
            self.tree_popup.grab_release()

    # MARK: - Right hand side panel

    def find_fb_device_folder(self, node):
        if daq.IChannel.can_cast_from(node):
            return daq.IChannel.cast_from(node)
        elif daq.IFunctionBlock.can_cast_from(node):
            return daq.IFunctionBlock.cast_from(node)
        elif daq.IDevice.can_cast_from(node):
            return daq.IDevice.cast_from(node)
        elif daq.ISyncComponent.can_cast_from(node):
            return daq.ISyncComponent.cast_from(node)
        elif daq.IFolder.can_cast_from(node):
            folder = daq.IFolder.cast_from(node)
            if folder.name not in AppContext.default_folders:
                return folder

        return self.find_fb_device_folder(
            node.parent) if node is not None else None

    def right_side_panel_clear(self):
        for widget in self.right_side_panel.children.values():
            widget.pack_forget()

    def right_side_panel_draw_node(self, node):
        if node is None:
            return

        found = self.find_fb_device_folder(
            node) if node.global_id not in self.context.custom_component_ids else node
        if found is None:
            return
        if not found.visible and not self.context.view_hidden_components:
            return
        if type(found) in (daq.IChannel, daq.IFunctionBlock, daq.IFolder):

            upper_nodes = list()
            parent_hidden = False

            current = found.parent
            while current is not None:
                if not current.visible and not self.context.view_hidden_components:
                    parent_hidden = True
                    break
                if daq.IDevice.can_cast_from(current):
                    break  # stop at device
                if daq.ISyncComponent.can_cast_from(current):
                    break  # stop at sync component
                if daq.IFolder.can_cast_from(current):
                    if current.local_id == 'IO':
                        break  # stop at IO folder
                    elif current.local_id == 'FB':
                        pass  # skip FB folder
                    else:
                        upper_nodes.append(
                            daq.IFolder.cast_from(current))
                elif daq.IFunctionBlock.can_cast_from(current):
                    upper_nodes.append(
                        daq.IFunctionBlock.cast_from(current))
                current = current.parent

            if parent_hidden:
                return

            for upper_node in reversed(upper_nodes):
                block_view = BlockView(
                    self.right_side_panel, upper_node, self.context)
                block_view.pack(fill=tk.X, padx=5, pady=5)

            def draw_sub_components(component, level=0):
                if component is None:
                    return

                if not component.visible and not self.context.view_hidden_components:
                    return

                if daq.IFunctionBlock.can_cast_from(component):
                    component = daq.IFunctionBlock.cast_from(component)
                    b = BlockView(self.right_side_panel, component,
                                  self.context, level == 0)
                    b.pack(fill=tk.X, padx=(5 + 10 * level, 5), pady=5)
                    if component.has_item('FB'):
                        fb_folder = component.get_item('FB')
                        fb_folder = daq.IFolder.cast_from(fb_folder)
                        for component in fb_folder.items:
                            draw_sub_components(component, level + 1)
                elif daq.IFolder.can_cast_from(component):
                    component = daq.IFolder.cast_from(component)
                    if component.name not in AppContext.default_folders:
                        b = BlockView(self.right_side_panel, component,
                                      self.context, level == 0)
                        b.pack(fill=tk.X, padx=(5 + 10 * level, 5), pady=5)

                        for item in component.items:
                            draw_sub_components(item, level + 1)

            draw_sub_components(found)

        elif type(found) in (daq.IDevice, daq.IComponent, daq.ISyncComponent):
            block_view = BlockView(self.right_side_panel, found, self.context)
            block_view.handle_expand_toggle()
            block_view.pack(fill=tk.X, padx=5, pady=5)

    # MARK: - Tree view handlers

    def handle_tree_select(self, event):

        selected_iid = utils.treeview_get_first_selection(self.tree)
        if selected_iid is None:
            self.context.selected_node = None
            return
        item = self.tree.item(selected_iid)
        # WA for IDs with spaces
        node_unique_id = ' '.join(str(val) for val in item['values'])
        if node_unique_id not in self.context.nodes:
            return
        node = self.context.nodes[node_unique_id]
        self.context.selected_node = node

        self.right_side_panel_clear()
        self.right_side_panel_draw_node(node)

    def handle_tree_menu_remove_function_block(self, node):
        if node is None:
            return
        if not daq.IFunctionBlock.can_cast_from(node):
            return

        node = daq.IFunctionBlock.cast_from(node)

        # searching nearest fb up the tree
        # if no parent fb found, then trying to remove from nearest parent device
        device = utils.get_nearest_device(node.parent, self.context.instance)
        parent_fb = utils.get_nearest_fb(node.parent, device)

        parent_fb.remove_function_block(node)
        self.context.selected_node = parent_fb
        self.tree_update(self.context.selected_node)

    def handle_tree_menu_remove_device(self, node):
        if type(node) is not daq.IDevice:
            node = daq.IDevice.cast_from(
                node) if daq.IDevice.can_cast_from(node) else None

        if not node:
            return
        parent = node.parent
        self.context.remove_device(node)

        self.context.selected_node = parent
        self.tree_update(self.context.selected_node)

    # MARK: - Other

    def on_refresh_event(self, event):
        self.tree_update(self.context.selected_node)

    def on_tab_change(self, event):
        self.tree_update(self.context.selected_node)

    def current_tab(self):
        return DisplayType.from_tab_index(self.nb.index(
            'current')) if self.nb is not None else DisplayType.UNSPECIFIED

    def handle_begin_update(self):
        selected_item = utils.treeview_get_first_selection(self.tree)
        if selected_item:
            self.begin_update_on_node(selected_item)
            self.set_node_update_status()
            self.tree_update(self.context.selected_node)

    def handle_end_update(self):
        selected_item = utils.treeview_get_first_selection(self.tree)
        if selected_item:
            self.end_update_on_node(selected_item)
            self.set_node_update_status()
            self.tree_update(self.context.selected_node)

    def begin_update_on_node(self, node):
        node_obj = utils.find_component(node, self.context.instance)
        node_obj = daq.IPropertyObject.cast_from(node_obj)
        node_obj.begin_update()

    def end_update_on_node(self, node):
        node_obj = utils.find_component(node, self.context.instance)
        node_obj = daq.IPropertyObject.cast_from(node_obj)
        try:
            node_obj.end_update()
        except RuntimeError:
            pass

    def handle_lock(self):
        node = utils.treeview_get_first_selection(self.tree)
        component = utils.find_component(node, self.context.instance)

        try:
            device = daq.IDevice.cast_from(component)
            device.lock()
            self._set_node_lock_status_recursive(node)
        except Exception as e:
            utils.show_error('Lock failed', f'{component.name}: {e}', self)
            print(f'Lock failed: {str(e)}', file=sys.stderr)

    def handle_unlock(self):
        node = utils.treeview_get_first_selection(self.tree)
        component = utils.find_component(node, self.context.instance)

        try:
            device = daq.IDevice.cast_from(component)
            device.unlock()
            self._set_node_lock_status_recursive(node)
        except Exception as e:
            print(f'Unlock failed: {str(e)}', file=sys.stderr)
            msg = str(e) + '. Do you want to forcefully unlock the device?'
            do_force_unlock = messagebox.askyesno('Unlock failed', msg)
            if do_force_unlock:
                self._force_unlock_device(node, component)

    def _force_unlock_device(self, node, component):
        try:
            device_private = daq.IDevicePrivate.cast_from(component)
            device_private.force_unlock()
            self._set_node_lock_status_recursive(node)
        except Exception as e:
            print('Force unlock failed: ', e, file=sys.stderr)
            utils.show_error('Force unlock failed', str(e), self)

    def set_node_update_status(self):
        for node in self.tree.get_children():
            self._set_node_update_status_recursive(node)

    def _set_node_update_status_recursive(self, node):
        color = 'red'
        node_obj = utils.find_component(node, self.context.instance)
        node_text = self.get_standard_folder_name(node_obj.name)
        if node_obj.updating:
            self.tree.item(node, tags=('selected',),
                           text=node_text + ' [*]')
            self.tree.tag_configure('selected', foreground=color)
        else:
            self.tree.item(node, tags=())
        children = self.tree.get_children(node)
        for child in children:
            self._set_node_update_status_recursive(child)

    def set_node_lock_status(self):
        for node in self.tree.get_children():
            self._set_node_lock_status_recursive(node)

    def _set_node_lock_status_recursive(self, node, parent_locked=False):
        color = 'gray'
        component = utils.find_component(node, self.context.instance)

        if daq.IDevice.can_cast_from(component):
            device = daq.IDevice.cast_from(component)
            try:
                locked = device.locked
            except:
                locked = False
        else:
            locked = parent_locked

        if locked:
            self.tree.item(node, tags=('locked',))
            self.tree.tag_configure('locked', foreground=color)
        else:
            self.tree.item(node, tags=())

        children = self.tree.get_children(node)
        for child in children:
            self._set_node_lock_status_recursive(child, locked)
            
    def _load_config(self, config):
        file = open(config, "r")
        if file is None:
            return
        config_string = file.read()
        file.close()

        updata_params = daq.UpdateParameters()
        self.context.instance.load_configuration(
            config_string, updata_params)


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
    parser.add_argument(
        '--config', help='Saved config', type=str, default='')
    parser.add_argument(
        '--module_path', help='Additional modules path', type=str, default='')

    app = App(parser.parse_args())
    app.mainloop()
