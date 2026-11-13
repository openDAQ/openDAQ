import tkinter as tk
from tkinter import ttk
from dataclasses import dataclass

import opendaq as daq

from .. import utils
from ..app_context import AppContext
from .dialog import Dialog
from .checkbox_list import CheckboxList
from .add_device_configuration_view import AddDeviceConfigView

_NO_CONFIG_STR = "-- Streaming only --"
_DEVICE = "Device"
_STREAMING = "Streaming"
_GENERAL = "General"
_ASP = "AllowedStreamingProtocols"
_ACS = "AutomaticallyConnectStreaming"
_PAT = "PrimaryAddressType"
_DEFAULT_ADDR = "-- Default --"
_IPV4 = "IPv4"
_IPV6 = "IPv6"

WHITE = 'white'
CARD_BORDER = '#ced4da'
MUTED = '#868e96'
GREEN = '#2f9e44'
ORANGE = '#e8590c'
RED = '#c92a2a'

# the chosen row's mark in the leftmost column, as a radio button would show it
MARKED, UNMARKED = '◉', '○'

# heading, width, stretch
CONNECT_COLUMNS = {
    'mark': ('', 28, False),
    'type': ('Type', 220, False),
    'kind': ('Kind', 80, False),
    'accepts': ('Accepts', 140, False),
    'address': ('Address', 220, True),
    'reachable': ('Reachable', 80, False),
}


class ConnectList(ttk.Frame):
    """The ways to connect as a single-select row list, one row per type, like the device list.
    rows are (value, type id, kind, accepts, address, reachable); on_select(value, revert) gets each new
    choice, and revert() puts the selection back on the previous row."""

    def __init__(self, parent, rows, current, on_select, columns, dpi_factor=1.0):
        super().__init__(parent)
        self.on_select = on_select
        self.current = current
        self.tree = ttk.Treeview(self, columns=columns, show='headings', selectmode=tk.BROWSE,
                                 height=max(1, len(rows)))
        for column in columns:
            heading, width, stretch = CONNECT_COLUMNS[column]
            self.tree.heading(column, text=heading, anchor=tk.W)
            self.tree.column(column, width=int(width * dpi_factor), minwidth=int(min(width, 60) * dpi_factor),
                             stretch=stretch, anchor=tk.CENTER if column == 'mark' else tk.W)
        self.tree.tag_configure('unreachable', foreground=RED)
        for value, title, kind, accepts, address, reachable in rows:
            cells = {'mark': UNMARKED, 'type': title, 'kind': kind, 'accepts': accepts, 'address': address,
                     'reachable': '' if reachable is None else 'yes' if reachable else 'no'}
            self.tree.insert('', tk.END, iid=value, values=[cells[c] for c in columns],
                             tags=('unreachable',) if reachable is False else ())
        self.columns = list(columns)
        self.tree.pack(fill=tk.X)
        self.select(current)
        self.tree.bind('<<TreeviewSelect>>', self.handle_selected)

    def select(self, value):
        # handle_selected ignores the event this raises, since the selection then equals current
        self.current = value
        self.mark(value)
        if value is not None and self.tree.exists(value):
            self.tree.selection_set(value)
            self.tree.focus(value)
        else:
            self.tree.selection_set(())

    def handle_selected(self, event):
        selection = self.tree.selection()
        if not selection or selection[0] == self.current:
            return
        previous = self.current
        self.current = selection[0]
        self.mark(selection[0])
        self.on_select(selection[0], lambda: self.select(previous))

    def mark(self, value):
        if 'mark' not in self.columns:
            return
        for iid in self.tree.get_children():
            self.tree.set(iid, 'mark', MARKED if iid == value else UNMARKED)

