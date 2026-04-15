import tkinter as tk
from tkinter import ttk

import opendaq as daq
from .properties_view import PropertiesView

from .. import utils
from ..app_context import AppContext
from ..event_port import EventPort
from .dialog import Dialog


class AddFunctionBlockDialog(Dialog):
    def __init__(self, parent, context, selected_component = None, **kwargs):
        Dialog.__init__(self, parent, 'Add function block', context, **kwargs)
        # send events to parent window
        self.event_port = EventPort(self.parent)
        self.parent_component = selected_component

        self.geometry('{}x{}'.format(
            int(900 * self.context.ui_scaling_factor * self.context.dpi_factor),
            int(400 * self.context.ui_scaling_factor * self.context.dpi_factor)))

        # parent

        parent_device_tree_frame = ttk.Frame(self)
        parent_device_tree = ttk.Treeview(parent_device_tree_frame)

        parent_device_scroll_bar = ttk.Scrollbar(
            parent_device_tree_frame, orient=tk.VERTICAL, command=parent_device_tree.yview)
        parent_device_tree.configure(
            yscrollcommand=parent_device_scroll_bar.set)
        parent_device_scroll_bar.pack(side=tk.RIGHT, fill=tk.Y)

        parent_device_tree.heading('#0', text='Parent', anchor=tk.W)

        parent_device_tree.column(
            '#0', anchor=tk.W, minwidth=int(200 * self.context.dpi_factor), stretch=True)

        parent_device_tree.bind('<<TreeviewSelect>>',
                                self.handle_parent_device_selected)
        parent_device_tree.pack(fill=tk.BOTH, expand=True)

        parent_device_tree_frame.grid(row=0, column=0, sticky=tk.NSEW)

        # child

        tree_frame = ttk.Frame(self)
        tree = ttk.Treeview(tree_frame, columns=('id', 'name'), displaycolumns=(
            'id', 'name'), show='tree headings', selectmode=tk.BROWSE)
        scroll_bar = ttk.Scrollbar(
            tree_frame, orient=tk.VERTICAL, command=tree.yview)
        tree.configure(yscrollcommand=scroll_bar.set)
        scroll_bar.pack(side=tk.RIGHT, fill=tk.Y)

        # define headings
        tree.heading('id', text='TypeId', anchor=tk.W)
        tree.heading('name', text='Name', anchor=tk.W)

        # layout
        tree.column('#0', width=0, stretch=tk.NO)
        dpi = self.context.dpi_factor
        tree.column('id', anchor=tk.W, minwidth=int(200 * dpi), width=int(300 *
                    self.context.ui_scaling_factor * dpi), stretch=tk.NO)
        tree.column('name', anchor=tk.W, minwidth=int(200 * dpi), width=int(300 *
                    self.context.ui_scaling_factor * dpi))

        # bind double-click and right-click
        tree.bind('<Double-1>', self.handle_fb_tree_double_click)
        tree.bind('<Button-3>', self.handle_right_click)
        tree.bind('<<TreeviewSelect>>', self.handle_fb_type_selected)

        tree.pack(fill=tk.BOTH, expand=True)

        tree_frame.grid(row=0, column=1, sticky=tk.NSEW)

        self.device_tree = parent_device_tree
        self.fb_tree = tree

        self.grid_rowconfigure(0, weight=1)
        self.grid_columnconfigure(0, weight=1)
        self.grid_columnconfigure(1, weight=2)
        self.grid_columnconfigure((0, 1), uniform='uniform')

        actions_row = ttk.Frame(tree_frame)
        self._keep_open_var = tk.BooleanVar(value=False)
        ttk.Checkbutton(actions_row, text='Keep open after adding',
                        variable=self._keep_open_var).pack(side=tk.LEFT)
        self._fb_config_btn = ttk.Button(actions_row, text='Add with config\u2026',
                                         command=lambda: self.handle_button(True), state=tk.DISABLED)
        self._fb_config_btn.pack(side=tk.RIGHT, padx=(5, 0))
        ttk.Button(actions_row, text='Add',
                   command=lambda: self.handle_button(False)).pack(side=tk.RIGHT)
        actions_row.pack(fill=tk.X, pady=(4, 0))

    def initial_update(self):
        self.update_dialog()

    def update_dialog(self):
        self.update_parent_devices(
            self.device_tree, '', self.context.instance)
        parent_to_select = self.parent_component.global_id if self.parent_component is not None else self.context.instance.global_id
        self.select_parent_device(parent_to_select)

    def select_parent_device(self, device_id: str):
        if self.device_tree.exists(device_id):
            self.device_tree.selection_set(device_id)

    def update_parent_devices(self, tree, parent_id, component):
        tree.delete(*tree.get_children())

        def traverse_devices_recursive(tree, parent_id, component):
            if component is None:
                return

            if daq.IDevice.can_cast_from(component):
                device = daq.IDevice.cast_from(component)
                tree.insert(parent_id, tk.END, text=device.name,
                            iid=device.global_id, open=tk.TRUE)
                parent_id = device.global_id

            if daq.IFunctionBlock.can_cast_from(component):
                function_block = daq.IFunctionBlock.cast_from(component)
                if function_block.available_function_block_types:
                    tree.insert(parent_id, tk.END, text=function_block.name,
                                iid=function_block.global_id, open=tk.TRUE)
                    parent_id = function_block.global_id

            if daq.IFolder.can_cast_from(component):
                folder = daq.IFolder.cast_from(component)
                for item in folder.items:
                    traverse_devices_recursive(tree, parent_id, item)

        traverse_devices_recursive(tree, parent_id, component)

    def update_function_blocks(self):
        self.fb_tree.delete(*self.fb_tree.get_children())

        available_function_block_types = self.parent_component.available_function_block_types
        for function_block_id in available_function_block_types:
            self.fb_tree.insert('', tk.END, iid=function_block_id, values=(
                function_block_id,
                daq.IFunctionBlockType.cast_from(available_function_block_types[function_block_id]).name))

    def handle_parent_device_selected(self, event):
        selected_item = utils.treeview_get_first_selection(
            self.device_tree)
        if selected_item is None:
            return

        parent_component = utils.find_component(
            selected_item, self.context.instance)
        if parent_component is not None:
            if daq.IDevice.can_cast_from(parent_component) or daq.IFunctionBlock.can_cast_from(parent_component):
                self.parent_component = daq.IDevice.cast_from(parent_component) if daq.IDevice.can_cast_from(
                    parent_component) else daq.IFunctionBlock.cast_from(parent_component)
                self.update_function_blocks()

    def handle_fb_type_selected(self, e=None):
        can_config = False
        selected = utils.treeview_get_first_selection(self.fb_tree)
        if selected and self.parent_component:
            try:
                fb_id = self.fb_tree.item(selected)['values'][0]
                fb_type = self.parent_component.available_function_block_types[fb_id]
                cfg = daq.IComponentType.cast_from(fb_type).create_default_config()
                can_config = len(cfg.all_properties) > 0
            except Exception:
                pass
        self._fb_config_btn.configure(state=tk.NORMAL if can_config else tk.DISABLED)

    def handle_right_click(self, event):
        utils.treeview_select_item(self.fb_tree, event)

        can_config = False
        selected = utils.treeview_get_first_selection(self.fb_tree)
        if selected and self.parent_component:
            try:
                fb_id = self.fb_tree.item(selected)['values'][0]
                fb_type = self.parent_component.available_function_block_types[fb_id]
                cfg = daq.IComponentType.cast_from(fb_type).create_default_config()
                can_config = len(cfg.all_properties) > 0
            except Exception:
                pass

        menu = tk.Menu(self, tearoff=0)
        menu.add_command(label='Add', command=lambda: self.handle_button(False))
        menu.add_command(label='Add with config',
                         command=lambda: self.handle_button(True),
                         state=tk.NORMAL if can_config else tk.DISABLED)
        menu.tk_popup(event.x_root, event.y_root)

    def handle_button(self, config: bool):
        parent_top = self.parent.winfo_toplevel()
        self.configure(cursor='watch')
        parent_top.configure(cursor='watch')
        # Defer work so Tkinter finishes the current event (repaint cursor) before blocking
        self.after(0, lambda: self._run_handle_button(config, parent_top))

    def _run_handle_button(self, config: bool, parent_top):
        self.add_fb(config)
        # Restore cursor if dialog is still open (error, config window opened, or cancel)
        try:
            self.configure(cursor='')
        except tk.TclError:
            pass
        try:
            parent_top.configure(cursor='')
        except tk.TclError:
            pass

    def handle_fb_tree_double_click(self, dummy):
        self.handle_button(False)

    def add_fb(self, open_config_dialog: bool):
        selected_item = utils.treeview_get_first_selection(self.fb_tree)
        if selected_item is None:
            return

        item = self.fb_tree.item(selected_item)
        function_block_id = item['values'][0]

        if not open_config_dialog:
            self.execute_add_fb(function_block_id)
            return

        fb_type = self.parent_component.available_function_block_types[function_block_id]
        component_type = daq.IComponentType.cast_from(fb_type)
        configuration = component_type.create_default_config()

        if len(configuration.all_properties) == 0:
            utils.show_error('Empty configuration', f'Selected function block type has no properties to configure.', self)
            return

        # Open configuration editor window
        win = tk.Toplevel(self)
        win.withdraw()  # hide until centered
        win.title('Function Block configuration')
        win.attributes("-topmost", True)
        win.transient(self)

        frame = tk.Frame(win)
        frame.pack(fill=tk.BOTH, expand=True)

        def apply():
            parent_top = self.parent.winfo_toplevel()
            win.destroy()
            self.configure(cursor='watch')
            parent_top.configure(cursor='watch')
            self.after(0, lambda: self.execute_add_fb(function_block_id, configuration))

        tree = PropertiesView(frame, configuration, self.context)
        tree.pack(fill=tk.BOTH, expand=True)

        ttk.Button(win, text="Add", command=apply).pack(side=tk.BOTTOM, anchor=tk.E, padx=5, pady=5)

        # Center over the main window
        win.update_idletasks()
        dpi = self.context.dpi_factor
        w, h = int(600 * dpi), int(400 * dpi)
        main = self.parent.winfo_toplevel()
        x = main.winfo_rootx() + main.winfo_width() // 2 - w // 2
        y = main.winfo_rooty() + main.winfo_height() // 2 - h // 2
        win.geometry(f'{w}x{h}+{x}+{y}')
        win.deiconify()

    def execute_add_fb(self, fb_id, config=None):
        parent_top = self.parent.winfo_toplevel()
        self.configure(cursor='watch')
        parent_top.configure(cursor='watch')
        self.update_idletasks()
        new_fb = None
        try:
            new_fb = self.parent_component.add_function_block(fb_id, config)
        except Exception as e:
            self.configure(cursor='')
            parent_top.configure(cursor='')
            utils.show_error('Error adding function block', f'{fb_id}: {str(e)}', self)
            return
        parent_top.configure(cursor='')
        self.context.selected_node = new_fb
        self.event_port.emit()
        if self._keep_open_var.get():
            self.update_dialog()
            self.update_function_blocks()
        else:
            self.close()
