import tkinter as tk
from tkinter import ttk, filedialog

import opendaq as daq

from .. import add_config_helpers as helpers
from ..add_config_helpers import ADDRESS_TYPES, STREAMING_MODES, HIDDEN_SETTINGS
from ..app_context import AppContext
from .dialog import Dialog
from .generic_properties_treeview import PropertiesTreeview

WHITE = 'white'
CARD_BORDER = '#ced4da'
MUTED = '#868e96'
GREEN = '#2f9e44'
RED = '#c92a2a'

NONE_SELECTED = 'None'


def transports_text(transports, ports=False):
    """name -> port as "plain, tls", or "plain 7420, tls 7422"."""
    return ', '.join(f'{name} {port}' if ports and port else name for name, port in transports.items())


def parse_port(text):
    """The port typed into a Port field, an int from 1 to 65535. Raises ValueError."""
    port = int((text or '').strip())
    if not 0 < port < 65536:
        raise ValueError(text)
    return port


# the chosen row's mark in the leftmost column, as a radio button would show it
MARKED, UNMARKED = '◉', '○'

# heading, width, stretch
CONNECT_COLUMNS = {
    'mark': ('', 28, False),
    'type': ('Type', 220, False),
    'kind': ('Kind', 80, False),
    'accepts': ('Transports', 140, False),
    'address': ('Address', 220, True),
    'reachable': ('Reachable', 80, False),
}


class ConnectList(ttk.Frame):
    """The ways to connect as a single-select row list, one row per type, like the device list.
    rows are (value, type id, kind, transports, address, reachable); on_select(value, revert) gets each new
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


def connect_row_data(p):
    """The Connect with rows of a PendingAdd: (type id, type id, kind, transports, address, reachable)."""
    rows = []
    for row in helpers.connect_rows(p.parent, p.info, p.typed):
        if row.type_id == (p.row.type_id if p.row else None):
            address = helpers.connects_to(p)
        elif row.capability is not None:
            address = helpers.address_of(row.capability, p.cfg.primary_address_type.name if p.cfg else 'IPv4')
        else:
            address = helpers.connection_string(p)
        rows.append((row.type_id, row.type_id, row.kind, transports_text(row.transports), address,
                     helpers.reachable(row.capability)))
    return rows


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

    def pack_in_column(self):
        """The last card in a column: open, it takes the column's remaining height; closed, only its header."""
        if self.body is None:
            self.pack(fill=tk.X, anchor=tk.N)
        else:
            self.pack(fill=tk.BOTH, expand=True)

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

    def entry(self, text, value, on_change, state='normal', show='', width=34):
        self.label(text)
        var = tk.StringVar(value=value)
        var.trace_add('write', lambda *_: on_change(var.get()))
        entry = ttk.Entry(self.body, textvariable=var, width=width, state=state, show=show)
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


