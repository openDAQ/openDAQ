import os
import tkinter as tk
from tkinter import ttk, simpledialog

import opendaq as daq

from .. import utils
from ..event_port import EventPort
from ..app_context import AppContext
from .dialog import Dialog


class AttributesDialog(Dialog):
    def __init__(self, parent, title, node, context: AppContext, **kwargs):
        Dialog.__init__(self, parent, title, None, **kwargs)
        self.parent = parent
        self.node = node
        self.context = context
        self.event_port = EventPort(parent)

        self.geometry(f'{600}x{800}')
        tree_frame = ttk.Frame(self)

        tree = ttk.Treeview(tree_frame, columns=(
            'value', 'access'), show='tree headings')

        scroll_bar = ttk.Scrollbar(
            tree_frame, orient=tk.VERTICAL, command=tree.yview)
        tree.configure(yscrollcommand=scroll_bar.set)
        scroll_bar.pack(side=tk.RIGHT, fill=tk.Y)
        tree.pack(fill=tk.BOTH, expand=True)
        tree_frame.pack(fill=tk.BOTH, expand=True)

        # define headings
        tree.heading('#0', anchor=tk.W, text='Name')
        tree.heading('#1', anchor=tk.W, text='Value')
        tree.heading('#2', anchor=tk.W, text='Locked')
        # layout
        tree.column('#0', anchor=tk.W, minwidth=100, width=100)
        tree.column('#1', anchor=tk.W, minwidth=100, width=300)
        tree.column('#2', anchor=tk.W, minwidth=80, width=80)

        tree.bind('<Double-1>', self.handle_double_click)
        tree.bind('<Button-3>', lambda event: self.handle_right_click(tree, event))

        self.additional_tree = None
        self.notebook = None
        if daq.ISignal.can_cast_from(node) or daq.IDevice.can_cast_from(node):
            additional_tree_frame = ttk.Frame(self)
            if daq.ISignal.can_cast_from(node):
                self.node = daq.ISignal.cast_from(node)
                self.notebook = ttk.Notebook(self)
                self.notebook.pack(fill=tk.BOTH, anchor=tk.W)
                signal_desc_frame = ttk.Frame(self.notebook)
                signal_domain_desc_frame = ttk.Frame(self.notebook)
                self.notebook.add(signal_desc_frame, text='Signal Descriptor')
                if self.node.domain_signal is not None:
                    self.notebook.add(signal_domain_desc_frame,
                                      text='Signal Domain Descriptor')
                self.notebook.bind('<<NotebookTabChanged>>',
                                   self.on_tab_change)
            elif daq.IDevice.can_cast_from(node):
                self.node = daq.IDevice.cast_from(node)
                ttk.Label(self, text='Device Info').pack(
                    anchor=tk.W, pady=5)
            # additional treeview for specific attributes

            additional_tree = ttk.Treeview(additional_tree_frame, columns=(
                'value'), show='tree headings')

            scroll_bar = ttk.Scrollbar(
                additional_tree_frame, orient=tk.VERTICAL, command=additional_tree.yview)
            additional_tree.configure(yscrollcommand=scroll_bar.set)
            scroll_bar.pack(side=tk.RIGHT, fill=tk.Y)
            additional_tree.pack(fill=tk.BOTH, expand=True)
            additional_tree_frame.pack(fill=tk.BOTH, expand=True)

            # define headings
            additional_tree.heading('#0', anchor=tk.W, text='Name')
            additional_tree.heading('#1', anchor=tk.W, text='Value')
            # layout
            additional_tree.column('#0', anchor=tk.W, minwidth=100)
            additional_tree.column('#1', anchor=tk.W, minwidth=100)

            self.additional_tree = additional_tree
            additional_tree.bind(
                '<Button-3>', lambda event: self.handle_right_click(additional_tree, event))

        self.tree = tree
        self.initial_update_func = lambda: self.tree_update()

    def get_selected_tab(self):
        if self.notebook:
            return self.notebook.index(self.notebook.select())
        return None

    def on_tab_change(self, event):
        self.additional_tree_update()

    def handle_copy(self, tree):
        sel = utils.treeview_get_first_selection(tree)
        if not sel:
            return
        item = tree.item(sel)
        value_to_copy = item['values'][0] if item['values'] else ''
        self.clipboard_clear()
        self.clipboard_append(value_to_copy)

    def handle_right_click(self, tree, event):
        utils.treeview_select_item(tree, event)

        menu = tk.Menu(self, tearoff=0)
        menu.add_command(
            label='Copy', command=lambda: self.handle_copy(tree))
        menu.tk_popup(event.x_root, event.y_root)

    def handle_double_click(self, event):
        node = self.node
        if not node:
            return

        sel = utils.treeview_get_first_selection(self.tree)[1:]
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
            try:  # handle the case when the main window is closed
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
                'Value': signal.domain_signal.global_id if signal.domain_signal else '', 'Locked': True,
                'Attribute': '.domain_signal'}
            self.attributes['Related Signals IDs'] = {'Value': os.linesep.join(
                [s.global_id for s in signal.related_signals]), 'Locked': True, 'Attribute': 'related_signals'}
            self.attributes['Streamed'] = {'Value': bool(
                signal.streamed), 'Locked': True, 'Attribute': 'streamed'}
            self.attributes['Last Value'] = {
                'Value': utils.get_last_value_for_signal(signal), 'Locked': True, 'Attribute': 'last_value'}

        if daq.IInputPort.can_cast_from(node):
            input_port = daq.IInputPort.cast_from(node)

            self.attributes['Signal ID'] = {
                'Value': input_port.signal.global_id if input_port.signal else '', 'Locked': True,
                'Attribute': 'signal'}
            self.attributes['Requires Signal'] = {'Value': bool(
                input_port.requires_signal), 'Locked': True, 'Attribute': 'requires_signal'}

        locked_attributes = node.locked_attributes

        self.attributes['Status'] = {
            'Value': {}, 'Locked': True, 'Attribute': 'status'}
        self.attributes['Status']['Value'] = dict(
            node.status_container.statuses.items()) or None

        for locked_attribute in locked_attributes:
            if locked_attribute not in self.attributes:
                continue
            self.attributes[locked_attribute]['Locked'] = True

        def tree_fill(parent: str, key: str, value: dict, locked_flag: int):
            locked = utils.yes_no[locked_flag]
            iid = f'{parent}.{key}'
            if type(value) is bool:
                value = utils.yes_no[value]
            elif type(value) is dict:
                self.tree.insert(
                    parent, tk.END, iid=iid, text=key, values=('', locked))
                for k, v in value.items():
                    tree_fill(iid, k, v, 1)
            else:
                self.tree.insert(
                    parent, tk.END, iid=iid, text=key, values=(value, locked))

        for attr in self.attributes:
            locked = self.attributes[attr]['Locked']
            value = self.attributes[attr]['Value']
            if type(value) is bool:
                value = utils.yes_no[value]

            tree_fill('', attr, value, locked)

        if self.additional_tree is not None:
            self.additional_tree_update()

    def fill_tree(self, tree, key, value, parent='', display_attributes=False):

        display_value = value
        # for user to see property value without expanding the tree
        if isinstance(value, daq.IProperty):
            display_value = value.value
            if value.value_type == daq.CoreType.ctBool:
                display_value = utils.yes_no[display_value]
        else:
            display_value = utils.metadata_converters[key](
                value) if key in utils.metadata_converters else value
        id = tree.insert(
            parent, tk.END, text=str(key), values=(str(display_value), ))

        # displaying Nones but not traversing further
        if value is None:
            return

        if isinstance(value, daq.IPropertyObject):
            for property in value.all_properties if self.context.view_hidden_components else value.visible_properties:
                self.fill_tree(tree, property.name, property,
                               id, display_attributes)
        elif isinstance(value, daq.IList):
            try:
                for i, item in enumerate(value):
                    self.fill_tree(tree, i, item, id, display_attributes)
            except Exception:
                pass
        elif isinstance(value, daq.IDict):
            try:
                for k, v in value.items():
                    self.fill_tree(tree, k, v, id, display_attributes)
            except Exception:
                pass
        elif issubclass(type(value), daq.IBaseObject) and display_attributes:
            for name in utils.get_attributes_of_node(value):
                v = getattr(value, name)
                self.fill_tree(tree, name, v, id, display_attributes)

    def additional_tree_update_signal(self):
        signal = daq.ISignal.cast_from(self.node)
        if self.get_selected_tab() == 0:
            desc = signal.descriptor
        else:
            desc = signal.domain_signal.descriptor

        if desc:
            for name in utils.get_attributes_of_node(desc):
                value = getattr(desc, name)
                self.fill_tree(self.additional_tree, name, value, '', True)

    def additional_tree_update_device(self):
        device = daq.IDevice.cast_from(self.node)
        if device.info:
            for property in self.context.properties_of_component(device.info):
                self.fill_tree(self.additional_tree,
                               property.name, property.value)

    def additional_tree_update(self):
        self.additional_tree.delete(*self.additional_tree.get_children())
        if daq.ISignal.can_cast_from(self.node):
            self.additional_tree_update_signal()
        elif daq.IDevice.can_cast_from(self.node):
            self.additional_tree_update_device()
