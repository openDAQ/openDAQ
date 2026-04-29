import tkinter as tk
from tkinter import ttk

import opendaq as daq

from .. import utils
from ..app_context import AppContext
from ..event_port import EventPort
from .dialog import Dialog
from .edit_container_property import EditContainerPropertyDialog


class LoadInstanceConfigDialog(Dialog):
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

        self._syncing_overlays = False
        self._overlay_widgets = {}
        self._active_dropdown_cb = None

        style = ttk.Style(self)
        style.configure('Selection.TCombobox',
                        fieldbackground='white',
                        background='white',
                        foreground='#1a1a1a',
                        selectbackground='white',
                        selectforeground='#1a1a1a')
        style.configure('Editable.TCombobox',
                        fieldbackground='white',
                        background='white',
                        foreground='#1a1a1a')
        style.configure('Overlay.TCheckbutton', background='white')

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
        self.tree.configure(
            yscrollcommand=lambda *a: (scroll_bar.set(*a), self._sync_overlays())
        )
        scroll_bar.pack(side=tk.RIGHT, fill=tk.Y)

        self.tree_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
        self.tree.bind('<Double-1>', lambda e: self.edit_value(e))
        self.tree.bind('<MouseWheel>', lambda e: self.tree.after_idle(self._sync_overlays))
        self.tree.bind('<ButtonRelease-1>', lambda e: self.tree.after(10, self._sync_overlays), add='+')
        self.tree.bind('<Configure>', lambda e: self.tree.after_idle(self._sync_overlays))
        self._toplevel_bind_id = self.winfo_toplevel().bind(
            '<<DialogReady>>', lambda e=None: self._sync_overlays(), add='+')
        self.bind('<Destroy>', self._on_destroy)

        self.on_refresh_event(None)

    # ----------------------------
    # Display helpers
    # ----------------------------

    def _printed_value(self, value_type, value):
        if value_type == daq.CoreType.ctBool:
            return ''
        if value is None:
            return ''
        return value

    def _safe_str(self, value):
        if value is None:
            return ''
        return str(value)

    def _on_destroy(self, event):
        if event.widget is self:
            try:
                self.winfo_toplevel().unbind('<<DialogReady>>', self._toplevel_bind_id)
            except Exception:
                pass

    def _place_bool_checkbox(self, iid, prop, meta):
        bbox = self.tree.bbox(iid, meta['editable_column'])
        if not bbox:
            return
        x, y, width, height = self._get_overlay_place_geometry(bbox)
        var = tk.BooleanVar(value=bool(prop.value))
        cb = ttk.Checkbutton(self.tree, variable=var, takefocus=False, style='Overlay.TCheckbutton')
        cb.place(x=x, y=y, width=width, height=height)

        def on_change(_prop=prop, _var=var):
            try:
                _prop.value = _var.get()
            except Exception as e:
                print("Failed to set bool:", e)
                return
            self.refresh()

        cb.configure(command=on_change)
        def _on_mousewheel(e):
            self.yview_scroll(int(-1 * (e.delta / 120)), 'units')
            self._sync_overlays()
            return 'break'

        cb.bind('<MouseWheel>', _on_mousewheel)
        self._overlay_widgets[iid] = cb

    def _make_combobox(self, iid, values, current_value, column, editable=False):
        bbox = self.tree.bbox(iid, column)
        if not bbox:
            return None
        x, combo_y, width, combo_height = self._get_overlay_place_geometry(bbox)
        #width -= self._scroll_bar.winfo_width()

        state = 'normal' if editable else 'readonly'
        combo_style = 'Editable.TCombobox' if editable else 'Selection.TCombobox'
        cb = ttk.Combobox(self.tree, values=values, state=state, style=combo_style)
        cb.set(current_value)
        cb.place(x=x, y=combo_y, width=width, height=combo_height)
        if not editable:
            def _toggle_dropdown(e, _cb=cb):
                # Manually toggle dropdown and block default Tk click handling
                # to avoid close-on-press and reopen-on-release behavior.
                active = self._active_dropdown_cb
                if active is _cb:
                    self._unpost_combobox_dropdown(_cb)
                    self._active_dropdown_cb = None
                    return 'break'

                if active is not None:
                    try:
                        if active.winfo_exists():
                            self._unpost_combobox_dropdown(active)
                    except Exception:
                        pass

                self._active_dropdown_cb = _cb
                _cb.focus_set()
                _cb.after_idle(lambda: self._post_combobox_dropdown(_cb))
                return 'break'

            def _clear_active_if_needed(e, _cb=cb):
                if self._active_dropdown_cb is _cb:
                    self._active_dropdown_cb = None

            cb.bind('<Button-1>', _toggle_dropdown)
            cb.bind('<Escape>', _clear_active_if_needed)
            cb.bind('<FocusOut>', _clear_active_if_needed)

        cb.bind('<MouseWheel>', lambda e: 'break')
        cb.bind('<Button-4>', lambda e: 'break')
        cb.bind('<Button-5>', lambda e: 'break')
        return cb

    def _place_enum_combobox(self, iid, option, meta):
        keys = [name for name in dir(daq.DeviceUpdateMode) if not name.startswith("_") and not name.lower() == name]
        current_key = option.update_mode.name
        cb = self._make_combobox(iid, keys, current_key, meta['editable_column'], editable=False)
        if cb is None:
            return

        def on_change(event, _option=option, _cb=cb):
            try:
                _option.update_mode = getattr(daq.DeviceUpdateMode, _cb.get())
            except Exception as e:
                print("Failed to set enum:", e)
                return
            self.on_refresh_event(None)

        cb.bind('<<ComboboxSelected>>', on_change)
        self._overlay_widgets[iid] = cb

    def _get_overlay_place_geometry(self, bbox):
        x, y, width, height = bbox
        vertical_inset = max(1, int(self.context.dpi_factor))
        place_y = y + vertical_inset
        place_height = max(1, height - 2 * vertical_inset)
        return x, place_y, width, place_height

    def _sync_option(self, iid, meta):
        # Filter - only IDeviceOption.update_mode and IProperty.value_type == ctBool get overlays
        if meta['kind'] not in ['device_option', 'property']:
            return
        is_property = meta['kind'] == 'property'
        if is_property and meta['type'] != 'bool':
            return
        elif not is_property and meta['field'] != 'update_mode':
            return

        bbox = self.tree.bbox(iid, meta['editable_column'])
        if bbox:
            if iid not in self._overlay_widgets:
                if is_property:
                    path = meta['path']
                    prop = self.get_regular_property_by_path(path)
                    self._place_bool_checkbox(iid, prop, meta)
                else:
                    path = meta['option_path']
                    option = self.get_device_options_by_path(path)
                    self._place_enum_combobox(iid, option, meta)
            else:
                x, py, w, ph = self._get_overlay_place_geometry(bbox)
                self._overlay_widgets[iid].place(x=x, y=py, width=w, height=ph)
                self._overlay_widgets[iid].lift()
        else:
            if iid in self._overlay_widgets:
                self._overlay_widgets[iid].place_forget()


    def _sync_overlays(self):
        if self._syncing_overlays:
            return
        self._syncing_overlays = True
        try:
            if not self.tree.winfo_viewable():
                return
            for iid, meta in self.item_meta.items():
                self._sync_option(iid, meta)
        finally:
            self._syncing_overlays = False

    def _clear_overlay_widgets(self):
        self._active_dropdown_cb = None
        for cb in self._overlay_widgets.values():
            try:
                cb.destroy()
            except Exception:
                pass
        self._overlay_widgets = {}

    def _post_combobox_dropdown(self, cb):
        try:
            self.tk.call('ttk::combobox::Post', cb)
        except Exception:
            cb.event_generate('<Down>')

    def _unpost_combobox_dropdown(self, cb):
        try:
            self.tk.call('ttk::combobox::Unpost', cb)
        except Exception:
            try:
                cb.event_generate('<Escape>')
            except Exception:
                pass

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
                    'path': utils.get_item_path(self.tree, item_id),
                    'editable_column': '#1',
                    'type': 'bool' if property.value_type == daq.CoreType.ctBool else 'other',
                    'readonly': property.read_only
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

        # UpdateMode - editable in "Value" column as the effective selected mode
        item_id = self.tree.insert(
            node_id,
            tk.END,
            text='UpdateMode',
            values=(
                '',
                ''
            )
        )
        self.item_meta[item_id] = {
            'kind': 'device_option',
            'option_path': list(option_path),
            'field': 'update_mode',
            'editable_column': '#1'
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

    def get_regular_property_by_path(self, path):
        def _get_property(prop_obj, path, depth=0):
            for property in self.context.properties_of_component(prop_obj):
                if property.name != path[depth]:
                    continue

                if depth == len(path) - 1:
                    return property
                prop = prop_obj.get_property_value(property.name)
                if isinstance(prop, daq.IBaseObject) and daq.IPropertyObject.can_cast_from(prop):
                    cast_prop_obj = daq.IPropertyObject.cast_from(prop)
                    _get_property(cast_prop_obj, path, depth + 1)
        return _get_property(self.update_params, path)

    def update_regular_property_value(self, path, new_value):
        def update_property(prop_obj, path, new_value, depth=0):
            for property in self.context.properties_of_component(prop_obj):
                if property.name == path[depth]:
                    if depth == len(path) - 1:
                        prop_obj.set_property_value(property.name, daq.EvalValue(new_value))
                        return
                    prop = prop_obj.get_property_value(property.name)
                    if isinstance(prop, daq.IBaseObject) and daq.IPropertyObject.can_cast_from(prop):
                        cast_prop_obj = daq.IPropertyObject.cast_from(prop)
                        update_property(cast_prop_obj, path,
                                        new_value, depth + 1)

        update_property(self.update_params, path, new_value)

    def update_device_option_value(self, option_path, field, new_value):
        option = self.get_device_options_by_path(option_path)

        if field == 'new_manufacturer':
            option.new_manufacturer = new_value
        elif field == 'new_serial_number':
            option.new_serial_number = new_value
        elif field == 'new_connection_string':
            option.new_connection_string = new_value
        else: # Update mode is edited via dropdown
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
            if meta['field'] == 'update_mode':
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
        self._clear_overlay_widgets()
        self.display_config_options('', self.update_params)
        self.after_idle(self._sync_overlays)
