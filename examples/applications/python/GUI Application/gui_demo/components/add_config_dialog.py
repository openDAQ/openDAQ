import tkinter as tk
from tkinter import ttk, filedialog

from ..add_device_context import ADDRESS_TYPES, STREAMING_MODES
from ..app_context import AppContext
from ..AEC_stub import (ConnectionSecurity, SecurityMode, DEVICE_SECURITY_MODES, STREAMING_SECURITY_MODES,
                        HIDDEN_SETTINGS, is_fingerprint)
from .login_dialog import file_types
from .dialog import Dialog
from .generic_properties_treeview import PropertiesTreeview

WHITE = 'white'
CARD_BORDER = '#ced4da'
MUTED = '#868e96'
GREEN = '#2f9e44'
ORANGE = '#e8590c'
RED = '#c92a2a'

NONE_SELECTED = 'None'


def levels_text(levels):
    return ', '.join(level.name for level in levels)


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


def connect_row_data(options):
    return [(o.type_id, o.label, o.kind, levels_text(o.security_levels), options.connect_address(o), o.reachable)
            for o in options.connect_options]


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


class AddConfigDialog(Dialog):
    """Add with config: every way to connect as its own row, changed only behind a warning, then the
    Connection and Login cards, the Streaming card and the type's settings in three columns."""

    def __init__(self, parent, context: AppContext, options_list, confirm_text=None):
        self.options = options_list[0]
        self.count = len(options_list)
        info = self.options.info
        if self.count > 1:
            title = f'Add {self.count} devices with config'
        elif info is not None:
            title = f'Add {info.name} with config'
        else:
            title = f'Add {self.options.typed_connection_string} with config'
        super().__init__(parent, title, context)
        self.confirmed = False
        self.opened = {}
        self.show_secrets = tk.BooleanVar(value=False)

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
        options = self.options
        inner = tk.Frame(self.header, background=WHITE)
        inner.pack(fill=tk.X, padx=12, pady=8)
        tk.Label(inner, text='Connect with', background=WHITE, font=('TkDefaultFont', 10, 'bold')).pack(
            anchor=tk.W, pady=(0, 6))
        rows = ConnectList(inner, connect_row_data(options), options.connect.type_id, self.connect_selected,
                           ('mark', 'type', 'kind', 'accepts', 'address', 'reachable'), self.context.dpi_factor)
        rows.pack(fill=tk.X)

    def connection_card_draw(self):
        options = self.options
        summary = f'{options.config.primary_address_type}, {options.config.security_mode.value}'
        card = Card(self.left, self, 'Connection', summary)
        card.pack(fill=tk.X, pady=(0, 10))
        if card.body is None:
            return

        has_addresses = options.connect.capability is not None and len(options.connect.capability.addresses) > 0
        card.combo('Address', ADDRESS_TYPES, options.config.primary_address_type, self.address_changed,
                   'readonly' if has_addresses else 'disabled')

        card.combo('Security', [m.value for m in DEVICE_SECURITY_MODES], options.config.security_mode.value,
                   lambda v: self.security_changed(SecurityMode(v)))
        try:
            options.device_security()
        except Exception as e:
            card.note(str(e), RED)

        unsecure = options.config.security_mode == SecurityMode.Unsecure
        card.entry('Server fingerprint', options.config.server_fingerprint, self.fingerprint_changed,
                   'disabled' if unsecure else 'normal')
        if not unsecure and options.config.server_fingerprint.strip() \
                and not is_fingerprint(options.config.server_fingerprint):
            card.note('Not sha256: followed by 64 hex digits', RED)

        card.check('Scan the network before connecting', options.config.scan_on_add, self.scan_changed)

    def streaming_card_draw(self):
        options = self.options
        streaming_options = options.config.streaming_options
        if streaming_options is None:
            return
        streaming = options.streaming.streaming_type.id if options.streaming is not None else NONE_SELECTED
        card = Card(self.middle, self, 'Streaming', f'{streaming}, {streaming_options.streaming_mode}')
        card.pack(fill=tk.BOTH, expand=True)
        if card.body is None:
            return

        card.combo('Streaming mode', STREAMING_MODES, streaming_options.streaming_mode, self.streaming_mode_changed)
        if streaming_options.streaming_mode == 'Disabled':
            return

        labels = [s.streaming_type.id for s in options.streaming_configs] + [NONE_SELECTED]
        card.combo('Streaming', labels, streaming, self.streaming_changed)
        if options.streaming is not None:
            card.combo('Security', [m.value for m in STREAMING_SECURITY_MODES], options.streaming.security_mode.value,
                       lambda v: self.streaming_security_changed(SecurityMode(v)))
            try:
                options.streaming_security_resolved()
            except Exception as e:
                card.note(f'Not attached: {e}', ORANGE)
            unsecure = options.streaming_mode_resolved() == SecurityMode.Unsecure
            card.entry('Server fingerprint', options.streaming.server_fingerprint,
                       self.streaming_fingerprint_changed, 'disabled' if unsecure else 'normal')
            if not unsecure and options.streaming.server_fingerprint.strip() \
                    and not is_fingerprint(options.streaming.server_fingerprint):
                card.note('Not sha256: followed by 64 hex digits', RED)

        if streaming_options.streaming_mode == 'MinHops':
            card.check('Share credentials with nested devices', streaming_options.share_credentials_with_nested_devices,
                       self.share_changed)

    def login_fields_draw(self, card, method, credentials, on_change):
        has_secret = False
        for field in method.fields:
            card.label(field.name if field.required else f'{field.name} (optional)')
            row = ttk.Frame(card.body, style='AddConfigCard.TFrame')
            row.grid(row=card.row, column=1, sticky=tk.W, pady=2)
            var = tk.StringVar(value=credentials.get(field.id, ''))
            var.trace_add('write', lambda *_, f=field.id, v=var: on_change(f, v.get()))
            # 0004: a set FilePath credential fails a remote add, since it would name a file on the gateway
            state = 'disabled' if field.kind == 'FilePath' and self.options.config.remote else 'normal'
            entry = ttk.Entry(row, textvariable=var, width=32, state=state,
                              show='*' if field.kind == 'Secret' and not self.show_secrets.get() else '')
            entry.pack(side=tk.LEFT)
            if field.kind == 'FilePath':
                button = ttk.Button(row, text='…', width=3, command=lambda v=var, f=field: self.browse_file(v, f))
                button.pack(side=tk.LEFT, padx=(4, 0))
                button.state([state])
            has_secret = has_secret or field.kind == 'Secret'
            card.row += 1
        return has_secret

    def login_card_draw(self):
        options = self.options
        summary = options.login.description if options.login is not None else 'none'
        card = Card(self.left, self, 'Login', summary)
        card.pack(fill=tk.BOTH, expand=True)
        if card.body is None:
            return

        has_secret = False
        if not options.login_methods:
            card.note('No login methods')
        else:
            methods = {m.description or m.id: m.id for m in options.login_methods}
            card.combo('Method', list(methods), options.login.description or options.login.id,
                       lambda v: self.login_changed(methods[v]))
            has_secret = self.login_fields_draw(card, options.login, options.config.authentication_config.supplied_credentials,
                                                self.credential_changed)

        if options.streaming_on and options.streaming_login_methods:
            card.heading(f'{options.streaming.streaming_type.id} login')
            methods = {m.description or m.id: m.id for m in options.streaming_login_methods}
            card.combo('Method', list(methods), options.streaming_login.description or options.streaming_login.id,
                       lambda v: self.streaming_login_changed(methods[v]))
            has_secret = self.login_fields_draw(card, options.streaming_login,
                                                options.streaming.authentication_config.supplied_credentials,
                                                self.streaming_credential_changed) or has_secret

        if has_secret:
            ttk.Checkbutton(card.body, text='Show', variable=self.show_secrets, style='AddConfigCard.TCheckbutton',
                            command=self.refresh).grid(row=card.row, column=1, sticky=tk.W)
            card.row += 1

    def settings_draw(self):
        options = self.options
        card = tk.Frame(self.right, background=WHITE, highlightthickness=1, highlightbackground=CARD_BORDER)
        card.pack(fill=tk.BOTH, expand=True)
        sections = [(f'Configure {options.connect.type_id} connection', options.config.type_config)]
        if options.streaming_on and not options.streaming.linked and options.streaming.type_config is not None:
            sections.append((f'{options.streaming.streaming_type.id} settings', options.streaming.type_config))
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
            tree = PropertiesTreeview(frame, config, self.context, hidden=HIDDEN_SETTINGS,
                                      style='AddConfigSettings.Treeview')
            tree.pack(fill=tk.BOTH, expand=True)

    def status_problems(self):
        """(errors, warnings) for the current choices."""
        options = self.options
        errors, warnings = [], []
        try:
            security = options.device_security()
        except Exception as e:
            errors.append(str(e))
            security = None
        for value in (options.config.server_fingerprint,
                      options.streaming.server_fingerprint if options.streaming is not None else ''):
            if value.strip() and not is_fingerprint(value):
                errors.append('A server fingerprint is not sha256: followed by 64 hex digits')
                break
        missing = options.missing_credentials()
        if missing:
            errors.append('Enter ' + ', '.join(missing))
        if security == ConnectionSecurity.Unsecure and options.login is not None and options.login.fields:
            warnings.append('the login crosses the network unencrypted')
        if options.streaming_on:
            try:
                options.streaming_security_resolved()
            except Exception as e:
                warnings.append(f'{options.streaming.streaming_type.id} is not attached: {e}')
        return errors, warnings, security

    def status_update(self):
        options = self.options
        errors, warnings, security = self.status_problems()
        if errors:
            self.status_label.configure(text='[ERROR] ' + '; '.join(errors), foreground=RED)
            self.add_button.state(['disabled'])
            return
        self.add_button.state(['!disabled'])
        text = f'Connects to {options.connection_string()} as {options.expected_security()}'
        if options.streaming_on and options.streaming_connection_string():
            text += ', streams over ' + options.streaming_connection_string()
        if warnings:
            self.status_label.configure(text=f'[WARNING] {text}; ' + '; '.join(warnings), foreground=ORANGE)
        else:
            self.status_label.configure(text='[OK] ' + text, foreground=GREEN)

    # MARK: - Changes
    def card_toggle(self, title):
        self.opened[title] = not self.opened.get(title, True)
        self.refresh()

    def connect_selected(self, type_id, revert):
        if type_id == self.options.connect.type_id:
            return
        if not ask_connect_change(self, self.context, type_id):
            revert()
            return
        self.connect_changed(type_id)

    def connect_changed(self, type_id):
        self.options.select_connect(type_id)
        self.refresh()

    def address_changed(self, value):
        self.options.config.primary_address_type = value
        self.refresh()

    def security_changed(self, mode):
        self.options.config.security_mode = mode
        self.refresh()

    def fingerprint_changed(self, value):
        self.options.config.server_fingerprint = value
        self.status_update()

    def streaming_changed(self, label):
        options = self.options
        match = next((s for s in options.streaming_configs if s.streaming_type.id == label), None)
        options.select_streaming(match.streaming_type.id if match is not None else None)
        self.refresh()

    def streaming_security_changed(self, mode):
        self.options.streaming.security_mode = mode
        self.refresh()

    def streaming_fingerprint_changed(self, value):
        self.options.streaming.server_fingerprint = value
        self.status_update()

    def streaming_mode_changed(self, value):
        self.options.config.streaming_options.streaming_mode = value
        self.refresh()

    def share_changed(self, value):
        self.options.config.streaming_options.share_credentials_with_nested_devices = value

    def scan_changed(self, value):
        self.options.config.scan_on_add = value

    def login_changed(self, method_id):
        self.options.config.authentication_config.set_authentication_method(method_id)
        self.refresh()

    def credential_changed(self, field_id, value):
        self.options.config.authentication_config.supplied_credentials[field_id] = value
        self.status_update()

    def streaming_login_changed(self, method_id):
        self.options.streaming.authentication_config.set_authentication_method(method_id)
        self.refresh()

    def streaming_credential_changed(self, field_id, value):
        self.options.streaming.authentication_config.supplied_credentials[field_id] = value
        self.status_update()

    def browse_file(self, var, field):
        path = filedialog.askopenfilename(parent=self, title=field.name, filetypes=file_types(field))
        if path:
            var.set(path)

    def handle_add_clicked(self):
        if self.status_problems()[0]:
            return
        self.confirmed = True
        self.destroy()

    def close(self):
        self.confirmed = False
        self.destroy()
