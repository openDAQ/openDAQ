import tkinter as tk
from tkinter import ttk, simpledialog
from ..utils import *
import opendaq as daq
import os


class AttributesDialog(tk.Toplevel):
    def __init__(self, parent, title, node, **kwargs):
        tk.Toplevel.__init__(self, parent, **kwargs)
        self.title(title)
        self.parent = parent
        self.context = node

        self.configure(padx=10, pady=5)

        tree = ttk.Treeview(self, columns=(
            'value', 'access'), show='tree headings')

        scroll_bar = ttk.Scrollbar(
            self, orient="vertical", command=tree.yview)
        tree.configure(yscrollcommand=scroll_bar.set)
        scroll_bar.pack(side="right", fill="y")
        tree.pack(fill="both", expand=True)

        # define headings
        tree.heading('#0', text='Name')
        tree.heading('#1', text='Value')
        tree.heading('#2', text='Locked')
        # layout
        tree.column('#0', anchor=tk.CENTER, width=80)
        tree.column('#1', anchor=tk.CENTER, stretch=True)
        tree.column('#2', anchor=tk.CENTER, width=80, stretch=False)
        # tree.grid(row=0, column=2, sticky=tk.NSEW)
        style = ttk.Style()
        style.configure("Treeview.Heading", font='Arial 10 bold')

        tree.bind("<Double-1>", self.handle_double_click)

        self.tree = tree

    def ok(self):
        self.destroy()

    def handle_double_click(self, event):
        node = self.context
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
            pass
        elif type(value) is int:
            new_value = simpledialog.askinteger(
                sel, prompt=prompt, initialvalue=value)
            pass
        elif type(value) is str:
            new_value = simpledialog.askstring(
                sel, prompt=prompt, initialvalue=value)
            pass

        if new_value is None or new_value == value:
            return

        setattr(node, attribute, new_value)

        print(f'Value changed for {sel}: {value} -> {new_value}')

        self.tree_update()

    def tree_update(self):
        self.tree.delete(*self.tree.get_children())

        self.attributes = {}
        node = self.context

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

    def show(self):
        self.wait_visibility()
        self.tree_update()
        self.grab_set()
        self.wait_window(self)
