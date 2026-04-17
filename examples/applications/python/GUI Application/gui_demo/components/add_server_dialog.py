import tkinter as tk
from tkinter import ttk

import opendaq as daq
from .properties_view import PropertiesView

from .. import utils
from ..app_context import AppContext
from ..event_port import EventPort
from .dialog import Dialog


class AddServerDialog(Dialog):
    def __init__(self, parent, context, selected_component=None, **kwargs):
        Dialog.__init__(self, parent, 'Add server', context, **kwargs)
        # send events to parent window
        self.event_port = EventPort(self.parent)
        self.parent_component = selected_component

        self.geometry('{}x{}'.format(
            int(900 * self.context.ui_scaling_factor * self.context.dpi_factor),
            int(400 * self.context.ui_scaling_factor * self.context.dpi_factor)))

        # child

        tree_frame = ttk.Frame(self)
        tree = ttk.Treeview(tree_frame, columns=('name', 'description','id'), displaycolumns=(
            'name', 'description','id'), show='tree headings', selectmode=tk.BROWSE)
        scroll_bar = ttk.Scrollbar(
            tree_frame, orient=tk.VERTICAL, command=tree.yview)
        tree.configure(yscrollcommand=scroll_bar.set)
        scroll_bar.pack(side=tk.RIGHT, fill=tk.Y)

        # define headings
        tree.heading('name', text='Name', anchor=tk.W)
        tree.heading('description', text='Description', anchor=tk.W)
        tree.heading('id', text='Id', anchor=tk.W)

        tree.column('#0', width=0, stretch=tk.NO)
        dpi = self.context.dpi_factor
        tree.column('name', anchor=tk.W, minwidth=int(200 * dpi), width=int(300 *
                    self.context.ui_scaling_factor * dpi), stretch=tk.NO)
        tree.column('description', anchor=tk.W, minwidth=int(200 * dpi), width=int(300 *
                    self.context.ui_scaling_factor * dpi))
        tree.column('id', anchor=tk.W, minwidth=int(200 * dpi), width=int(300 *
                    self.context.ui_scaling_factor * dpi), stretch=tk.NO)

        # bind double-click and right-click
        tree.bind('<Double-1>', self.handle_server_tree_double_click)
        tree.bind('<Button-3>', self.handle_right_click)
        tree.bind('<<TreeviewSelect>>', self.handle_server_type_selected)

        tree.pack(fill=tk.BOTH, expand=True)

        tree_frame.grid(row=0, column=0, sticky=tk.NSEW)

        self.server_tree = tree

        self.grid_rowconfigure(0, weight=1)
        self.grid_columnconfigure(0, weight=1)

        actions_row = ttk.Frame(tree_frame)
        self._keep_open_var = tk.BooleanVar(value=False)
        ttk.Checkbutton(actions_row, text='Keep open after adding',
                        variable=self._keep_open_var).pack(side=tk.LEFT)
        self._enable_discovery_var = tk.BooleanVar(value=True)
        ttk.Checkbutton(actions_row, text='Enable discovery',
                        variable=self._enable_discovery_var).pack(side=tk.LEFT, padx=(10, 0))
        self._server_config_btn = ttk.Button(actions_row, text='Add with config\u2026',
                                              command=lambda: self.handle_button(True), state=tk.DISABLED)
        self._server_config_btn.pack(side=tk.RIGHT, padx=(5, 0))
        ttk.Button(actions_row, text='Add',
                   command=lambda: self.handle_button(False)).pack(side=tk.RIGHT)
        actions_row.pack(fill=tk.X, pady=(4, 0))

    def initial_update(self):
        self.update_server_types()

    def update_server_types(self):
            self.server_tree.delete(*self.server_tree.get_children())

            available_server_types = self.context.instance.available_server_types
            for server_type_id in available_server_types:
                server_type = daq.IServerType.cast_from(available_server_types[server_type_id])
                self.server_tree.insert('', tk.END, iid=server_type_id,
                                        values=(server_type.name, server_type.description, server_type_id))

    def handle_server_type_selected(self, e=None):
        can_config = False
        selected = utils.treeview_get_first_selection(self.server_tree)

        if selected:
            server_id = self.server_tree.item(selected)['values'][2]
            can_config = self._is_server_configurable(server_id)

        self._server_config_btn.configure(state=tk.NORMAL if can_config else tk.DISABLED)

    def handle_right_click(self, event):
        utils.treeview_select_item(self.server_tree, event)

        can_config = False
        selected = utils.treeview_get_first_selection(self.server_tree)
        if selected:
            server_id = self.server_tree.item(selected)['values'][2]
            can_config = self._is_server_configurable(server_id)

        menu = tk.Menu(self, tearoff=0)
        menu.add_command(label='Add', command=lambda: self.handle_button(False))
        menu.add_command(label='Add with config',
                         command=lambda: self.handle_button(True),
                         state=tk.NORMAL if can_config else tk.DISABLED)
        menu.tk_popup(event.x_root, event.y_root)
        
    def _is_server_configurable(self, server_id):
        available_types = self.context.instance.available_server_types
        if server_id not in available_types:
            return False

        server_type = available_types[server_id]

        if daq.IComponentType.can_cast_from(server_type):
            comp_type = daq.IComponentType.cast_from(server_type)
            config = comp_type.create_default_config()
            return len(config.all_properties) > 0

        return False

    def handle_button(self, config: bool):
        parent_top = self.parent.winfo_toplevel()
        self.configure(cursor='watch')
        parent_top.configure(cursor='watch')
        # Defer so Tkinter finishes the current event (cursor repaint) before blocking
        self.after(0, lambda: self._run_handle_button(config, parent_top))

    def _run_handle_button(self, config: bool, context_top):
        self.add_server(config)
        # Restore cursor if dialog is still open (error, config window opened, or cancel)
        try:
            self.configure(cursor='')
        except tk.TclError:
            pass
        try:
            context_top.configure(cursor='')
        except tk.TclError:
            pass

    def handle_server_tree_double_click(self, dummy):
        self.handle_button(False)

    def add_server(self, open_config_dialog: bool):
        selected_item = utils.treeview_get_first_selection(self.server_tree)
        if selected_item is None:
            return

        item = self.server_tree.item(selected_item)
        server_type_id = item['values'][2]

        if not open_config_dialog:
            self.execute_add_server(server_type_id)
            return

        server_type = self.context.instance.available_server_types[server_type_id]
        component_type = daq.IComponentType.cast_from(server_type)
        configuration = component_type.create_default_config()

        if len(configuration.all_properties) == 0:
            utils.show_error('Empty configuration', f'Selected server type has no properties to configure.', self)
            return

        # Open configuration editor window
        win = tk.Toplevel(self)
        win.withdraw()  # hide until centered
        win.title('Server configuration')
        win.attributes("-topmost", True)
        win.transient(self)

        frame = tk.Frame(win)
        frame.pack(fill=tk.BOTH, expand=True)

        def apply():
            parent_top = self.parent.winfo_toplevel()
            win.destroy()
            self.configure(cursor='watch')
            parent_top.configure(cursor='watch')
            self.after(0, lambda: self.execute_add_server(server_type_id, configuration))

        tree = PropertiesView(frame, configuration, self.context)
        tree.pack(fill=tk.BOTH, expand=True)

        ttk.Button(win, text="Add", command=apply).pack(side=tk.BOTTOM, anchor=tk.E, padx=5, pady=5)

        win.update_idletasks()
        dpi = self.context.dpi_factor
        w, h = int(600 * dpi), int(400 * dpi)
        main = self.parent.winfo_toplevel()
        x = main.winfo_rootx() + main.winfo_width() // 2 - w // 2
        y = main.winfo_rooty() + main.winfo_height() // 2 - h // 2
        win.geometry(f'{w}x{h}+{x}+{y}')
        win.deiconify()

    def execute_add_server(self, server_type_id, config=None):
        parent_top = self.parent.winfo_toplevel()
        self.configure(cursor='watch')
        parent_top.configure(cursor='watch')
        self.update_idletasks()
        new_server = None
        try:
            new_server = self.context.instance.add_server(server_type_id, config)
        except Exception as e:
            self.configure(cursor='')
            parent_top.configure(cursor='')
            utils.show_error('Error adding server', f'{server_type_id}: {str(e)}', self)
            return

        # Enable mDNS discovery if the checkbox is checked
        if self._enable_discovery_var.get():
            try:
                new_server.enable_discovery()
            except Exception as e:
                # Server was added successfully but discovery failed -- warn, don't undo the add
                parent_top.configure(cursor='')
                utils.show_error('Warning', f'Server added but enable_discovery failed: {str(e)}', self)

        parent_top.configure(cursor='')
        self.event_port.emit()
        if self._keep_open_var.get():
            self.update_server_types()
        else:
            self.close()