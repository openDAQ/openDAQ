import tkinter as tk
from tkinter import ttk

import opendaq as daq

from .. import utils
from ..app_context import AppContext
from ..event_port import EventPort
from .dialog import Dialog
from .edit_container_property import EditContainerPropertyDialog


class LoadInstanceConfigDialog2(Dialog):
    def __init__(self, parent, context: AppContext, file):
        super().__init__(parent, 'Load configuration', context)

        self.config_string = file.read()
        file.close()

        self.event_port = EventPort(self, event_callback=self.on_refresh_event)
        self.geometry('{}x{}'.format(
                1200 * self.context.ui_scaling_factor, 650 * self.context.ui_scaling_factor))
        self.protocol('WM_DELETE_WINDOW', self.cancel)

        self.update_params = daq.UpdateParameters()
        self.update_params.device_update_options = daq.DeviceUpdateOptions(self.config_string)

        # item_id -> metadata describing what this row edits
        self.item_meta = {}

        button_frame = ttk.Frame(self)
        button_frame.pack(side=tk.BOTTOM, anchor=tk.E, pady=10)

        ttk.Button(button_frame, text='Load configuration', command=self.load_configuration).pack(
            padx=10, ipadx=20 * self.context.ui_scaling_factor, side=tk.LEFT)
        ttk.Button(button_frame, text='Cancel', command=self.cancel).pack(
            padx=10, ipadx=10 * self.context.ui_scaling_factor, side=tk.LEFT)

        self.tree_frame = ttk.Frame(self)

        self.tree = ttk.Treeview(
            self.tree_frame,
            columns=('value', 'new_value'),
            show='tree headings'
        )
        self.tree.heading('#0', anchor=tk.W, text='Property')
        self.tree.heading('value', anchor=tk.W, text='Value')
        self.tree.heading('new_value', anchor=tk.W, text='New value')

        self.tree.column('#0', anchor=tk.W, width=int(320 * self.context.ui_scaling_factor))
        self.tree.column('value', anchor=tk.W, width=int(330 * self.context.ui_scaling_factor))
        self.tree.column('new_value', anchor=tk.W, width=int(330 * self.context.ui_scaling_factor))

        self.tree.pack(fill=tk.BOTH, expand=True, side=tk.LEFT)

        scroll_bar = ttk.Scrollbar(
            self.tree_frame, orient=tk.VERTICAL, command=self.tree.yview)
        self.tree.configure(yscrollcommand=scroll_bar.set)
        scroll_bar.pack(side=tk.RIGHT, fill=tk.Y)

        self.tree_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
        self.tree.bind('<Double-1>', self.edit_value)

        self.display_config_options('', self.update_params)

    # ----------------------------
    # Display helpers
    # ----------------------------

    def _printed_value(self, value_type, value):
        if value_type == daq.CoreType.ctBool:
            return utils.yes_no[value]
        if value is None:
            return ''
        return value

    def _safe_str(self, value):
        if value is None:
            return ''
        return str(value)

    def display_config_options(self, parent_node, prop_object):
        if not parent_node:
            self.tree.delete(*self.tree.get_children())
            self.item_meta.clear()

        # Show PropertyObject contents
        for property in self.context.properties_of_component(prop_object):
            prop = prop_object.get_property_value(property.name)
            if isinstance(prop, daq.IBaseObject) and daq.IPropertyObject.can_cast_from(prop):
                cast_property = daq.IPropertyObject.cast_from(prop)
                node_id = self.tree.insert(
                    parent_node, tk.END, text=property.name, open=True)
                self.display_config_options(node_id, cast_property)
            else:
                property_value = self._printed_value(property.item_type, prop)
                item_id = self.tree.insert(parent_node, tk.END,
                                           text=property.name, values=(property_value, '')
                )
                self.item_meta[item_id] = {
                    'kind': 'property',
                    'path': utils.get_item_path(self.tree, item_id)
                }

        if not parent_node and self.update_params.device_update_options is not None:
            root_node = self.tree.insert('', tk.END, text='Device update options', open=True)
            self.display_device_update_options(root_node, self.update_params.device_update_options, option_path=[])

    def display_device_update_options(self, parent_node, options, option_path):
        device_title = options.local_id or '<device>'
        node_id = self.tree.insert(parent_node, tk.END, text=device_title, open=True)

        # LocalId - read only
        self.tree.insert(
            node_id, tk.END, text='LocalId', values=(self._safe_str(options.local_id), ''))

        # Manufacturer -> NewManufacturer
        item_id = self.tree.insert(
            node_id, tk.END,
            text='Manufacturer',
            values=(
                self._safe_str(options.manufacturer),
                self._safe_str(options.new_manufacturer)
            )
        )
        self.item_meta[item_id] = {
            'kind': 'device_option',
            'option_path': list(option_path),
            'field': 'new_manufacturer',
            'editable_column': '#2'
        }

        # SerialNumber -> NewSerialNumber
        item_id = self.tree.insert(
            node_id,
            tk.END,
            text='SerialNumber',
            values=(
                self._safe_str(options.serial_number),
                self._safe_str(options.new_serial_number)
            )
        )
        self.item_meta[item_id] = {
            'kind': 'device_option',
            'option_path': list(option_path),
            'field': 'new_serial_number',
            'editable_column': '#2'
        }

        # ConnectionString -> NewConnectionString
        item_id = self.tree.insert(
            node_id,
            tk.END,
            text='ConnectionString',
            values=(
                self._safe_str(options.connection_string),
                self._safe_str(options.new_connection_string)
            )
        )
        self.item_meta[item_id] = {
            'kind': 'device_option',
            'option_path': list(option_path),
            'field': 'new_connection_string',
            'editable_column': '#2'
        }

        # UpdateMode - editable in "New value" column as the effective selected mode
        item_id = self.tree.insert(
            node_id,
            tk.END,
            text='UpdateMode',
            values=(
                '',
                self._safe_str(options.update_mode)
            )
        )
        self.item_meta[item_id] = {
            'kind': 'device_option',
            'option_path': list(option_path),
            'field': 'update_mode',
            'editable_column': '#2'
        }

        for i, child in enumerate(options.child_device_options):
            self.display_device_update_options(
                node_id,
                child,
                option_path=list(option_path) + [i]
            )

    # ----------------------------
    # Data access helpers
    # ----------------------------

    def get_device_options_by_path(self, option_path):
        option = self.update_params.device_update_options
        for index in option_path:
            option = option.child_device_options[index]
        return option

    def update_regular_property_value(self, path, new_value):
        def update_property(context, path, new_value, depth=0):
            for property in self.context.properties_of_component(context):
                if property.name == path[depth]:
                    if depth == len(path) - 1:
                        context.set_property_value(property.name, daq.EvalValue(new_value))
                        return
                    prop = context.get_property_value(property.name)
                    if isinstance(prop, daq.IBaseObject) and daq.IPropertyObject.can_cast_from(prop):
                        casted_property = daq.IPropertyObject.cast_from(prop)
                        update_property(casted_property, path,
                                        new_value, depth + 1)

        update_property(self.update_params, path, new_value)

    def update_device_option_value(self, option_path, field, new_value):
        option = self.get_device_options_by_path(option_path)

        # TODO: Accept only strings
        if field == 'new_manufacturer':
            option.new_manufacturer = new_value
        elif field == 'new_serial_number':
            option.new_serial_number = new_value
        elif field == 'new_connection_string':
            option.new_connection_string = new_value
        elif field == 'update_mode':
            current_value = option.update_mode
            enum_type = type(current_value)

            # TODO: Check how enum editing is done elsewhere
            # Accept enum object, enum member name, integer value, or raw value
            try:
                if isinstance(new_value, enum_type):
                    option.update_mode = new_value
                else:
                    try:
                        option.update_mode = enum_type[new_value]
                    except Exception:
                        try:
                            option.update_mode = enum_type(int(new_value))
                        except Exception:
                            option.update_mode = new_value
            except Exception as exc:
                raise ValueError(f'Invalid UpdateMode value: {new_value}') from exc
        else:
            raise ValueError(f'Unsupported device option field: {field}')

    # ----------------------------
    # Editing
    # ----------------------------

    def save_entry_value(self, entry, item_id, column):
        new_value = entry.get()
        entry.destroy()

        meta = self.item_meta.get(item_id)
        if meta is None:
            return

        try:
            if meta['kind'] == 'property':
                self.tree.set(item_id, column, new_value)
                self.update_regular_property_value(meta['path'], new_value)

            elif meta['kind'] == 'device_option':
                self.tree.set(item_id, column, new_value)
                self.update_device_option_value(
                    meta['option_path'],
                    meta['field'],
                    new_value
                )

        except Exception as exc:
            print(f'Failed to update value: {exc}')
            self.on_refresh_event(None)

    def edit_value(self, event):
        row_id = self.tree.identify_row(event.y)
        column = self.tree.identify_column(event.x)

        if not row_id or column not in ('#1', '#2'):
            return

        meta = self.item_meta.get(row_id)
        if meta is None:
            return

        # Regular property-object properties:
        if meta['kind'] == 'property':
            if column != '#1':
                return

            path = meta['path']
            prop = utils.get_property_for_path(self.context, path, self.update_params)

            if prop.value_type in (daq.CoreType.ctDict, daq.CoreType.ctList):
                EditContainerPropertyDialog(self, prop, self.update_params).show()
                return

            if prop.value_type == daq.CoreType.ctBool:
                prop.value = not prop.value
                self.tree.set(row_id, column, str(prop.value))
                return

            x, y, width, height = self.tree.bbox(row_id, column)
            value = self.tree.set(row_id, column)

            entry = ttk.Entry(self.tree)
            entry.place(x=x, y=y, width=width, height=height)
            entry.insert(0, value)
            entry.focus()

            entry.bind(
                '<Return>',
                lambda e: self.save_entry_value(entry, row_id, column)
            )
            entry.bind(
                '<FocusOut>',
                lambda e: self.save_entry_value(entry, row_id, column)
            )
            return

        # DeviceUpdateOptions rows:
        if meta['kind'] == 'device_option':
            if column != meta.get('editable_column', '#2'):
                return

            x, y, width, height = self.tree.bbox(row_id, column)
            value = self.tree.set(row_id, column)

            entry = ttk.Entry(self.tree)
            entry.place(x=x, y=y, width=width, height=height)
            entry.insert(0, value)
            entry.focus()

            entry.bind(
                '<Return>',
                lambda e: self.save_entry_value(entry, row_id, column)
            )
            entry.bind(
                '<FocusOut>',
                lambda e: self.save_entry_value(entry, row_id, column)
            )

    # ----------------------------
    # Actions
    # ----------------------------

    def load_configuration(self):
        self.context.instance.load_configuration(
            self.config_string, self.update_params)
        self.destroy()

    def cancel(self):
        self.update_params = None
        self.destroy()

    def on_refresh_event(self, event):
        self.display_config_options('', self.update_params)