class Card(tk.Frame):
    """A white card with a +/- header, drawn like the Sync tab's cards. Collapsed, it shows a one line
    summary of its choices instead of its body."""

    def __init__(self, parent, dialog, title, summary=''):
        super().__init__(parent, background=WHITE, highlightthickness=1, highlightbackground=CARD_BORDER)
        self.dialog = dialog
        self.title = title
        opened = dialog.opened.get(title, True)

        top = tk.Frame(self, background=WHITE)
        top.pack(fill=tk.X, padx=(4, 8), pady=6)
        size = int(10 * dialog.context.dpi_factor)
        toggle = tk.Canvas(top, width=size, height=size, background=WHITE, highlightthickness=0, cursor='hand2')
        toggle.create_line(0, size // 2, size, size // 2, fill='gray', width=2)
        if not opened:
            toggle.create_line(size // 2, 0, size // 2, size, fill='gray', width=2)
        toggle.pack(side=tk.LEFT, padx=(4, 8))
        name = tk.Label(top, text=title, background=WHITE, font=('TkDefaultFont', 10, 'bold'), cursor='hand2')
        name.pack(side=tk.LEFT)
        if not opened and summary:
            tk.Label(top, text=summary, background=WHITE, foreground=MUTED).pack(side=tk.LEFT, padx=(10, 0))
        for widget in (top, toggle, name):
            widget.bind('<Button-1>', lambda e: dialog.card_toggle(title))

        self.body = None
        if opened:
            tk.Frame(self, height=1, background=CARD_BORDER).pack(fill=tk.X)
            self.body = ttk.Frame(self, style='AddConfigCard.TFrame', padding=(12, 8, 12, 10))
            self.body.pack(fill=tk.BOTH, expand=True)
            self.body.columnconfigure(1, weight=1)
            self.row = 0

    def label(self, text):
        ttk.Label(self.body, text=text, style='AddConfigMuted.TLabel').grid(
            row=self.row, column=0, sticky=tk.W, padx=(0, 12), pady=2)

    def heading(self, text):
        ttk.Label(self.body, text=text, style='AddConfigCard.TLabel', font=('TkDefaultFont', 9, 'bold')).grid(
            row=self.row, column=0, columnspan=2, sticky=tk.W, pady=(8, 2))
        self.row += 1

    def combo(self, text, values, current, on_select, state='readonly'):
        self.label(text)
        var = tk.StringVar(value=current)
        combo = ttk.Combobox(self.body, textvariable=var, values=values, state=state, width=30)
        combo.bind('<<ComboboxSelected>>', lambda e: on_select(var.get()))
        combo.grid(row=self.row, column=1, sticky=tk.W, pady=2)
        self.row += 1
        return combo

    def entry(self, text, value, on_change, state='normal', show=''):
        self.label(text)
        var = tk.StringVar(value=value)
        var.trace_add('write', lambda *_: on_change(var.get()))
        entry = ttk.Entry(self.body, textvariable=var, width=34, state=state, show=show)
        entry.grid(row=self.row, column=1, sticky=tk.W, pady=2)
        self.row += 1
        return entry

    def note(self, text, color=MUTED, mono=False):
        label = ttk.Label(self.body, text=text, style='AddConfigCard.TLabel', foreground=color,
                          font=('Consolas', 8) if mono else ('TkDefaultFont', 8),
                          wraplength=int(260 * self.dialog.context.dpi_factor))
        label.grid(row=self.row, column=1, sticky=tk.W, pady=(0, 4))
        self.row += 1
        return label

    def check(self, text, value, on_change, state='normal'):
        var = tk.BooleanVar(value=value)
        button = ttk.Checkbutton(self.body, text=text, variable=var, style='AddConfigCard.TCheckbutton',
                                 command=lambda: on_change(var.get()))
        button.grid(row=self.row, column=0, columnspan=2, sticky=tk.W, pady=(6, 0))
        button.state([state] if state == 'disabled' else ['!disabled'])
        self.row += 1


# False once Don't ask again was ticked; the warning stays off until the app closes
ASK_ON_CONNECT_CHANGE = True


class ConnectChangeDialog(Dialog):
    """Warns that another Connect with type starts every choice again, with Don't ask again."""

    def __init__(self, parent, context: AppContext, type_id):
        super().__init__(parent, 'Change Connect with', context)
        self.result = False
        body = ttk.Frame(self, padding=(4, 10, 4, 0))
        body.pack(fill=tk.X)
        ttk.Label(body, image='::tk::icons::warning').grid(row=0, column=0, rowspan=2, sticky=tk.N, padx=(0, 12))
        ttk.Label(body, text=f'Connecting with {type_id} will result in the loss of your changes, are you sure?',
                  wraplength=int(360 * context.dpi_factor)).grid(row=0, column=1, sticky=tk.W)
        self.dont_ask = tk.BooleanVar(value=False)
        ttk.Checkbutton(body, text="Don't ask again", variable=self.dont_ask).grid(row=1, column=1, sticky=tk.W,
                                                                                   pady=(10, 0))
        buttons = ttk.Frame(self, padding=(0, 12, 0, 4))
        buttons.pack(fill=tk.X)
        ttk.Button(buttons, text='Cancel', command=self.close).pack(side=tk.RIGHT)
        change = ttk.Button(buttons, text='Change', command=self.ok)
        change.pack(side=tk.RIGHT, padx=(0, 8))
        self.bind('<Return>', lambda e: self.ok())
        self.after_idle(change.focus_set)

    def ok(self):
        global ASK_ON_CONNECT_CHANGE
        if self.dont_ask.get():
            ASK_ON_CONNECT_CHANGE = False
        self.result = True
        self.destroy()

    def close(self):
        self.result = False
        self.destroy()


def ask_connect_change(parent, context, type_id):
    """True to go ahead with another Connect with type."""
    if not ASK_ON_CONNECT_CHANGE:
        return True
    dialog = ConnectChangeDialog(parent, context, type_id)
    dialog.show()
    return dialog.result


@dataclass
class ServerCapabilityInternal:
    connection_string: str
    ipv4_connection_string: str
    ipv6_connection_string: str

class AddConfigDialog(Dialog):
    @classmethod
    def for_discoverable(cls, parent, context: AppContext, parent_device, device_info):
        """Populate server capabilities and tcp/ip and streaming from device_info."""
        discoverable = True
        server_capabilities = {}

        # Find TCP/IP and streaming capabilities
        has_tcp_ip = False
        has_streaming = False
        for c in device_info.server_capabilities:
            capability = ServerCapabilityInternal(c.connection_string, None, None)

            if c.has_property("AddressInfo"):
                address_info = c.get_property_value("AddressInfo")
                for p in address_info.visible_properties:
                    address = p.value
                    address_type = address.get_property_value("Type")
                    address_string = address.get_property_value("ConnectionString")

                    if address_type == "IPv4":
                        capability.ipv4_connection_string = address_string
                    elif address_type == "IPv6":
                        capability.ipv6_connection_string = address_string

            server_capabilities[c.protocol_id] = capability

            if c.get_property_value("ConnectionType") == "TCP/IP":
                has_tcp_ip = True
            if c.get_property_value("ProtocolType") in ["Streaming", "ConfigurationAndStreaming"]:
                has_streaming = True

        daq_connection_string = device_info.connection_string
        return cls(
            parent,
            context,
            parent_device,
            discoverable,
            server_capabilities,
            daq_connection_string,
            has_streaming,
            has_tcp_ip)

    @classmethod
    def from_connection_string(cls, parent, context: AppContext, parent_device, connection_string, implied_protocol):
        """Show user sensible settings based on entered connection string with minimal assumptions."""
        discoverable = False
        server_capabilities = {}
        server_capabilities[implied_protocol] = ServerCapabilityInternal(connection_string, None, None)

        streaming_protocols = []
        module_manager: daq.IModuleManager = parent_device.context.module_manager
        for _module in module_manager.modules:
            # Reporting streaming types is optional: a module may return OPENDAQ_ERR_NOTIMPLEMENTED.
            # Skip such modules instead of letting one of them break the whole dialog.
            try:
                available_streaming_types = _module.available_streaming_types
            except RuntimeError:
                continue
            for streaming in available_streaming_types:
                streaming_protocols.append(streaming)

        if implied_protocol not in streaming_protocols:
            # When connection string implies non-streaming protocol, the user can select streaming protocols
            for streaming in streaming_protocols:
                server_capabilities[streaming] = ServerCapabilityInternal(None, None, None)
        # else: the only server capability is the streaming protocol implied by the connection string

        # We can't know, must assume true
        has_streaming = True
        has_tcp_ip = True
        return cls(
            parent,
            context,
            parent_device,
            discoverable,
            server_capabilities,
            connection_string,
            has_streaming,
            has_tcp_ip)

    def __init__(
            self,
            parent,
            context: AppContext,
            parent_device,
            discoverable,
            server_capabilities,
            connection_string,
            has_streaming,
            has_tcp_ip):
        super().__init__(parent, 'Add with config', context)
        self.context = context
        self.parent_device = parent_device

        self.discoverable = discoverable
        self.server_capabilities = server_capabilities
        self.default_connection_string = connection_string
        self.has_streaming = has_streaming
        self.has_tcp_ip = has_tcp_ip

        self.geometry('{}x{}'.format(
            int(1200 * self.context.ui_scaling_factor * self.context.dpi_factor), int(600 * self.context.ui_scaling_factor * self.context.dpi_factor)))

        self.protocol('WM_DELETE_WINDOW', self.cancel)

        # MARK: Layout creation
        self.rowconfigure(0, weight=1)
        self.columnconfigure(0, weight=1, uniform='column')
        self.columnconfigure(1, weight=3, uniform='column')

        left_frame = ttk.Frame(self)

        self.protocol_combobox = self.add_combobox(left_frame, "Configuration protocols:")
        self.protocol_combobox.bind("<<ComboboxSelected>>", self.handle_device_protocol_select)
        self.address_combobox = self.add_combobox(left_frame, "Address type:")
        self.address_combobox.bind("<<ComboboxSelected>>", self.handle_address_type_select)
        self.streaming_checklist = self.add_checkbox_list(left_frame, "Streaming protocols:")
        self.streaming_checklist.register_callback(self.handle_checklist_changed)

        left_frame.grid(row=0, column=0)
        left_frame.grid_configure(sticky=tk.NSEW)

        right_frame = ttk.Frame(self)

        self.tabs = ttk.Notebook(right_frame)
        self.tabs.pack(fill=tk.BOTH, expand=True)

        bottom_frame = ttk.Frame(right_frame)
        bottom_frame.pack(side=tk.BOTTOM, fill=tk.X)

        self.status_label = ttk.Label(bottom_frame, text="")
        self.status_label.pack(side=tk.LEFT, anchor=tk.W, fill=tk.X, expand=True)

        self.add_device_button = ttk.Button(bottom_frame, text='Add device', command=self.handle_add_button_clicked)
        self.add_device_button.pack(side=tk.RIGHT, anchor=tk.E, padx=5, pady=5)

        right_frame.grid(row=0, column=1)
        right_frame.grid_configure(sticky=tk.NSEW)

        # Filter and save config, initialize left panel
        self.connection_string = None
        self.config = self.create_add_device_config()
        self.update_selection_widgets()

        # Initialize members related to editor tabs
        self.selected_device_config = self.protocol_combobox.get()
        self.selected_streaming_configs = []

        self.general_tab = self.create_general_tab()
        self.device_tab = self.create_device_tab()
        self.streaming_tabs = []

        self.selected_address_type = self.address_combobox.get()

        self.update_connection_string()

        self.update_general_tab()
        self.update_device_tab()
        self.update_streaming_tabs()

        # Update streaming tabs and general section as all available streamings are checked
        self.handle_checklist_changed(self.streaming_checklist.get_checked())
        self.tabs.select(self.general_tab)

    def handle_add_button_clicked(self):
        self.sync_device_and_streaming_configs()
        self.destroy()

    def cancel(self):
        self.config = None
        self.destroy()

    def close(self):
        # When escape is pressed adding the device is cancelled
        self.cancel()

    def to_error_state(self, message):
        self.add_device_button.config(state="disabled")
        self.set_status_label(f"[ERROR] {message}", style="StatusError.TLabel")

    def to_warning_state(self, message):
        self.add_device_button.config(state="disabled")
        self.set_status_label(f"[WARNING] {message}", style="StatusWarning.TLabel")

    def to_valid_state(self, message):
        self.add_device_button.config(state="!disabled")
        self.set_status_label(f"[OK] {message}", style="StatusOk.TLabel")

    def set_status_label(self, message, style):
        self.status_label.configure(text=message, style=style)

    # MARK: Add widgets
    def add_combobox(self, frame, title):
        sub_frame = ttk.Frame(frame)
        sub_frame.pack(fill=tk.BOTH, pady=(0, 5))
        ttk.Label(sub_frame, text=title, font=("Arial", 10, "bold")).pack(side=tk.TOP, fill=tk.X, anchor=tk.W, pady=5)
        var = tk.StringVar()
        cbox = ttk.Combobox(sub_frame, state="readonly", textvariable=var)
        cbox.pack(side=tk.TOP, fill=tk.X, anchor=tk.W, padx=(0, 25))

        return cbox

    def add_checkbox_list(self, frame, title):
        sub_frame = ttk.Frame(frame)
        sub_frame.pack(fill=tk.BOTH, expand=True, pady=(5, 10))
        ttk.Label(sub_frame, text=title, font=("Arial", 10, "bold")).pack(side=tk.TOP, fill=tk.X, anchor=tk.W)

        checkbox_list = CheckboxList(sub_frame, items=None)
        checkbox_list.pack(fill=tk.BOTH, expand=True)

        return checkbox_list

    # MARK: Handle widget events
    def handle_device_protocol_select(self, event=None):
        value = self.protocol_combobox.get()
        if not value or len(value) == 0:
            self.selected_device_config = None
            return

        self.selected_device_config = value

        self.update_connection_string()
        self.update_device_tab()

    def handle_address_type_select(self, event=None):
        self.selected_address_type = self.address_combobox.get()
        self.update_connection_string()

    def handle_checklist_changed(self, checked_labels: list):
        self.selected_streaming_configs = checked_labels
        self.update_connection_string()
        self.update_streaming_tabs()

        general_section = self.config.get_property_value(_GENERAL)
        if general_section.has_property(_ASP):
            allowed_protocols = general_section.get_property(_ASP)
            allowed = daq.List()
            for label in self.selected_streaming_configs:
                allowed.append(utils.value_to_coretype(
                    label, allowed_protocols.item_type))
            allowed_protocols.value = allowed
            self.general_tab.editor.refresh()

    # MARK: Update notebook tabs
    def create_device_tab(self):
        device_tab = AddDeviceConfigView(self.tabs)
        device_tab.pack(fill=tk.BOTH, expand=True)

        self.tabs.add(device_tab, text=_DEVICE)
        self.tabs.insert(1, device_tab)
        self.tabs.hide(device_tab)
        return device_tab

    def update_device_tab(self):
        if len(self.selected_device_config) == 0 or self.selected_device_config == _NO_CONFIG_STR:
            if self.device_tab:
                self.device_tab.clear()
                self.tabs.hide(self.device_tab)
            return

        tab_title = self.selected_device_config.replace("OpenDAQ", "").replace("Configuration", " Configuration")
        self.tabs.tab(self.device_tab, text=tab_title)

        device_section = self.config.get_property_value(_DEVICE)
        self.device_tab.edit(device_section.get_property(self.selected_device_config), self.context)
        self.tabs.add(self.device_tab) # unhide

    def update_streaming_tabs(self):
        for tab in self.streaming_tabs:
            self.tabs.forget(tab)
            tab.destroy()

        streaming_section = self.config.get_property_value(_STREAMING)

        self.streaming_tabs = []
        for property_name in self.selected_streaming_configs:
            new_tab = AddDeviceConfigView(self)
            new_tab.edit(streaming_section.get_property(property_name), self.context)

            tab_title = property_name.replace("OpenDAQ", "").replace("Streaming", " Streaming")
            self.tabs.add(new_tab, text=tab_title)
            self.tabs.insert(tk.END, new_tab)
            self.streaming_tabs.append(new_tab)

    def create_general_tab(self):
        general_tab = AddDeviceConfigView(self.tabs)
        general_tab.pack(fill=tk.BOTH, expand=True)

        self.tabs.add(general_tab, text=_GENERAL)
        self.tabs.insert(0, general_tab)
        self.tabs.hide(general_tab)
        return general_tab

    def update_general_tab(self):
        self.general_tab.edit(self.config.get_property(_GENERAL), self.context, self.hidden_general_entries)
        self.tabs.add(self.general_tab)

    def update_selection_widgets(self):
        self.update_device_combobox()
        self.update_primary_address_combobox()
        self.update_checklist(self.config.get_property_value(_STREAMING), True)

    def update_device_combobox(self):
        # Lookup streaming options
        streaming_section = self.config.get_property_value(_STREAMING)
        streaming_options = []
        for property in self.context.properties_of_component(streaming_section):
            streaming_options.append(property.name)

        device_section = self.config.get_property_value(_DEVICE)
        device_options = []

        for property in self.context.properties_of_component(device_section):
            # Avoid duplication in device combobox and streaming checklist
            if property.name not in streaming_options:
                device_options.append(property.name)

        # -- Streaming only -- option when streaming is available and for streaming connections to non-discoverable devices
        if len(streaming_options) > 0 and (self.discoverable or len(device_options) == 0):
            device_options.insert(0, _NO_CONFIG_STR)

        self.protocol_combobox["values"] = device_options

        default_idx = min(1, len(device_options) - 1)
        self.protocol_combobox.set(device_options[default_idx])

    def update_primary_address_combobox(self):
        vals = [_DEFAULT_ADDR]
        if self.discoverable and len(self.server_capabilities) > 0:
            vals += [_IPV4, _IPV6]
        self.address_combobox["values"] = vals
        self.address_combobox.set(_DEFAULT_ADDR)

    def update_checklist(self, streaming_section, select_all):
        for property in self.context.properties_of_component(streaming_section):
            self.streaming_checklist.insert(property.name, select_all)

    def update_connection_string(self):
        """Update connection string used to add device. This function also
        performs a quick validation of the UI state and sets the status message."""

        def choose_connection_string(address_type: str, server_capability: ServerCapabilityInternal):
            if address_type == _IPV4 and server_capability.ipv4_connection_string:
                return server_capability.ipv4_connection_string
            elif address_type == _IPV6 and server_capability.ipv6_connection_string:
                return server_capability.ipv6_connection_string
            elif address_type == _DEFAULT_ADDR and server_capability.connection_string:
                return server_capability.connection_string
            return None

        # A configuration protocol is selected
        if self.selected_device_config != _NO_CONFIG_STR:
            if len(self.server_capabilities) == 0:
                self.connection_string = self.default_connection_string
                self.to_valid_state(f"Connecting to local device: {self.connection_string}")
                return

            # Remote devices have protocol IDs defined in server capabilities
            sc_match = None
            for protocol_id, sc_internal in self.server_capabilities.items():
                if protocol_id == self.selected_device_config:
                    sc_match = sc_internal
                    break

            if sc_match is None:
                self.to_error_state("Selected protocol unavailable.")

            s = choose_connection_string(self.selected_address_type, sc_match)

            if s is not None:
                self.connection_string = s
                self.to_valid_state(f"Configuration connection to: {self.connection_string}")
            else:
                self.to_error_state(f"Connection string not available for address type: {self.selected_address_type}.")
            return

        # No config and 0 or >= 2 streaming connections selected, unclear how to connect
        if len(self.selected_streaming_configs) != 1:
            self.connection_string = None
            self.to_error_state("Select a configuration protocol or exactly one streaming protocol.")
            return

        streaming_protocol = self.selected_streaming_configs[0]
        sc_match = None
        for protocol_id, sc_internal in self.server_capabilities.items():
            if protocol_id == streaming_protocol:
                sc_match = sc_internal
                break

        if sc_match is None:
            self.to_error_state("Selected streaming protocol unavailable.")

        s = choose_connection_string(self.selected_address_type, sc_match)
        if s is not None:
            self.connection_string = s
            self.to_valid_state(f"Streaming connection to: {self.connection_string}")
            return

        # Should not have reached this
        self.to_warning_state("Invalid configuration.")

    def create_add_device_config(self):
        supported_protocols = self.compute_supported_protocols()

        # Default add device config assumes all kinds of server capabilities
        config = self.context.instance.create_default_add_device_config()

        self.filter_device_section(config, supported_protocols)
        self.filter_streaming_section(config, supported_protocols)
        self.hidden_general_entries = self.filter_general_section(config, supported_protocols)

        return config

    def compute_supported_protocols(self):
        # Remote devices have protocol IDs defined in server capabilities
        if len(self.server_capabilities) > 0:
            supported_protocols = list()
            for protocol_id in self.server_capabilities:
                supported_protocols.append(protocol_id)
            return supported_protocols

        # For local devices
        supported_prefix = self.default_connection_string.split("://")[0]

        available_device_types = self.parent_device.available_device_types

        # Find protocol ID matching the connection string prefix
        for protocol_id, device_type in available_device_types.items():
            dt_dict = device_type.as_dictionary

            if dt_dict["Prefix"] == supported_prefix:
                return [protocol_id]
        return []

    def filter_device_section(self, config, supported_protocols):
        device_section = config.get_property_value(_DEVICE)
        for property in self.context.properties_of_component(device_section):
            if property.name not in supported_protocols:
                device_section.remove_property(property.name)

    def filter_streaming_section(self, config, supported_protocols):
        streaming_section = config.get_property_value(_STREAMING)
        for property in self.context.properties_of_component(streaming_section):
            if property.name not in supported_protocols:
                streaming_section.remove_property(property.name)

    def filter_general_section(self, config, supported_protocols):
        general_section = config.get_property_value(_GENERAL)
        hidden_property_paths = []

        def add_to_hidden(section, properties):
            for p in properties:
                if section.has_property(p):
                    hidden_property_paths.append(p)

        # Hide options that are covered with the UI interaction
        # The user edits allowed streaming protocols through checklist and
        # automatically connect streaming will connect if any allowed
        add_to_hidden(general_section, [_ASP, _ACS, _PAT])

        if "OpenDAQNativeConfiguration" not in supported_protocols:
            add_to_hidden(general_section, ["Username", "Password", "ClientType", "ExclusiveControlDropOthers"])

        if not self.has_streaming:
            add_to_hidden(
                general_section,
                ["StreamingConnectionHeuristic",
                 "PrioritizedStreamingProtocols"])

        return hidden_property_paths

    def sync_device_and_streaming_configs(self):
        device_section = self.config.get_property_value(_DEVICE)
        streaming_section = self.config.get_property_value(_STREAMING)

        # Copy all the streaming configs from streaming to device, as they could only have been modified
        # in the streaming section
        for streaming in self.context.properties_of_component(streaming_section):
            for device in self.context.properties_of_component(device_section):
                if device.name == streaming.name:
                    # Recursively update
                    utils.update_properties(device.value, streaming.value)
                    break
