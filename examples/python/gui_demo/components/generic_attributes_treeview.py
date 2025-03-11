import tkinter as tk
from tkinter import ttk, simpledialog

import opendaq as daq
import os

from .. import utils
from ..app_context import AppContext
from ..event_port import EventPort

class AttributesTreeview(ttk.Treeview):
    def __init__(self, parent, frame, node=None, context: AppContext = None, **kwargs):
        ttk.Treeview.__init__(self, frame, columns=('value', 'access', *context.metadata_fields), show='tree headings', **kwargs)

        self.context = context
        self.node = node
        self.attributes = None
        self.event_port = EventPort(parent)

        scroll_bar = ttk.Scrollbar(
            self, orient=tk.VERTICAL, command=self.yview)
        self.configure(yscrollcommand=scroll_bar.set)
        scroll_bar.pack(side=tk.RIGHT, fill=tk.Y)
        scroll_bar_x = ttk.Scrollbar(
            self, orient=tk.HORIZONTAL, command=self.xview)
        self.configure(xscrollcommand=scroll_bar_x.set)
        scroll_bar_x.pack(side=tk.BOTTOM, fill=tk.X)
        self.pack(fill=tk.BOTH, expand=True)

        self.tag_configure('readonly', foreground='gray')

        # define headings
        self.heading('#0', anchor=tk.W, text='Name')
        self.heading('#1', anchor=tk.W, text='Value')
        self.heading('#2', anchor=tk.W, text='Locked')
        # layout
        self.column('#0', anchor=tk.W, minwidth=100, width=100)
        self.column('#1', anchor=tk.W, minwidth=100, width=300)
        self.column('#2', anchor=tk.W, minwidth=80, width=80)

        self.bind('<Double-1>', lambda event: self.handle_double_click())
        self.bind('<Button-3>', lambda event: self.handle_right_click(event))

        self.tree_update()

    def handle_copy(self):
        sel = utils.treeview_get_first_selection(self)
        if not sel:
            return
        item = self.item(sel)
        value_to_copy = item['values'][0] if item['values'] else ''
        self.clipboard_clear()
        self.clipboard_append(value_to_copy)

    def handle_right_click(self, event):
        utils.treeview_select_item(self, event)

        menu = tk.Menu(self, tearoff=0)
        menu.add_command(label='Copy', command=lambda: self.handle_copy())
        menu.tk_popup(event.x_root, event.y_root)

    def handle_double_click(self):
        node = self.node
        if not node:
            return

        sel = utils.treeview_get_first_selection(self)
        sel = sel[1:] if sel else sel
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
            new_value = simpledialog.askinteger(
                sel, prompt=prompt, initialvalue=value, parent=self)
            try:
                pass
            except tk.TclError:
                return

        elif type(value) is str:
            new_value = simpledialog.askstring(
                sel, prompt=prompt, initialvalue=value, parent=self)
            try:  # handle the case when the main window is closed
                pass
            except tk.TclError:
                return

        if new_value is None or new_value == value:
            return

        setattr(node, attribute, new_value)

        print(f'Value changed for {sel}: {value} -> {new_value}')

        self.tree_update()
        self.event_port.emit()

    def tree_update(self):
        self.delete(*self.get_children())

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
            if signal.related_signals:
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
                self.insert(
                    parent, tk.END, iid=iid, text=key, values=('', locked))
                for k, v in value.items():
                    tree_fill(iid, k, v, 1)
            else:
                self.insert(
                    parent, tk.END, iid=iid, text=key, values=(value, locked))

        for attr in self.attributes:
            locked = self.attributes[attr]['Locked']
            value = self.attributes[attr]['Value']
            if type(value) is bool:
                value = utils.yes_no[value]

            tree_fill('', attr, value, locked)

    def fill_tree(self, key, value, parent='', display_attributes=False):
        display_value = value
        # for user to see property value without expanding the tree
        if isinstance(value, daq.IProperty):
            display_value = value.value
            if value.value_type == daq.CoreType.ctBool:
                display_value = utils.yes_no[display_value]
        else:
            display_value = utils.metadata_converters[key](
                value) if key in utils.metadata_converters else value
        id = self.insert(parent, tk.END, text=str(key), values=(str(display_value),))

        # displaying Nones but not traversing further
        if value is None:
            return

        if isinstance(value, daq.IPropertyObject):
            for property in value.all_properties if self.context.view_hidden_components else value.visible_properties:
                self.fill_tree(property.name, property, id, display_attributes)
        elif isinstance(value, daq.IList):
            try:
                for i, item in enumerate(value):
                    self.fill_tree(i, item, id, display_attributes)
            except Exception:
                pass
        elif isinstance(value, daq.IDict):
            try:
                for k, v in value.items():
                    self.fill_tree(k, v, id, display_attributes)
            except Exception:
                pass
        elif issubclass(type(value), daq.IBaseObject) and display_attributes:
            for name in utils.get_attributes_of_node(value):
                v = getattr(value, name)
                self.fill_tree(name, v, id, display_attributes)