class AddConfigDialog(Dialog):
    """Add with config: every way to connect as its own row, changed only behind a warning, then the
    Connection and Login cards, the Streaming card and the type's settings in three columns."""

    def __init__(self, parent, context: AppContext, pendings, confirm_text=None):
        self.p = pendings[0]
        self.count = len(pendings)
        if self.count > 1:
            title = f'Add {self.count} devices with config'
        else:
            title = f'Add {self.p.name} with config'
        super().__init__(parent, title, context)
        self.confirmed = False
        self.opened = {}
        self.show_secrets = tk.BooleanVar(value=False)
        # what a Port field holds that is not a port, by field; kept until it is corrected
        self.port_errors = {}

        scale = self.context.ui_scaling_factor * self.context.dpi_factor
        self.geometry('{}x{}'.format(int(1300 * scale), int(760 * scale)))
        self.protocol('WM_DELETE_WINDOW', self.close)

        style = ttk.Style(self)
        style.configure('AddConfigCard.TFrame', background=WHITE)
        style.configure('AddConfigCard.TLabel', background=WHITE)
        style.configure('AddConfigMuted.TLabel', background=WHITE, foreground=MUTED)
        style.configure('AddConfigCard.TCheckbutton', background=WHITE)
        style.configure('AddConfigSettings.Treeview', background=WHITE, fieldbackground=WHITE, borderwidth=0)
        # the theme draws the border with the tree's field element, so the settings tree leaves it out and sits
        # flat in its card
        style.layout('AddConfigSettings.Treeview', [('Treeview.treearea', {'sticky': 'nswe'})])

        footer = ttk.Frame(self, padding=(0, 8, 0, 4))
        footer.pack(side=tk.BOTTOM, fill=tk.X)
        # a device's own config among several is kept for the Add, not added from here
        if confirm_text is None:
            confirm_text = 'Add device' if self.count == 1 else f'Add {self.count} devices'
        self.add_button = ttk.Button(footer, text=confirm_text, command=self.handle_add_clicked)
        self.add_button.pack(side=tk.RIGHT)
        ttk.Button(footer, text='Cancel', command=self.close).pack(side=tk.RIGHT, padx=(0, 8))
        self.status_label = ttk.Label(footer, text='', wraplength=int(600 * scale))
        self.status_label.pack(side=tk.LEFT, fill=tk.X, expand=True)

        self.header = tk.Frame(self, background=WHITE, highlightthickness=1, highlightbackground=CARD_BORDER)
        self.header.pack(fill=tk.X, pady=(5, 10))

        # three columns: Connection and Login, Streaming, the settings tree
        body = ttk.Frame(self)
        body.pack(fill=tk.BOTH, expand=True)
        for column in range(3):
            body.columnconfigure(column, weight=1, uniform='third')
        body.rowconfigure(0, weight=1)
        self.left = ttk.Frame(body)
        self.left.grid(row=0, column=0, sticky=tk.NSEW, padx=(0, 5))
        self.middle = ttk.Frame(body)
        self.middle.grid(row=0, column=1, sticky=tk.NSEW, padx=5)
        self.right = ttk.Frame(body)
        self.right.grid(row=0, column=2, sticky=tk.NSEW, padx=(5, 0))

        self.refresh()

    @property
    def cfg(self):
        return self.p.cfg

    # MARK: - Drawing
    def refresh(self):
        self.header_draw()
        for frame in (self.left, self.middle, self.right):
            for child in frame.winfo_children():
                child.destroy()
        self.connection_card_draw()
        self.settings_draw()
        self.streaming_card_draw()
        self.login_card_draw()
        self.status_update()

    def header_draw(self):
        for child in self.header.winfo_children():
            child.destroy()
        inner = tk.Frame(self.header, background=WHITE)
        inner.pack(fill=tk.X, padx=12, pady=8)
        tk.Label(inner, text='Connect with', background=WHITE, font=('TkDefaultFont', 10, 'bold')).pack(
            anchor=tk.W, pady=(0, 6))
        rows = ConnectList(inner, connect_row_data(self.p), self.p.row.type_id, self.connect_selected,
                           ('mark', 'type', 'kind', 'accepts', 'address', 'reachable'), self.context.dpi_factor)
        rows.pack(fill=tk.X)

    def connection_card_draw(self):
        cfg = self.cfg
        selection = helpers.transport_config(cfg.type_config)
        summary = ', '.join(v for v in (cfg.primary_address_type.name, selection.transport if selection else '') if v)
        card = Card(self.left, self, 'Connection', summary)
        card.pack(fill=tk.X, pady=(0, 10))
        if card.body is None:
            return

        has_addresses = self.p.row.capability is not None and len(self.p.row.capability.address_info) > 0
        card.combo('Address', ADDRESS_TYPES, cfg.primary_address_type.name, self.address_changed,
                   'readonly' if has_addresses else 'disabled')
        self.transport_draw(card, 'device', cfg.type_config, helpers.DEVICE_TRANSPORT, self.p.row.transports)
        card.check('Scan the network before connecting', cfg.scan_on_add, self.scan_changed)

    def transport_draw(self, card, key, type_config, name, announced):
        """Transport and Port of a device or streaming type config."""
        selection = helpers.transport_config(type_config, name)
        if selection is None:
            card.combo('Transport', [], '', lambda v: None, 'disabled')
            card.note('The type lists no transports')
        else:
            card.combo('Transport', helpers.transport_names(selection), selection.transport,
                       lambda v: self.transport_changed(selection, v))
            port = announced.get(selection.transport)
            if port:
                card.note(f'Announced on port {port}')

        override = helpers.port_override(type_config)
        if override is not None:
            text = self.port_errors.get(key, str(override))
            card.entry('Port', text, lambda v: self.port_changed(key, type_config, v), width=8)
            if key in self.port_errors:
                card.note('Not a port: a number from 1 to 65535', RED)

    def login_fields_draw(self, card, method):
        has_secret = False
        for field in method.fields.values():
            card.label(field.name if field.required else f'{field.name} (optional)')
            row = ttk.Frame(card.body, style='AddConfigCard.TFrame')
            row.grid(row=card.row, column=1, sticky=tk.W, pady=2)
            var = tk.StringVar(value=self.p.credentials.get(field.id, ''))
            var.trace_add('write', lambda *_, f=field.id, v=var: self.credential_changed(f, v.get()))
            kind = field.kind.name
            entry = ttk.Entry(row, textvariable=var, width=32,
                              show='*' if kind == 'Secret' and not self.show_secrets.get() else '')
            entry.pack(side=tk.LEFT)
            if kind == 'FilePath':
                ttk.Button(row, text='…', width=3, command=lambda v=var, f=field: self.browse_file(v, f)).pack(
                    side=tk.LEFT, padx=(4, 0))
            has_secret = has_secret or kind == 'Secret'
            card.row += 1
        return has_secret

    def login_card_draw(self):
        p = self.p
        method = helpers.login_method(p)
        type_config = self.cfg.type_config
        plain_login = p.auth is None and type_config is not None and type_config.has_property('Username')
        summary = method.description if method is not None else 'Username and password' if plain_login else 'none'
        card = Card(self.left, self, 'Login', summary)
        card.pack_in_column()
        if card.body is None:
            return

        has_secret = False
        methods = helpers.offered_methods(p.auth, p.row.capability)
        if methods:
            labels = {m.description or m.id: m.id for m in methods}
            card.combo('Method', list(labels), method.description or method.id,
                       lambda v: self.login_changed(labels[v]))
            has_secret = self.login_fields_draw(card, method)
        elif plain_login:
            # a type without methods takes its login from its own settings
            card.entry('User name', type_config.get_property_value('Username'),
                       lambda v: type_config.set_property_value('Username', v))
            card.entry('Password', type_config.get_property_value('Password'),
                       lambda v: type_config.set_property_value('Password', v),
                       show='' if self.show_secrets.get() else '*')
            has_secret = True
        else:
            card.note('No login methods')

        if has_secret:
            ttk.Checkbutton(card.body, text='Show', variable=self.show_secrets, style='AddConfigCard.TCheckbutton',
                            command=self.refresh).grid(row=card.row, column=1, sticky=tk.W)
            card.row += 1

    def streaming_card_draw(self):
        cfg = self.cfg
        options = cfg.streaming_options
        if options is None:
            return
        streaming = helpers.selected_streaming(cfg)
        streaming_id = streaming.streaming_type.id if streaming is not None else NONE_SELECTED
        card = Card(self.middle, self, 'Streaming', f'{streaming_id}, {options.streaming_mode.name}')
        card.pack_in_column()
        if card.body is None:
            return

        card.combo('Streaming mode', STREAMING_MODES, options.streaming_mode.name, self.streaming_mode_changed)
        labels = list(helpers.streaming_configs(cfg)) + [NONE_SELECTED]
        card.combo('Streaming', labels, streaming_id, self.streaming_changed)
        if streaming is None:
            return
        if helpers.linked(cfg, streaming):
            card.note("Linked to the device's type: its settings start as the device's")
        capability = next((c for c in helpers.capabilities_of(self.p.info)
                           if c.prefix == streaming.streaming_type.connection_string_prefix), None)
        announced = dict(capability.available_transports.items()) if capability is not None else {}
        self.transport_draw(card, 'streaming', streaming.type_config, helpers.STREAMING_TRANSPORT, announced)

    def settings_draw(self):
        cfg = self.cfg
        card = tk.Frame(self.right, background=WHITE, highlightthickness=1, highlightbackground=CARD_BORDER)
        card.pack(fill=tk.BOTH, expand=True)
        sections = [(f'Configure {self.p.row.type_id} connection', cfg.type_config)]
        streaming = helpers.selected_streaming(cfg)
        if streaming is not None and streaming.type_config is not None:
            sections.append((f'{streaming.streaming_type.id} settings', streaming.type_config))
        for index, (title, config) in enumerate(sections):
            if index > 0:
                tk.Frame(card, height=1, background=CARD_BORDER).pack(fill=tk.X)
            tk.Label(card, text=title, background=WHITE, font=('TkDefaultFont', 10, 'bold'), anchor=tk.W).pack(
                fill=tk.X, padx=12, pady=6)
            tk.Frame(card, height=1, background=CARD_BORDER).pack(fill=tk.X)
            frame = ttk.Frame(card, style='AddConfigCard.TFrame', padding=(8, 0, 0, 0))
            frame.pack(fill=tk.BOTH, expand=True)
            if config is None or len(config.visible_properties) == 0:
                ttk.Label(frame, text='No settings', style='AddConfigMuted.TLabel').pack(anchor=tk.W, pady=6)
                continue
            tree = PropertiesTreeview(frame, config, self.context, hidden=HIDDEN_SETTINGS + ['Port'],
                                      style='AddConfigSettings.Treeview')
            tree.pack(fill=tk.BOTH, expand=True)

    def status_errors(self):
        errors = []
        if self.port_errors:
            errors.append('A Port is not a number from 1 to 65535')
        missing = helpers.missing_credentials(self.p)
        if missing:
            errors.append('Enter ' + ', '.join(missing))
        return errors

    def status_update(self):
        errors = self.status_errors()
        if errors:
            self.status_label.configure(text='[ERROR] ' + '; '.join(errors), foreground=RED)
            self.add_button.state(['disabled'])
            return
        self.add_button.state(['!disabled'])
        text = f'Connects to {helpers.connects_to(self.p)}'
        selection = helpers.transport_config(self.cfg.type_config)
        if selection is not None:
            text += f' over {selection.transport}'
        self.status_label.configure(text='[OK] ' + text, foreground=GREEN)

    # MARK: - Changes
    def card_toggle(self, title):
        self.opened[title] = not self.opened.get(title, True)
        self.refresh()

    def connect_selected(self, type_id, revert):
        if type_id == self.p.row.type_id:
            return
        if not ask_connect_change(self, self.context, type_id):
            revert()
            return
        self.connect_changed(type_id)

    def connect_changed(self, type_id):
        self.port_errors = {}
        row = next(r for r in helpers.connect_rows(self.p.parent, self.p.info, self.p.typed) if r.type_id == type_id)
        helpers.select_row(self.p, row)
        self.refresh()

    def address_changed(self, value):
        self.cfg.primary_address_type = daq.AddressType.__members__[value]
        self.refresh()

    def transport_changed(self, selection, name):
        selection.transport = name
        self.refresh()

    def port_changed(self, key, type_config, text):
        try:
            type_config.set_property_value('Port', parse_port(text))
            self.port_errors.pop(key, None)
        except (ValueError, RuntimeError):
            self.port_errors[key] = text
        self.status_update()

    def streaming_changed(self, label):
        self.cfg.streaming_options.streaming_type = None if label == NONE_SELECTED else label
        self.refresh()

    def streaming_mode_changed(self, value):
        self.cfg.streaming_options.streaming_mode = daq.StreamingMode.__members__[value]
        self.refresh()

    def scan_changed(self, value):
        self.cfg.scan_on_add = value

    def login_changed(self, method_id):
        self.p.auth.authentication_method_id = method_id
        self.p.credentials = {}
        self.refresh()

    def credential_changed(self, field_id, value):
        self.p.credentials[field_id] = value
        self.status_update()

    def browse_file(self, var, field):
        path = filedialog.askopenfilename(parent=self, title=field.name, filetypes=helpers.file_types(field))
        if path:
            var.set(path)

    def handle_add_clicked(self):
        if self.status_errors():
            return
        self.confirmed = True
        self.destroy()

    def close(self):
        self.confirmed = False
        self.destroy()
