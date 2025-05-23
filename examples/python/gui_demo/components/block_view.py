import tkinter as tk
from tkinter import ttk

import opendaq as daq

from .. import utils
from ..event_port import EventPort
from .input_ports_view import InputPortsView
from .output_signals_view import OutputSignalsView
from .properties_view import PropertiesView
from .attributes_dialog import AttributesDialog

class BlockView(ttk.Frame):

    def __init__(self, parent, node, context=None, expanded=False, **kwargs):
        ttk.Frame.__init__(self, parent, **kwargs)
        self.parent = parent
        self.expanded = expanded
        self.node = node
        self.context = context
        self.event_port = EventPort(self, event_callback=self.refresh)

        active = False
        name = 'None'

        if node and daq.IComponent.can_cast_from(self.node):
            node = daq.IComponent.cast_from(self.node)
            active = node.active
            name = node.name
        self.configure(relief=tk.SOLID, border=0.5, padding=5)

        self.edit_image = None
        self.collapsed_img = None
        self.expanded_img = None

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
            if 'right' in context.icons:
                self.collapsed_img = context.icons['right']
            if 'down' in context.icons:
                self.expanded_img = context.icons['down']

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
        self.toggle_button = tk.Button(
            self.header_frame, text='+', image=self.collapsed_img, borderwidth=0, command=self.handle_expand_toggle)
        self.toggle_button.pack(side=tk.LEFT)

        self.label_icon = ttk.Label(self.header_frame)
        self.label_icon.pack(side=tk.LEFT)
        self.label = ttk.Label(self.header_frame, text=name)
        self.label.pack(side=tk.LEFT)
        self.edit_button = tk.Button(self.header_frame, text='Edit', image=self.edit_image, borderwidth=0, 
                                     command=lambda: AttributesDialog(self, 'Attributes', self.node, self.context).show())
        self.edit_button.pack(side=tk.RIGHT)
        self.active_var = tk.IntVar(self, value=active)
        self.checkbox = ttk.Checkbutton(
            self.header_frame, text='Active', command=self.handle_active_toggle, variable=self.active_var)
        self.checkbox.pack(side=tk.RIGHT)

        self.expanded_frame = ttk.Frame(self, padding=5)

        if node:
            self.properties = None
            self.input_ports = None
            self.output_signals = None

            if daq.IDevice.can_cast_from(self.node):
                self.node = daq.IDevice.cast_from(self.node)
                self.properties = PropertiesView(
                    self.expanded_frame, self.node, self.context)
                self.output_signals = OutputSignalsView(
                    self.expanded_frame, self.node, self.context)
                self.label_icon.config(image=self.device_img)
                self.cols = [0, 1]
                self.rows = [0]

                available_op_modes = []
                op_modes_nums = list(self.node.available_operation_modes)
                if 0 in op_modes_nums:
                    available_op_modes.append('Unknown')
                if 1 in op_modes_nums:
                    available_op_modes.append('Idle')
                if 2 in op_modes_nums:
                    available_op_modes.append('Operation')
                if 3 in op_modes_nums:
                    available_op_modes.append('SafeOperation')

                op_mode = self.node.operation_mode
                mode_string = ''
                if op_mode == daq.OperationModeType.Unknown:
                    mode_string = 'Unknown'
                elif op_mode == daq.OperationModeType.Idle:
                    mode_string = 'Idle'
                elif op_mode == daq.OperationModeType.Operation:
                    mode_string = 'Operation'
                elif op_mode == daq.OperationModeType.SafeOperation:
                    mode_string = 'SafeOperation'

                opt = tk.StringVar(value=mode_string)
                def on_option_change(*args):
                    var = opt.get()
                    if var == 'Unknown':
                        self.node.operation_mode = daq.OperationModeType.Unknown
                    elif var == 'Idle':
                        self.node.operation_mode = daq.OperationModeType.Idle
                    elif var == 'Operation':
                        self.node.operation_mode = daq.OperationModeType.Operation
                    elif var == 'SafeOperation':
                        self.node.operation_mode = daq.OperationModeType.SafeOperation

                opt.trace_add('write', on_option_change)

                combined = tk.Frame(self.expanded_frame)
                combined.grid(row=1, column=0, padx=5, pady=5, sticky='w')

                label = tk.Label(combined, text='Operation mode: ')
                label.pack(side='left')
                options = tk.OptionMenu(combined, opt, *available_op_modes)
                options.pack(side='left')


            elif daq.IFunctionBlock.can_cast_from(self.node):
                self.node = daq.IFunctionBlock.cast_from(self.node)
                self.properties = PropertiesView(
                    self.expanded_frame, self.node, self.context)
                self.input_ports = InputPortsView(
                    self.expanded_frame, self.node, self.context)
                self.output_signals = OutputSignalsView(
                    self.expanded_frame, self.node, self.context)
                self.label_icon.config(image=self.function_block_img)
                self.cols = [0, 1]
                self.rows = [0, 1]
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
            elif daq.IComponent.can_cast_from(self.node):
                self.node = daq.IComponent.cast_from(self.node)
                self.properties = PropertiesView(
                    self.expanded_frame, self.node, self.context)
                self.label_icon.config(image=self.component_img)
                self.cols = [0]
                self.rows = [0]

        combined = tk.Frame(self.expanded_frame)
        combined.grid(row=2, column=0, padx=5, pady=5, sticky='w')

        self.on_expand()
        self.status_square = tk.Frame(combined, width=10, height=10)
        self.status_square.pack(side='left')
        self.status_message = tk.Message(combined, text='Status not set', width=400)
        self.status_message.pack(side='left')
        self.change_status()

    def change_status(self):
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
                self.status_message.config(text='Status: ' + str(status) + ' Message: ' + message)
            elif status:
                self.status_message.config(text='Status: ' + str(status))
            else:
                self.status_message.config(text='')
        except:
            pass
        self.status_square.config(bg=color)


    def handle_expand_toggle(self):
        self.expanded = not self.expanded
        self.on_expand()

    def on_expand(self):
        if self.expanded:
            self.expanded_frame.pack(fill=tk.BOTH)
            self.expanded_frame.grid_columnconfigure(
                self.cols, weight=1, minsize=300, uniform='column')
            self.expanded_frame.grid_rowconfigure(self.rows, weight=1,
                                                  minsize=300 if self.input_ports and self.output_signals
                                                                 or daq.IFolder.can_cast_from(self.node) and
                                                                 not daq.IDevice.can_cast_from(self.node) else 600)
            if self.properties:
                self.properties.grid(
                    row=0, column=0, rowspan=2 if self.input_ports and self.output_signals else 1, sticky=tk.NSEW)

            if self.input_ports:
                self.input_ports.grid(row=0, column=1, sticky=tk.NSEW)

            if self.output_signals:
                self.output_signals.grid(
                    row=1 if self.input_ports else 0, column=1, sticky=tk.NSEW)

            self.toggle_button.config(text='-', image=self.expanded_img)
        else:  # collapsed
            if self.properties:
                self.properties.grid_forget()
            if self.input_ports:
                self.input_ports.grid_forget()
            if self.output_signals:
                self.output_signals.grid_forget()
            self.expanded_frame.pack_forget()
            self.toggle_button.config(text='+', image=self.collapsed_img)

    def refresh(self, event):
        pass

    def handle_active_toggle(self):
        if daq.IComponent.can_cast_from(self.node):
            ctx = daq.IComponent.cast_from(self.node)
            ctx.active = not ctx.active
            self.active_var.set(ctx.active)
            self.event_port.emit()
