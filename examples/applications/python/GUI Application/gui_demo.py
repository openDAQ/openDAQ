#!/usr/bin/env python

import argparse
import os
import enum
import sys
import platform

import tkinter as tk
from tkinter import ttk
import tkinter.font as tkfont
from tkinter.filedialog import asksaveasfile
from tkinter.filedialog import askopenfile
from tkinter.filedialog import askopenfilename
import opendaq as daq
from tkinter import messagebox

try:
    from ctypes import windll
    windll.shcore.SetProcessDpiAwareness(1)
except Exception:
    pass

try:
    from gui_demo.components.block_view import BlockView
    from gui_demo.components.properties_view import PropertiesView
    from gui_demo.components.add_device_dialog import AddDeviceDialog
    from gui_demo.components.add_server_dialog import AddServerDialog
    from gui_demo.components.add_function_block_dialog import AddFunctionBlockDialog
    from gui_demo.components.load_instance_config_dialog import LoadInstanceConfigDialog
    from gui_demo.app_context import AppContext
    from gui_demo import utils
    from gui_demo.event_port import EventPort
except Exception as e:
    from opendaq.gui_demo.components.block_view import BlockView
    from opendaq.gui_demo.components.properties_view import PropertiesView
    from opendaq.gui_demo.components.add_device_dialog import AddDeviceDialog
    from opendaq.gui_demo.components.add_server_dialog import AddServerDialog
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
    MODULES = 6
    UNSPECIFIED = 99

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
        elif index == 5:
            return DisplayType.MODULES
        return DisplayType.UNSPECIFIED

