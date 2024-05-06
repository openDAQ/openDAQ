import os
import opendaq as daq
import tkinter as tk
from tkinter import ttk, simpledialog

from ..utils import *
from ..event_port import EventPort
from .diaolog import Dialog


class AttributesDialog(Dialog):
    def __init__(self, parent, title, node, **kwargs):
        Dialog.__init__(self, parent, title, None, **kwargs)
        self.title(title)
        self.parent = parent
        self.node = node
        self.event_port = EventPort(parent)

        tree_frame = tk.Frame(self)

        tree = ttk.Treeview(tree_frame, columns=(
            'value', 'access'), show='tree headings')

        scroll_bar = ttk.Scrollbar(
            tree_frame, orient="vertical", command=tree.yview)
        tree.configure(yscrollcommand=scroll_bar.set)
        scroll_bar.pack(side="right", fill="y")
        tree.pack(fill="both", expand=True)
        tree_frame.pack(fill="both", expand=True)

        # define headings
        tree.heading('#0', text='Name')
        tree.heading('#1', text='Value')
        tree.heading('#2', text='Locked')
        # layout
        tree.column('#0', anchor=tk.CENTER, width=160, stretch=True)
        tree.column('#1', anchor=tk.CENTER, width=480, stretch=True)
        tree.column('#2', anchor=tk.CENTER, width=80, stretch=False)
        style = ttk.Style()
        style.configure("Treeview.Heading", font='Arial 10 bold')

        tree.bind("<Double-1>", self.handle_double_click)

        self.additional_tree = None

        if daq.ISignal.can_cast_from(node) or daq.IDevice.can_cast_from(node):

            if daq.ISignal.can_cast_from(node):
                tk.Label(self, text='Signal Desciptor').pack(
                    anchor=tk.W, pady=5)
            elif daq.IDevice.can_cast_from(node):
                tk.Label(self, text='Device Attributes').pack(
                    anchor=tk.W, pady=5)

            # additional treeview for specific attributes

            additional_tree_frame = tk.Frame(self)

            additional_tree = ttk.Treeview(additional_tree_frame, columns=(
                'value'), show='tree headings')

            scroll_bar = ttk.Scrollbar(
                additional_tree_frame, orient="vertical", command=additional_tree.yview)
            additional_tree.configure(yscrollcommand=scroll_bar.set)
            scroll_bar.pack(side="right", fill="y")
            additional_tree.pack(fill="both", expand=True)
            additional_tree_frame.pack(fill="both", expand=True)

            # define headings
            additional_tree.heading('#0', text='Name')
            additional_tree.heading('#1', text='Value')
            # layout
            additional_tree.column('#0', anchor=tk.CENTER, width=80)
            additional_tree.column('#1', anchor=tk.CENTER, stretch=True)

            self.additional_tree = additional_tree

        self.tree = tree
        self.initial_update_func = lambda: self.tree_update()

    def handle_double_click(self, event):
        node = self.node
        if not node:
            return

        sel = treeview_get_first_selection(self.tree)
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
        elif type(value) is int:
            self.withdraw()
            new_value = simpledialog.askinteger(
                sel, prompt=prompt, initialvalue=value)
            try:
                self.deiconify()
            except tk.TclError:
                return

        elif type(value) is str:
            self.withdraw()
            new_value = simpledialog.askstring(
                sel, prompt=prompt, initialvalue=value)
            try: # handle the case when the main window is closed
                self.deiconify()
            except tk.TclError:
                return

        if new_value is None or new_value == value:
            return

        setattr(node, attribute, new_value)

        print(f'Value changed for {sel}: {value} -> {new_value}')

        self.tree_update()
        self.event_port.emit()

    def tree_update(self):
        self.tree.delete(*self.tree.get_children())

        self.attributes = {}
        node = self.node

        if node is None:
            return

        self.attributes['Name'] = {
            'Value': node.name, 'Locked': False, 'Attribute': 'name'}
        self.attributes['Description'] = {
            'Value': node.description, 'Locked': False, 'Attribute': 'description'}
        self.attributes['Active'] = {'Value': bool(
            node.active), 'Locked': False, 'Attribute': 'active'}
        self.attributes['Global ID'] = {
            'Value': node.global_id, 'Locked': True, 'Attribute': 'global_id'}
        self.attributes['Local ID'] = {
            'Value': node.local_id, 'Locked': True, 'Attribute': 'local_id'}
        self.attributes['Tags'] = {
            'Value': node.tags.list, 'Locked': False, 'Attribute': 'tags'}
        self.attributes['Visible'] = {'Value': bool(
            node.visible), 'Locked': False, 'Attribute': 'visible'}

        if daq.ISignal.can_cast_from(node):
            signal = daq.ISignal.cast_from(node)

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

        if daq.IInputPort.can_cast_from(node):
            input_port = daq.IInputPort.cast_from(node)

            self.attributes['Signal ID'] = {
                'Value': input_port.signal.global_id if input_port.signal else '', 'Locked': True, 'Attribute': 'signal'}
            self.attributes['Requires Signal'] = {'Value': bool(
                input_port.requires_signal), 'Locked': True, 'Attribute': 'requires_signal'}

        locked_attributes = node.locked_attributes

        for locked_attribute in locked_attributes:
            if locked_attribute not in self.attributes:
                continue
            self.attributes[locked_attribute]['Locked'] = True

        for attr in self.attributes:
            value = self.attributes[attr]['Value']
            locked = yes_no[self.attributes[attr]['Locked']]

            if type(value) is bool:
                value = yes_no[value]
            self.tree.insert(
                '', tk.END, iid=attr, text=attr, values=(value, locked))

        if self.additional_tree is not None:
            self.additional_tree_update()

    def additional_tree_update_signal(self):
        signal = daq.ISignal.cast_from(self.node)
        desc = signal.descriptor

        if not desc:
            return

        descriptor = {}
        descriptor['name'] = desc.name
        descriptor['dimensions'] = desc.dimensions
        descriptor['sample_type'] = desc.sample_type

        if desc.unit:
            descriptor['unit'] = {}
            descriptor['unit']['id'] = desc.unit.id
            descriptor['unit']['name'] = desc.unit.name
            descriptor['unit']['symbol'] = desc.unit.symbol
            descriptor['unit']['quantity'] = desc.unit.quantity

        if desc.value_range:
            descriptor['value_range'] = {}
            descriptor['value_range']['low_value'] = desc.value_range.low_value
            descriptor['value_range']['high_value'] = desc.value_range.high_value

        if desc.rule:
            descriptor['rule'] = {}
            if desc.rule.type:
                descriptor['rule']['type'] = {}
                descriptor['rule']['type']['name'] = desc.rule.type.name
                descriptor['rule']['type']['value'] = desc.rule.type.value
            descriptor['rule']['core_type'] = desc.rule.core_type
            descriptor['rule']['parameters'] = desc.rule.parameters

        descriptor['origin'] = desc.origin

        if desc.tick_resolution:
            descriptor['tick_resolution'] = {}
            descriptor['tick_resolution']['numerator'] = desc.tick_resolution.numerator
            descriptor['tick_resolution']['denominator'] = desc.tick_resolution.denominator

        if desc.post_scaling:
            descriptor['post_scaling'] = {}
            descriptor['post_scaling']['type'] = desc.post_scaling.type
            descriptor['post_scaling']['core_type'] = desc.post_scaling.core_type
            descriptor['post_scaling']['input_sample_type'] = desc.post_scaling.input_sample_type
            descriptor['post_scaling']['output_sample_type'] = desc.post_scaling.output_sample_type
            descriptor['post_scaling']['parameters'] = desc.post_scaling.parameters

        descriptor['struct_fields'] = desc.struct_fields
        self.fill_additional_tree('', descriptor)

    def additional_tree_update(self):
        self.additional_tree.delete(*self.additional_tree.get_children())
        if daq.ISignal.can_cast_from(self.node):
            self.additional_tree_update_signal()
        elif daq.IDevice.can_cast_from(self.node):
            self.additional_tree_update_device()

    def additional_tree_update_device(self):
        device = daq.IDevice.cast_from(self.node)
        info = device.info

        if not info:
            return

        self.fill_additional_tree('', info)

    def fill_additional_tree(self, parent_node_name, attributes):
        if not attributes:
            return

        if type(attributes) is dict:
            for name, value in attributes.items():
                display_value = value if type(value) is not dict and type(
                    value) is not daq.IDict else ''
                iid = parent_node_name + '.' + name
                self.additional_tree.insert(
                    parent_node_name, tk.END, iid=iid, text=name, values=(display_value))
                if type(value) is dict or type(value) is daq.IDict:
                    self.fill_additional_tree(iid, value)
        elif type(attributes) is daq.IDeviceInfo:
            for property in attributes.visible_properties:
                iid = parent_node_name + '.' + property.name
                value = attributes.get_property_value(property.name)
                self.additional_tree.insert(
                    parent_node_name, tk.END, iid=iid, text=property.name, values=(value))
