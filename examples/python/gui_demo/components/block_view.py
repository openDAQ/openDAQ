import tkinter as tk
import opendaq as daq

from .component import Component
from .input_ports_view import InputPortsView
from .output_signals_view import OutputSignalsView
from .properties_view import PropertiesView
from .attributes_dialog import AttributesDialog

class BlockView(tk.Frame, Component):

    def __init__(self, parent, node, context=None, expanded=False, **kwargs):
        tk.Frame.__init__(self, parent, **kwargs)
        Component.__init__(self)
        self.parent = parent
        self.expanded = expanded
        self.node = node
        self.context = context

        active = False
        name = 'None'

        if node is not None and daq.IComponent.can_cast_from(self.node):
            node = daq.IComponent.cast_from(self.node)
            active = node.active
            name = node.name

        self.configure(relief=tk.GROOVE, border=0.5, padx=5, pady=5)
        self.edit_image = context.icons['settings'] if context and context.icons and 'settings' in context.icons else None
        self.collapsed_img = self.context.icons['right'] if self.context and self.context.icons and 'right' in self.context.icons else None
        self.expanded_img = self.context.icons['down'] if self.context and self.context.icons and 'down' in self.context.icons else None
        self.header_frame = tk.Frame(self)
        self.header_frame.pack(fill=tk.X)
        self.toggle_button = tk.Button(
            self.header_frame, text='+', image=self.collapsed_img, command=self.handle_expand_toggle)
        self.toggle_button.pack(side=tk.LEFT)
        
        self.device_img = self.context.icons['device'] if self.context and self.context.icons and 'device' in self.context.icons else None
        self.function_block_img = self.context.icons['function_block'] if self.context and self.context.icons and 'function_block' in self.context.icons else None
        self.folder_img = self.context.icons['folder'] if self.context and self.context.icons and 'folder' in self.context.icons else None

        self.label_icon = tk.Label(self.header_frame)
        self.label_icon.pack(side=tk.LEFT)
        self.label = tk.Label(self.header_frame, text=name)
        self.label.pack(side=tk.LEFT)
        self.edit_button = tk.Button(self.header_frame, text='Edit', image=self.edit_image,
                                     command=lambda: AttributesDialog(self, 'Attributes', self.node).show())
        self.edit_button.pack(side=tk.RIGHT)
        self.checkbox = tk.Checkbutton(
            self.header_frame, text='Active', command=self.handle_active_toggle)
        if active:
            self.checkbox.select()
        self.checkbox.pack(side=tk.RIGHT)

        self.expanded_frame = tk.Frame(self, pady=5)

        if node is not None:
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
            elif daq.IFunctionBlock.can_cast_from(self.node):
                self.node = daq.IFunctionBlock.cast_from(self.node)
                self.properties = PropertiesView(
                    self.expanded_frame, self.node, self.context)
                self.input_ports = InputPortsView(
                    self.expanded_frame, self.node, self.context)
                self.output_signals = OutputSignalsView(
                    self.expanded_frame, self.node, self.context)
                self.label_icon.config(image=self.function_block_img)
            elif daq.IFolder.can_cast_from(self.node):
                self.node = daq.IFolder.cast_from(self.node)
                self.properties = PropertiesView(
                    self.expanded_frame, self.node, self.context)
                self.label_icon.config(image=self.folder_img)
        self.on_expand()

    def handle_expand_toggle(self):
        self.expanded = not self.expanded
        self.on_expand()

    def on_expand(self):
        if self.expanded:
            self.expanded_frame.pack(fill=tk.BOTH)
            self.expanded_frame.grid_columnconfigure(0, weight=1, minsize=300)
            self.expanded_frame.grid_rowconfigure(0, weight=1, minsize=200)

            if self.properties is not None:
                self.properties.grid(
                    row=0, column=0, rowspan=2 if self.properties and self.output_signals else 1, sticky=tk.NSEW)

            if self.input_ports is not None:
                self.input_ports.grid(row=0, column=1, sticky=tk.NSEW)
                self.expanded_frame.grid_columnconfigure(
                    1, weight=1, minsize=300)

            if self.output_signals is not None:
                self.output_signals.grid(
                    row=1 if self.input_ports else 0, column=1, sticky=tk.NSEW)
                self.expanded_frame.grid_rowconfigure(
                    1 if self.input_ports else 0, weight=1, minsize=200)
                self.expanded_frame.grid_columnconfigure(
                    1, weight=1, minsize=300)

            self.toggle_button.config(text='-', image=self.expanded_img)
        else:  # collapsed
            if self.properties is not None:
                self.properties.grid_forget()
            if self.input_ports is not None:
                self.input_ports.grid_forget()
            if self.output_signals is not None:
                self.output_signals.grid_forget()
            self.expanded_frame.pack_forget()
            self.toggle_button.config(text='+', image=self.collapsed_img)

    def refresh(self):
        return super().refresh()

    def handle_active_toggle(self):
        if daq.IComponent.can_cast_from(self.node):
            ctx = daq.IComponent.cast_from(self.node)
            ctx.active = not ctx.active
        self.refresh()
