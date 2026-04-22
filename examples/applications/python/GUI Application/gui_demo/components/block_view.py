import tkinter as tk
from tkinter import ttk
from typing import Optional

import opendaq as daq

from .. import utils
from ..event_port import EventPort
from .input_ports_view import InputPortsView
from .output_signals_view import OutputSignalsView
from .properties_view import PropertiesView
from .recorder_view import RecorderView
from .attributes_dialog import AttributesDialog
from .signal_preview_panel import SignalPreviewPanel

class BlockView(ttk.Frame):

    def __init__(self, parent, node, context=None, **kwargs):
        ttk.Frame.__init__(self, parent, **kwargs)
        self.parent = parent
        self.node = node
        self.context = context
        self.event_port = EventPort(self, event_callback=self.refresh)
        self._component_core_event_handler = None

        self.active = False
        self.parent_active = True
        name = 'None'

        if node and daq.IComponent.can_cast_from(self.node):
            node = daq.IComponent.cast_from(self.node)
            self.active = node.active
            self.parent_active = node.parent_active
            name = node.name
        self.configure(relief=tk.FLAT, border=0, padding=(0,5,10,0))

        self.edit_image = None

        self.device_img = None
        self.function_block_img = None
        self.folder_img = None
        self.component_img = None
        self.sync_component_img = None

        self.rows = []
        self.cols = []

        if context and context.icons:
            if 'settings' in context.icons:
                self.edit_image = context.icons['settings']
            if 'device' in context.icons:
                self.device_img = context.icons['device']
            if 'function_block' in context.icons:
                self.function_block_img = context.icons['function_block']
            if 'folder' in context.icons:
                self.folder_img = context.icons['folder']
            if 'circle' in context.icons:
                self.component_img = context.icons['circle']
            if 'link' in context.icons:
                self.sync_component_img = context.icons['link']

        self.header_frame = ttk.Frame(self)
        self.header_frame.pack(fill=tk.X)

        self.label_icon = ttk.Label(self.header_frame)
        self.label_icon.pack(side=tk.LEFT)
        self.label = ttk.Label(self.header_frame, text=name)
        self.label.pack(side=tk.LEFT)
        self.edit_button = tk.Button(self.header_frame, text='Edit', image=self.edit_image, borderwidth=0, 
                                     command=lambda: AttributesDialog(self, 'Attributes', self.node, self.context).show())
        self.edit_button.pack(side=tk.RIGHT, padx=(6, 10))
        self.active_var = tk.IntVar(self, value=self.active)
        checkbox_state = tk.NORMAL if self.parent_active else tk.DISABLED
        self.checkbox = ttk.Checkbutton(
            self.header_frame, text='Active', command=self.handle_active_toggle, variable=self.active_var,
            state=checkbox_state)
        self.checkbox.pack(side=tk.RIGHT)

        self.expanded_frame = ttk.Frame(self)

        if node:
            self.properties = None
            self.input_ports = None
            self.output_signals = None
            self.recoder = None
            
            if daq.IDevice.can_cast_from(self.node):
                self.node = daq.IDevice.cast_from(self.node)
                self.properties = PropertiesView(
                    self.expanded_frame, self.node, self.context)

                self._create_right_stack()
                
                signals = self.node.get_signals(daq.AnySearchFilter() if self.context.view_hidden_components else None)
                self.output_signals = OutputSignalsView(self.right_stack, self.node, self.context)
                self.output_signals.pack(fill=tk.X)
                    
                self.label_icon.config(image=self.device_img)
                self.cols = [0, 1]
                self.rows = [0]

                op_modes_nums = list(self.node.available_operation_modes)
                available_op_modes = []
                if 0 in op_modes_nums:
                    available_op_modes.append('Unknown')
                if 1 in op_modes_nums:
                    available_op_modes.append('Idle')
                if 2 in op_modes_nums:
                    available_op_modes.append('Operation')
                if 3 in op_modes_nums:
                    available_op_modes.append('Safe operation')

                op_mode = self.node.operation_mode
                mode_string = ''
                if op_mode == daq.OperationModeType.Unknown:
                    mode_string = 'Unknown'
                elif op_mode == daq.OperationModeType.Idle:
                    mode_string = 'Idle'
                elif op_mode == daq.OperationModeType.Operation:
                    mode_string = 'Operation'
                elif op_mode == daq.OperationModeType.SafeOperation:
                    mode_string = 'Safe operation'

                mode_to_enum = {
                    'Unknown': daq.OperationModeType.Unknown,
                    'Idle': daq.OperationModeType.Idle,
                    'Operation': daq.OperationModeType.Operation,
                    'Safe operation': daq.OperationModeType.SafeOperation,
                }

                ttk.Label(self.header_frame, text=' | ').pack(side=tk.LEFT)

                op_mode_btn = ttk.Menubutton(
                    self.header_frame,
                    text=mode_string,
                    direction='below',
                )
                op_mode_btn.pack(side=tk.LEFT)

                op_mode_menu = tk.Menu(op_mode_btn, tearoff=0)
                op_mode_btn['menu'] = op_mode_menu

                def make_select(mode):
                    def select():
                        self.node.operation_mode = mode_to_enum.get(mode, daq.OperationModeType.Unknown)
                        op_mode_btn.config(text=mode)
                        self.event_port.emit()
                    return select

                for mode in available_op_modes:
                    op_mode_menu.add_command(label=mode, command=make_select(mode))

                self._bind_mousewheel_recursive(self.right_stack)
            
            elif daq.IFunctionBlock.can_cast_from(self.node):
                self._create_right_stack()

                self.cols = [0, 1]

                if daq.IRecorder.can_cast_from(self.node):
                    self.node = daq.IRecorder.cast_from(self.node)
                    self.recoder = RecorderView(self.right_stack, self.node, self.context)
                self.node = daq.IFunctionBlock.cast_from(self.node)
                self.properties = PropertiesView(
                    self.expanded_frame, self.node, self.context)

                self.input_ports = InputPortsView(self.right_stack, self.node, self.context)
                self.input_ports.pack(fill=tk.BOTH, expand=True)

                self.output_signals = OutputSignalsView(self.right_stack, self.node, self.context)
                self.output_signals.pack(fill=tk.BOTH, expand=True)

                if self.recoder:
                    self.recoder.pack(fill=tk.X)
                
                self.label_icon.config(image=self.function_block_img)
                
                self._signal_preview = None
                if daq.IChannel.can_cast_from(self.node):
                    self._signal_preview = SignalPreviewPanel(
                        self._right_container, self.node, self.context)
                    self._signal_preview.place(
                        relx=0, rely=1.0, relwidth=1.0,
                        anchor='sw')
                    
                self.cols = [0, 1]
                self.rows = [0]
                
                self._bind_mousewheel_recursive(self.right_stack)

                self.cols = [0, 1]
                self.rows = [0]
                
                self._bind_mousewheel_recursive(self.right_stack)
                
            elif daq.IFolder.can_cast_from(self.node):
                self.node = daq.IFolder.cast_from(self.node)
                self.properties = PropertiesView(
                    self.expanded_frame, self.node, self.context)
                self.label_icon.config(image=self.folder_img)
                self.cols = [0]
                self.rows = [0]
            elif daq.ISyncComponent.can_cast_from(self.node):
                self.node = daq.ISyncComponent.cast_from(self.node)
                self.properties = PropertiesView(
                    self.expanded_frame, self.node, self.context)
                self.label_icon.config(image=self.sync_component_img)
                self.cols = [0]
                self.rows = [0]
            elif daq.ISignal.can_cast_from(self.node):
                self.node = daq.ISignal.cast_from(self.node)
                signal_icon = context.icons.get('signal') if context and context.icons else None
                self.label_icon.config(image=signal_icon)
                self.edit_button.pack_forget()
                self.checkbox.pack(side=tk.RIGHT, padx=(6, 14))
                
                self.properties = PropertiesView(
                    self.expanded_frame, self.node, self.context, read_only=True)

                self._create_right_stack()
                self.output_signals = OutputSignalsView(self.right_stack, self.node, self.context)
                self.output_signals.pack(fill=tk.BOTH, expand=True)
                
                self.cols = [0, 1]
                self.rows = [0]

                self._bind_mousewheel_recursive(self.right_stack)
            elif daq.IComponent.can_cast_from(self.node):
                self.node = daq.IComponent.cast_from(self.node)
                self.properties = PropertiesView(
                    self.expanded_frame, self.node, self.context)
                self.label_icon.config(image=self.component_img)
                self.cols = [0]
                self.rows = [0]

        self.layout_view()

        self.status_square = None
        self.status_message = None

        container = self.node.status_container
        if len(container.statuses.items()) > 0:
            ttk.Label(self.header_frame, text=' | ').pack(side=tk.LEFT)

            status_frame = tk.Frame(self.header_frame, cursor='hand2')
            status_frame.pack(side=tk.LEFT)

            self.status_square = tk.Frame(status_frame, width=10, height=10, cursor='hand2')
            self.status_square.pack_propagate(False)
            self.status_square.pack(side=tk.LEFT, padx=(0, 4))

            self.status_message = tk.Label(status_frame, text='', cursor='hand2')
            self.status_message.pack(side=tk.LEFT)

            def _on_click(e, c=container):
                self.show_all_statuses(c)

            def _on_enter(e):
                for w in (status_frame, self.status_message):
                    w.configure(bg='#e0e0e0')

            def _on_leave(e):
                bg = self.header_frame.winfo_rgb(ttk.Style().lookup('TFrame', 'background'))
                bg_hex = '#{:04x}{:04x}{:04x}'.format(*bg)
                for w in (status_frame, self.status_message):
                    w.configure(bg=bg_hex)

            for widget in (status_frame, self.status_square, self.status_message):
                widget.bind('<Button-1>', _on_click)
                widget.bind('<Enter>', _on_enter)
                widget.bind('<Leave>', _on_leave)

            self.change_status()

        if node and daq.IComponent.can_cast_from(self.node):
            component = daq.IComponent.cast_from(self.node)
            self._component_core_event_handler = daq.QueuedEventHandler(self._on_component_core_event)
            component.on_component_core_event + self._component_core_event_handler
            self.bind('<Destroy>', self._on_destroy)

    def _create_right_stack(self):
        self._right_container = ttk.Frame(self.expanded_frame)
        self._right_canvas = tk.Canvas(self._right_container, highlightthickness=0)
        right_scrollbar = ttk.Scrollbar(
            self._right_container, orient=tk.VERTICAL, command=self._right_canvas.yview)
        self.right_stack = ttk.Frame(self._right_canvas)

        # Keep the scroll region in sync with the content size
        self.right_stack.bind('<Configure>',
            lambda e: self._right_canvas.configure(scrollregion=self._right_canvas.bbox('all')))
        self._right_canvas.create_window((0, 0), window=self.right_stack, anchor=tk.NW)
        self._right_canvas.configure(yscrollcommand=right_scrollbar.set)
        self._right_canvas.bind('<Configure>',
            lambda e: self._right_canvas.itemconfig('all', width=e.width))

        self._right_canvas.bind('<MouseWheel>', self._on_right_mousewheel)
        self.right_stack.bind('<MouseWheel>', self._on_right_mousewheel)

        right_scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        self._right_canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

    def _on_right_mousewheel(self, event):
        # Only do scroll when needed
        if self._right_canvas.yview() == (0.0, 1.0):
            return
        self._right_canvas.yview_scroll(int(-1 * (event.delta / 120)), 'units')

    def _bind_mousewheel_recursive(self, widget):
        # Propagate mouse wheel from children up
        widget.bind('<MouseWheel>', self._on_right_mousewheel)
        for child in widget.winfo_children():
            self._bind_mousewheel_recursive(child)

    def _on_destroy(self, event):
        if hasattr(self, '_signal_refresh_job') and self._signal_refresh_job is not None:
            try:
                self.after_cancel(self._signal_refresh_job)
            except Exception:
                pass
            self._signal_refresh_job = None
        if self._component_core_event_handler is not None and self.node is not None and daq.IComponent.can_cast_from(self.node):
            try:
                component = daq.IComponent.cast_from(self.node)
                component.on_component_core_event - self._component_core_event_handler
            except Exception:
                pass
            self._component_core_event_handler = None

    def _on_component_core_event(self, sender, args: daq.IEventArgs):
        if args.event_name == "AttributeChanged":
            if not daq.ICoreEventArgs.can_cast_from(args):
                return
            core_event_args = daq.ICoreEventArgs.cast_from(args)
            params = core_event_args.parameters

            if params["AttributeName"] == "Active":
                self.active = bool(params["Active"])
                if "ParentActive" in params:
                    self.parent_active = bool(params["ParentActive"])
                self.update_active_checkbox(self.active, self.parent_active)
        elif args.event_name in ("PropertyValueChanged", "PropertyAdded", "PropertyRemoved"):
            if self.properties is not None:
                self.properties.refresh()

    def show_all_statuses(self, container):
        dpi = self.context.dpi_factor if self.context else 1.0
        w, h = int(600 * dpi), int(200 * dpi)
        window = tk.Toplevel(self)
        window.withdraw()
        window.title('All statuses')
        window.attributes('-topmost', True)
        window.transient(self)
    
        columns = ('Name', 'Status', 'Message')
    
        tree = ttk.Treeview(window, columns=columns, show='headings')
    
        scroll_bar = ttk.Scrollbar(window, orient=tk.VERTICAL, command=tree.yview)
        tree.configure(yscrollcommand=scroll_bar.set)
        scroll_bar.pack(side=tk.RIGHT, fill=tk.Y)
    
        for col in columns:
            tree.heading(col, text=col, anchor='w')
    
        tree.pack(expand=True, fill='both')
    
        def refresh_statuses():
            tree.delete(*tree.get_children())
            for k, v in container.statuses.items():
                tree.insert('', tk.END, values=(k, v.name, container.get_status_message(k)))
    
        refresh_statuses()
    
        poll_job = [None]
    
        def poll():
            if not window.winfo_exists():
                return
            refresh_statuses()
            poll_job[0] = window.after(1000, poll)
    
        poll_job[0] = window.after(1000, poll)
    
        def on_close():
            if poll_job[0] is not None:
                window.after_cancel(poll_job[0])
            window.destroy()
    
        window.protocol('WM_DELETE_WINDOW', on_close)
    
        main = self.winfo_toplevel()
        window.update_idletasks()
        x = main.winfo_rootx() + main.winfo_width() // 2 - w // 2
        y = main.winfo_rooty() + main.winfo_height() // 2 - h // 2
        window.geometry(f'{w}x{h}+{x}+{y}')
        window.deiconify()

    def change_status(self):
        if self.status_square is None:
            return
        color = utils.StatusColor.NOT_SET
        try:
            status = self.node.status_container.get_status('ComponentStatus')
            if status == daq.Enumeration(daq.String('ComponentStatusType'), daq.String('Ok'), self.node.context.type_manager):
                color = utils.StatusColor.OK
            elif status == daq.Enumeration(daq.String('ComponentStatusType'), daq.String('Warning'), self.node.context.type_manager):
                color = utils.StatusColor.WARNING
            elif status == daq.Enumeration(daq.String('ComponentStatusType'), daq.String('Error'), self.node.context.type_manager):
                color = utils.StatusColor.ERROR
            message = self.node.status_container.get_status_message('ComponentStatus')
            if status and message and message != '':
                self.status_message.config(text= status.name + ' - ' + message)
            elif status:
                self.status_message.config(text= status.name)
            else:
                self.status_message.config(text= 'Status')
        except:
            self.status_message.config(text= 'Status')
            pass
        self.status_square.config(bg=color)

    def layout_view(self):
            self.expanded_frame.pack(fill=tk.BOTH, expand=True)

            # Determine the right-side widget (if any)
            right_widget = None
            if hasattr(self, '_right_container'):
                right_widget = self._right_container
            elif self.input_ports:
                right_widget = self.input_ports
            elif self.output_signals:
                right_widget = self.output_signals

            if self.properties and right_widget:
                self.properties.place(
                    relx=0, rely=0, relwidth=0.55, relheight=1.0)
                right_widget.place(
                    relx=0.55, rely=0, relwidth=0.45, relheight=1.0)
            elif self.properties:
                self.properties.place(
                    relx=0, rely=0, relwidth=1.0, relheight=1.0)

            if self.recoder and not hasattr(self, 'right_stack'):
                self.recoder.place(
                    relx=0.55, rely=0, relwidth=0.45, relheight=1.0)

    def refresh(self, event):
        pass

    def update_active_checkbox(self, active: bool, parent_active: Optional[bool] = None):
        self.active_var.set(1 if active else 0)
        if parent_active is not None:
            self.checkbox.config(state=tk.NORMAL if parent_active else tk.DISABLED)

    def handle_active_toggle(self):
        if daq.IComponent.can_cast_from(self.node):
            ctx = daq.IComponent.cast_from(self.node)
            ctx.active = not ctx.active
            self.active_var.set(ctx.active)
            self.event_port.emit()
    
    def handle_active_toggle(self):
        if daq.IComponent.can_cast_from(self.node):
            ctx = daq.IComponent.cast_from(self.node)
            ctx.active = not ctx.active
            self.active_var.set(ctx.active)
            self.event_port.emit()

    def _handle_enable_discovery(self):
        try:
            self.node.enable_discovery()
        except Exception as e:
            print(f'Enable discovery failed: {e}')
            utils.show_error('Enable discovery failed', str(e), self)

    def _handle_disable_discovery(self):
        try:
            self.node.disable_discovery()
        except Exception as e:
            print(f'Disable discovery failed: {e}')
            utils.show_error('Disable discovery failed', str(e), self)
    