class ContextParams:
    module_path: str = ''
    discovery_servers: list = None

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
            
        if args.discovery_server:
            context_params.discovery_servers = [
                s.strip() for s in args.discovery_server.split(',') if s.strip()
            ]
        else:
            context_params.discovery_servers = []

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

        self.modules_map = {}

        self.title('openDAQ demo')
        self.geometry('{}x{}'.format(
            int(1500 * self.context.ui_scaling_factor * self.context.dpi_factor),
            int(800 * self.context.ui_scaling_factor * self.context.dpi_factor)))

        main_frame_top = ttk.Frame(self)
        main_frame_top.pack(fill=tk.X)

        self.menu_bar_create()

        add_device_button = ttk.Button(
            main_frame_top, text='Add device', command=self.handle_add_device_button_clicked)
        add_device_button.pack(side=tk.LEFT, padx=5)

        add_function_block_button = ttk.Button(
            main_frame_top, text='Add function block', command=self.handle_add_function_block_button_clicked)
        add_function_block_button.pack(side=tk.LEFT, padx=5)
        
        add_server_button = ttk.Button(
            main_frame_top, text='Add server', command=self.handle_add_server_button_clicked)
        add_server_button.pack(side=tk.LEFT, padx=5)

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
        nb.add(ttk.Frame(nb), text='Modules')
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
        treeview_rowheight = max(20, int(round(
            30 * self.context.ui_scaling_factor * self.context.dpi_factor)))
        style.configure('Treeview', rowheight=treeview_rowheight)

        style.configure('Treeview.Heading', font='Arial 10 bold')
        style.configure('Treeview.Column', padding=(
            5 * self.context.ui_scaling_factor))

        # Style for status text labels
        style.configure("StatusOk.TLabel",
                foreground="green",
                font=("TkDefaultFont", 10, "bold"))
        style.configure("StatusError.TLabel",
                    foreground="red",
                    font=("TkDefaultFont", 10, "bold"))
        style.configure("StatusWarning.TLabel",
                    foreground="goldenrod",
                    font=("TkDefaultFont", 10, "bold"))

        default_font = tkfont.nametofont('TkDefaultFont')
        default_font.configure(size=9 * self.context.ui_scaling_factor)

        self.context.load_icons(os.path.join(
            os.path.dirname(__file__), 'gui_demo', 'icons'))

        self.init_opendaq()

        if args.config != '':
            self._load_config(args.config)

        self.poll_opendaq_events()

    def poll_opendaq_events(self):
        try:
            daq.event_queue.process_events()
        except Exception as e:
            print("Callback processing error:", e)

        try:
            self.context.instance.context.scheduler.run_main_loop_iteration()
        except Exception as e:
            print("Scheduler processing error:", e)

        # Re-schedule after 50 ms
        self.after(20, self.poll_opendaq_events)

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
        file_menu.add_command(label='Load module',
                              command=self.handle_load_modules_button_clicked)
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
        tree.column('#0', width=int(350 * self.context.ui_scaling_factor * self.context.dpi_factor))
        # hide the column with unique id
        tree.column('#1', width=0, minwidth=0, stretch=False)

        # bind selection
        tree.bind('<<TreeviewSelect>>', self.handle_tree_select)
        tree.bind('<ButtonRelease-3>', self.handle_tree_right_button_release)
        tree.bind('<Button-3>', self.handle_tree_right_button)
        #tree.bind('<Double-1>', lambda e: 'break')
        tree.bind('<Button-1>', self.handle_tree_click)

        # add a scrollbar
        scroll_bar = ttk.Scrollbar(
            frame, orient=tk.VERTICAL, command=tree.yview)
        tree.configure(yscroll=scroll_bar.set)
        scroll_bar.pack(fill=tk.Y, side=tk.RIGHT)

        parent_frame.add(frame)
        tree.tag_configure('warning', foreground=utils.StatusColor.WARNING)
        tree.tag_configure('error', foreground=utils.StatusColor.ERROR)
        tree.tag_configure('inactive', foreground='gray')
        self.tree = tree

    def tree_update(self, new_selected_node=None):
        self.tree.delete(*self.tree.get_children())
        self.right_side_panel_clear()

        self.context.selected_node = new_selected_node

        if self.current_tab() == DisplayType.MODULES:
            self.modules_map = {}
            for mod in self.context.instance.module_manager.modules:
                info = mod.module_info
                
                if info is None:
                    continue
                
                mod_id = str(info.id)
                if not mod_id:
                    mod_id = f'__module_{len(self.modules_map)}__'

                display_name = str(info.name) if info.name else mod_id

                self.tree.insert('', tk.END, iid=mod_id,
                                 text=self._format_tree_item_text(display_name), open=False)
                self.modules_map[mod_id] = mod
            return

        self.tree_traverse_components_recursive(
            self.context.instance, self.current_tab())
        self.tree_restore_selection(
            self.context.selected_node)  # reset in case the selected node outdates
        self.set_node_update_status()
        self.set_node_lock_status()
        self.set_node_active_status()

    def tree_traverse_components_recursive(
            self, component, display_type=DisplayType.UNSPECIFIED, tree_parent_id=None):
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
        if tree_parent_id is not None:
            parent_id = tree_parent_id
        elif display_type not in (
                DisplayType.UNSPECIFIED, DisplayType.TOPOLOGY, DisplayType.SYSTEM_OVERVIEW,
                DisplayType.TOPOLOGY_CUSTOM_COMPONENTS, None) or component.parent is None:
            parent_id = ''
        else:
            parent_id = component.parent.global_id
        
        is_fb = daq.IFunctionBlock.can_cast_from(component)
        is_channel = daq.IChannel.can_cast_from(component)
            
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
                elif tree_parent_id is not None and is_fb:
                    self.tree_add_component(parent_id, daq.IFunctionBlock.cast_from(component))
            elif display_type == DisplayType.FUNCTION_BLOCKS:
                if daq.IFunctionBlock.can_cast_from(
                        component) and not daq.IChannel.can_cast_from(component):
                    self.tree_add_component(
                        parent_id, daq.IFunctionBlock.cast_from(component))

        if folder is not None and (self.context.view_hidden_components or folder.visible):
            if display_type == DisplayType.FUNCTION_BLOCKS and is_fb and not is_channel:
                for item in items:
                    self.tree_traverse_components_recursive(
                        item, display_type=display_type, tree_parent_id=component.global_id)
            elif display_type == DisplayType.CHANNELS and (is_channel or (tree_parent_id is not None and is_fb)):
                for item in items:
                    self.tree_traverse_components_recursive(
                        item, display_type=display_type, tree_parent_id=component.global_id)
            elif not (is_fb and display_type == DisplayType.FUNCTION_BLOCKS):
                for item in items:
                    self.tree_traverse_components_recursive(
                        item, display_type=display_type, tree_parent_id=tree_parent_id)

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
        component_name = self.get_component_tree_name(component)
        icon = self.context.icons['circle']
        skip = not self.context.view_hidden_components and not component.visible

        if daq.IChannel.can_cast_from(component):
            icon = self.context.icons['channel']
        elif daq.ISignal.can_cast_from(component):
            icon = self.context.icons['signal']
        elif daq.IFunctionBlock.can_cast_from(component):
            icon = self.context.icons['function_block']
        elif daq.IInputPort.can_cast_from(component):
            icon = self.context.icons['input_port']
        elif daq.IDevice.can_cast_from(component):
            icon = self.context.icons['device']
        elif daq.IFolder.can_cast_from(component):
            icon = self.context.icons['folder']
            component_name = self.get_component_tree_name(component)
        elif daq.ISyncComponent.can_cast_from(component):
            icon = self.context.icons['link']
        else:  # skipping unknown type components
            skip = not show_unknown

        if not skip:
            status_string = None
            try:
                status = component.status_container.get_status('ComponentStatus')
                if status == daq.Enumeration(daq.String('ComponentStatusType'), daq.String('Warning'), component.context.type_manager):
                    status_string = 'warning'
                elif status == daq.Enumeration(daq.String('ComponentStatusType'), daq.String('Error'), component.context.type_manager):
                    status_string = 'error'
            except:
                pass
            
            is_open = not daq.IFunctionBlock.can_cast_from(component)
            
            self.tree.insert(parent_node_id, tk.END, iid=component_node_id, image=icon,
                             text=self._format_tree_item_text(component_name), open=is_open, values=(component_node_id,), tags=(status_string,))



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

    def operation_mode_to_string(self, op_mode):
        if op_mode == daq.OperationModeType.Unknown:
            return '/'
        if op_mode == daq.OperationModeType.Idle:
            return 'Idle'
        if op_mode == daq.OperationModeType.Operation:
            return 'Operation'
        if op_mode == daq.OperationModeType.SafeOperation:
            return 'SafeOperation'
        return ''

    def get_component_tree_name(self, component):
        component_name = self.get_standard_folder_name(component.name)
        if daq.IDevice.can_cast_from(component):
            device = daq.IDevice.cast_from(component)
            mode = self.operation_mode_to_string(device.operation_mode)
            if mode:
                component_name = f'{component_name} | {mode}'
        return component_name

    def _build_component_state_labels(self, component, tags):
        labels = []

        # Component health state (for all components)
        if 'error' in tags:
            labels.append('err')
        elif 'warning' in tags:
            labels.append('warn')

        # Activity state (for all components)
        if component is not None and daq.IComponent.can_cast_from(component):
            try:
                if not daq.IComponent.cast_from(component).active:
                    labels.append('inactive')
            except Exception:
                pass

        # Device-only states
        if component is not None and daq.IDevice.can_cast_from(component):
            try:
                if not utils.is_device_connected(daq.IDevice.cast_from(component)):
                    labels.append('disconnected')
            except Exception:
                pass

        # Inherited lock state
        if 'locked' in tags:
            labels.append('locked')

        # Updating marker
        if 'selected' in tags:
            labels.append('*')

        return labels

    def _update_tree_item_visual_state(self, node):
        component = utils.find_component(node, self.context.instance)
        if component is None:
            return

        tags = set(self.tree.item(node, 'tags'))
        base_name = self.get_component_tree_name(component)
        labels = self._build_component_state_labels(component, tags)
        suffix = f" [{', '.join(labels)}]" if labels else ''
        self.tree.item(node, text=self._format_tree_item_text(base_name + suffix))

    def _format_tree_item_text(self, text):
        # Visual gap between icon and text in the tree item column.
        return f' {text}'

    def tree_restore_selection(self, old_node=None):
        desired_iid = old_node.global_id if old_node else ''
        current_iid = utils.treeview_get_first_selection(self.tree) or ''

        node = utils.find_component(desired_iid, self.context.instance)

        # if component is alive and in treeview
        if node and self.tree.exists(desired_iid):
            if desired_iid != current_iid:  # if component is not already selected
                self.tree.selection_set(desired_iid)
                self.tree.focus(desired_iid)
                self.tree.see(desired_iid)
        elif old_node and old_node.parent:  # try to select parent
            self.tree_restore_selection(old_node.parent)
        else:  # fallback
            self.tree.selection_set('')

    def right_side_panel_create(self, parent_frame):
        sframe = ttk.Frame(parent_frame)
        sframe.pack(fill=tk.BOTH, expand=True)

        self.right_side_panel = sframe
        self.right_side_canvas = None

    # MARK: - Add device dialog
    def add_device_dialog_show(self):
        dialog = AddDeviceDialog(self, self.context, None)
        dialog.show()

    # MARK: - Add function block dialog
    def add_function_block_dialog_show(self, component=None):
        dialog = AddFunctionBlockDialog(self, self.context, component)
        dialog.show()
        
    # MARK: - Add server dialog
    def add_server_dialog_show(self, component=None):
        dialog = AddServerDialog(self, self.context, component)
        dialog.show()

    # MARK: - Button handlers
    def handle_add_device_button_clicked(self):
        self.add_device_dialog_show()

    def handle_add_function_block_button_clicked(self):
        self.add_function_block_dialog_show()
        
    def handle_add_server_button_clicked(self):
        self.add_server_dialog_show()

    def handle_save_config_button_clicked(self):
        file = asksaveasfile(initialfile='config.json', title='Save configuration',
                             defaultextension='.json', filetypes=[('All Files', '*.*'), ('Json', '*.json')])
        if file is None:
            return
        config_string = self.context.instance.save_configuration()
        a = file.write(config_string)
        file.close()

    def handle_load_config_button_clicked(self):
        file = askopenfile(
            parent=self,
            title='Load configuration',
            defaultextension="json",
            filetypes=[('JSON', f'*.json')]
        )
        if file is None:
            return

        dialog = LoadInstanceConfigDialog(self, self.context, file)
        dialog.show()
        self.tree_update()

    def handle_load_modules_button_clicked(self):
        if platform.system() == 'Windows':
            extension = '.module.dll'
        else:
            extension = '.module.so'

        file_path = askopenfilename(
            parent=self,
            title='Load module',
            defaultextension=extension,
            filetypes=[('openDAQ module', f'*{extension}')]
        )

        if not file_path:
            return

        try:
            self.context.instance.module_manager.load_module(file_path)
            self.tree_update()
        except Exception as e:
            print('Load module failed:', e, file=sys.stderr)
            utils.show_error('Load module failed', str(e), self)

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
            
    def handle_tree_click(self, event):
        iid = self.tree.identify_row(event.y)
        element = self.tree.identify_element(event.x, event.y)

        if element == 'indicator':
            return

        if iid and iid == utils.treeview_get_first_selection(self.tree):
            self.tree.item(iid, open=not self.tree.item(iid, 'open'))
            return 'break'  # prevent <<TreeviewSelect>> from refiring unnecessarily

    def create_property_object_menu(self, node):
        popup = tk.Menu(self.tree, tearoff=0)

        popup.add_command(label='Begin update', command=self.handle_begin_update)
        popup.add_command(label='End update', command=self.handle_end_update)

        return popup

    def create_component_menu(self, node):
        return self.create_property_object_menu(node)

    def create_function_block_menu(self, node):
        popup = self.create_property_object_menu(node)

        if node.available_function_block_types:
            popup.add_command(
                label='Add Function block',
                command=lambda: self.add_function_block_dialog_show(node)
            )
        if not daq.IChannel.can_cast_from(node):
            popup.add_command(
                label='Remove',
                command=lambda: self.handle_tree_menu_remove_function_block(node)
            )

        return popup

    def create_device_menu(self, node):
        popup = self.create_property_object_menu(node)

        popup.add_command(label='Lock', command=self.handle_lock)
        popup.add_command(label='Unlock', command=self.handle_unlock)

        if node.available_function_block_types:
            popup.add_command(
                label='Add Function block',
                command=lambda: self.add_function_block_dialog_show(node)
            )


        if node.global_id != self.context.instance.global_id:
            popup.add_command(
                label='Remove',
                command=lambda: self.handle_tree_menu_remove_device(node)
            )

        return popup

    def handle_tree_right_button_release(self, event):
        iid = utils.treeview_get_first_selection(self.tree)

        node = None
        if iid:
            node = utils.find_component(iid, self.context.instance)

        popup = None
        if node:
            if daq.IFunctionBlock.can_cast_from(node):
                popup = self.create_function_block_menu(daq.IFunctionBlock.cast_from(node))
            elif daq.IDevice.can_cast_from(node):
                popup = self.create_device_menu(daq.IDevice.cast_from(node))

        if popup is None:
            popup = self.create_property_object_menu(node)

        try:
            popup.tk_popup(event.x_root, event.y_root, 0)
        finally:
            popup.grab_release()

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
            return daq.IFolder.cast_from(node)
        elif daq.ISignal.can_cast_from(node):
            return daq.ISignal.cast_from(node)

        return self.find_fb_device_folder(
            node.parent) if node is not None else None

    def right_side_panel_clear(self):
        for widget in list(self.right_side_panel.children.values()):
            widget.destroy()

    def right_side_panel_draw_node(self, node):
        if node is None:
            return

        found = self.find_fb_device_folder(
            node) if node.global_id not in self.context.custom_component_ids else node
        if found is None:
            return
        if not found.visible and not self.context.view_hidden_components:
            return

        block_view = BlockView(self.right_side_panel, found, self.context)
        block_view.pack(fill=tk.BOTH,  expand=True)

    # MARK: - Right hand side panel - MODULES
    def right_side_panel_draw_module(self, mod_id):
        if mod_id not in self.modules_map:
            return
        
        mod = self.modules_map[mod_id]
        
        self._draw_module_header(self.right_side_panel, mod)
        self._draw_module_type_columns(self.right_side_panel, mod)

    def _draw_module_header(self, frame, mod):
        info = mod.module_info
        vi = info.version_info

        name = str(info.name) if info.name else str(info.id)

        ttk.Label(frame, text=name,
                  font=("TkDefaultFont", 13, "bold")).pack(anchor=tk.W, padx=10, pady=(10, 5))

        if daq.IDevelopmentVersionInfo.can_cast_from(vi):
            dev_vi = daq.IDevelopmentVersionInfo.cast_from(vi)
            version_str = f"{dev_vi.major}.{dev_vi.minor}.{dev_vi.patch}.{dev_vi.tweak}"
            branch = dev_vi.branch_name
            hash_digest = dev_vi.hash_digest
        elif vi:
            version_str = f"{vi.major}.{vi.minor}.{vi.patch}"
            branch = hash_digest = None
        else:
            version_str = "N/A"
            branch = hash_digest = None

        fields = [("ID", str(info.id)), ("Version", version_str)]
        if branch:
            fields.append(("Branch", branch))
        if hash_digest:
            fields.append(("Hash", hash_digest))

        for label, value in fields:
            row = ttk.Frame(frame)
            row.pack(fill=tk.X, padx=10, pady=1)
            ttk.Label(row, text=f"{label}:", width=12, anchor=tk.W).pack(side=tk.LEFT)
            ttk.Label(row, text=str(value), anchor=tk.W).pack(side=tk.LEFT)

        ttk.Separator(frame, orient=tk.HORIZONTAL).pack(fill=tk.X, padx=10, pady=(10, 5))

    def _draw_module_type_columns(self, frame, mod):
        columns_frame = ttk.PanedWindow(frame, orient=tk.HORIZONTAL)
        columns_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

        left_frame = ttk.Frame(columns_frame)
        type_tree = ttk.Treeview(left_frame, show='tree', selectmode=tk.BROWSE)
        type_tree.column('#0', width=int(250 * self.context.dpi_factor))
        type_scroll = ttk.Scrollbar(left_frame, orient=tk.VERTICAL, command=type_tree.yview)
        type_tree.configure(yscrollcommand=type_scroll.set)
        type_tree.pack(fill=tk.BOTH, expand=True, side=tk.LEFT)
        type_scroll.pack(fill=tk.Y, side=tk.RIGHT)
        columns_frame.add(left_frame, weight=1)

        right_frame = ttk.Frame(columns_frame)
        columns_frame.add(right_frame, weight=1)

        def set_sash(event=None):
            w = columns_frame.winfo_width()
            if w > 1:
                columns_frame.sashpos(0, w // 2)
        columns_frame.bind('<Map>', set_sash)

        def _safe_get(getter):
            try:
                return getter()
            except (AttributeError, RuntimeError):
                return {}

        sections = [
            ("Device Types", _safe_get(lambda: mod.available_device_types), "device"),
            ("Function Block Types", _safe_get(lambda: mod.available_function_block_types), "function_block"),
            ("Server Types", _safe_get(lambda: mod.available_server_types), "server"),
            ("Streaming Types", _safe_get(lambda: mod.available_streaming_types), "streaming"),
        ]

        type_data_map = {}
        for section_name, types_dict, type_kind in sections:
            count = len(types_dict) if types_dict else 0
            section_id = type_tree.insert('', tk.END, text=f" {section_name} ({count})", open=False)
            if types_dict:
                for key, comp_type in types_dict.items():
                    ctype = daq.IComponentType.cast_from(comp_type)
                    iid = type_tree.insert(section_id, tk.END, text=f" {ctype.name or key}")
                    type_data_map[iid] = {"comp_type": comp_type, "type_kind": type_kind, "key": key}

        def on_type_selected(event):
            selected = utils.treeview_get_first_selection(type_tree)
            if selected is None or selected not in type_data_map:
                return
            for widget in right_frame.winfo_children():
                widget.destroy()
            self._draw_module_type_detail(right_frame, type_data_map[selected])

        type_tree.bind('<<TreeviewSelect>>', on_type_selected)

        def update_columns_height(event=None):
            if self.right_side_canvas is not None:
                self.right_side_canvas.update_idletasks()
                canvas_h = self.right_side_canvas.winfo_height()
                remaining = canvas_h - columns_frame.winfo_y() - 10
                if remaining > 100:
                    columns_frame.configure(height=remaining)

        if self.right_side_canvas is not None:
            self.right_side_canvas.bind('<Configure>', update_columns_height, add='+')
        columns_frame.after_idle(update_columns_height)

    def _draw_module_type_detail(self, frame, entry):
        comp_type = entry["comp_type"]
        type_kind = entry["type_kind"]
        key = entry["key"]
        ctype = daq.IComponentType.cast_from(comp_type)

        ttk.Label(frame, text=ctype.name or key,
                  font=("TkDefaultFont", 11, "bold")).pack(anchor=tk.W, padx=10, pady=(5, 2))

        if ctype.description:
            ttk.Label(frame, text=ctype.description, foreground="gray",
                      wraplength=400).pack(anchor=tk.W, padx=10, pady=(0, 5))

        info_fields = [("ID", ctype.id)]
        if type_kind == "device" and daq.IDeviceType.can_cast_from(comp_type):
            prefix = getattr(daq.IDeviceType.cast_from(comp_type), 'prefix', None)
            info_fields.append(("Prefix", prefix))  # the label row already handles None -> "N/A"
        elif type_kind == "streaming" and daq.IStreamingType.can_cast_from(comp_type):
            prefix = getattr(daq.IStreamingType.cast_from(comp_type), 'prefix', None)
            info_fields.append(("Prefix", prefix))

        for label, value in info_fields:
            row = ttk.Frame(frame)
            row.pack(fill=tk.X, padx=10, pady=1)
            ttk.Label(row, text=f"{label}:", width=12, anchor=tk.W).pack(side=tk.LEFT)
            ttk.Label(row, text=str(value) if value else "N/A", anchor=tk.W).pack(side=tk.LEFT)

        config = ctype.create_default_config()
        if config is not None and len(config.all_properties) > 0:
            ttk.Separator(frame, orient=tk.HORIZONTAL).pack(fill=tk.X, padx=10, pady=10)
            ttk.Label(frame, text="Default Configuration",
                      font=("TkDefaultFont", 10, "bold")).pack(anchor=tk.W, padx=10, pady=(0, 5))
            config_frame = ttk.Frame(frame, height=int(300 * self.context.dpi_factor))
            config_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
            config_frame.pack_propagate(False)
            PropertiesView(config_frame, config, self.context, read_only=True).pack(fill=tk.BOTH, expand=True)

    # MARK: - Tree view handlers

    def handle_tree_select(self, event):

        selected_iid = utils.treeview_get_first_selection(self.tree)
        if selected_iid is None:
            self.context.selected_node = None
            return
        
        if self.current_tab() == DisplayType.MODULES:
            self.right_side_panel_clear()
            self.right_side_panel_draw_module(selected_iid)
            return

        item = self.tree.item(selected_iid)
        # WA for IDs with spaces
        node_unique_id = ' '.join(str(val) for val in item['values'])
        if node_unique_id not in self.context.nodes:
            return
        node = self.context.nodes[node_unique_id]
        if (daq.IFolder.can_cast_from(node)
                and not daq.IDevice.can_cast_from(node)
                and not daq.IFunctionBlock.can_cast_from(node)):
            self.tree.item(selected_iid, open=not self.tree.item(selected_iid, 'open'))
            self.tree.selection_set('')
            return

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

    def add_tag_and_configure(self, node, tag, color):
        current_tags = set(self.tree.item(node, 'tags'))
        current_tags.add(tag)
        self.tree.tag_configure(tag, foreground=color)
        self.tree.item(node, tags=tuple(current_tags))

    def remove_tag(self, node, tag):
        current_tags = set(self.tree.item(node, 'tags'))
        if tag in current_tags:
            current_tags.remove(tag)
        self.tree.item(node, tags=tuple(current_tags))

    def _set_node_update_status_recursive(self, node):
        node_obj = utils.find_component(node, self.context.instance)
        if node_obj.updating:
            self.add_tag_and_configure(node, 'selected', 'red')
        else:
            self.remove_tag(node, 'selected')
        self._update_tree_item_visual_state(node)
        children = self.tree.get_children(node)
        for child in children:
            self._set_node_update_status_recursive(child)

    def set_node_lock_status(self):
        for node in self.tree.get_children():
            self._set_node_lock_status_recursive(node)

    def _set_node_lock_status_recursive(self, node, parent_locked=False):
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
            self.add_tag_and_configure(node, 'locked', 'gray')
        else:
            self.remove_tag(node, 'locked')
        self._update_tree_item_visual_state(node)

        children = self.tree.get_children(node)
        for child in children:
            self._set_node_lock_status_recursive(child, locked)

    def set_node_active_status(self):
        for node in self.tree.get_children():
            self._set_node_active_status_recursive(node)

    def _set_node_active_status_recursive(self, node):
        component = utils.find_component(node, self.context.instance)

        current_tags = set(self.tree.item(node, 'tags'))
        has_warning_or_error = 'warning' in current_tags or 'error' in current_tags

        is_inactive = False
        if component is not None and daq.IComponent.can_cast_from(component):
            try:
                is_inactive = not daq.IComponent.cast_from(component).active
            except Exception:
                is_inactive = False

        # warning/error color has higher priority
        if not has_warning_or_error and is_inactive:
            self.add_tag_and_configure(node, 'inactive', 'gray')
        else:
            self.remove_tag(node, 'inactive')
        self._update_tree_item_visual_state(node)

        for child in self.tree.get_children(node):
            self._set_node_active_status_recursive(child)

    def _load_config(self, config):
        file = open(config, 'r')
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
    parser.add_argument('-v', '--version', action='version',
        version=f'{os.path.dirname(__file__)} {daq.__dict__.get("__version__", "@VERSION@").replace("@VERSION@", "Unknown version")}')
    parser.add_argument(
        '--discovery_server', help='Discovery server protocols (comma-separated, e.g. "mdns")',
        type=str, default='mdns')

    app = App(parser.parse_args())
    app.mainloop()
