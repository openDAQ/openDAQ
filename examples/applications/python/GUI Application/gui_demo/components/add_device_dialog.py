import tkinter as tk
from tkinter import ttk, messagebox
import threading

import opendaq as daq

from .. import utils
from .. import AEC_stub as stub
from ..add_device_context import AddDeviceContext, info_value
from ..app_context import AppContext
from ..event_port import *
from .dialog import Dialog
from .add_config_dialog import AddConfigDialog, WHITE, connect_row_data, levels_text
from .login_dialog import GuiCredentialProvider
from .device_info_dialog import DeviceInfoDialog

MUTED = '#868e96'
# Tk lets the last character of a wrapped line overhang the width a little
WRAP_MARGIN = 8
# an opened device line's details start under its name, past the arrow
DETAILS_INDENT = 18
# the panel's inner margin left and right; its frames pad themselves, so a row's lines reach both edges
PANEL_PAD = 12
# a vertical ttk scrollbar's width, left free beside the scrolled device rows
SCROLLBAR_ROOM = 18
PANEL_BORDER = '#828790'  # the device list's border, so both sides are framed alike
GREEN = '#2f9e44'
RED = '#c92a2a'
BLUE = '#0078d7'
EACH_DEFAULT = "Each device's default"


class AddDeviceDialog(Dialog):
    def __init__(self, parent, context: AppContext, node, **kwargs):
        Dialog.__init__(self, parent, 'Add device', context, **kwargs)
        self.node = node
        # send events to parent window
        self.event_port = EventPort(self.parent)
        self.infos = {}
        self.options = []
        self._search_generation = 0

        scale = self.context.ui_scaling_factor * self.context.dpi_factor
        self.geometry('{}x{}'.format(int(1000 * scale), int(540 * scale)))

        style = ttk.Style(self)
        # the right side is a white panel with a border, like the app's other panels
        style.configure('AddDevicePanel.TFrame', background=WHITE)
        style.configure('AddDevicePanel.TLabel', background=WHITE)
        style.configure('AddDeviceHeader.TLabel', background=WHITE, font=('TkDefaultFont', 12, 'bold'))
        style.configure('AddDeviceSection.TLabel', background=WHITE, font=('TkDefaultFont', 9, 'bold'))
        style.configure('AddDeviceMuted.TLabel', background=WHITE, foreground=MUTED)

        # one row under both sides: Keep open on the left, the add buttons on the right
        footer = ttk.Frame(self)
        footer.pack(side=tk.BOTTOM, fill=tk.X, pady=(8, 0))

        panes = ttk.PanedWindow(self, orient=tk.HORIZONTAL)
        panes.pack(fill=tk.BOTH, expand=True)
        left = ttk.Frame(panes)
        side = ttk.Frame(panes, padding=(8, 0, 0, 0))
        panes.add(left, weight=3)
        panes.add(side, weight=2)
        panes.bind('<Map>', lambda e: self.after_idle(lambda: panes.sashpos(0, int(panes.winfo_width() * 0.58))))

        # device rows
        device_tree_frame = ttk.Frame(left)
        device_tree = ttk.Treeview(device_tree_frame, columns=('name', 'loc', 'conn'), displaycolumns=(
            'name', 'loc', 'conn'), show='tree headings', selectmode=tk.EXTENDED)
        device_scroll_bar = ttk.Scrollbar(device_tree_frame, orient=tk.VERTICAL, command=device_tree.yview)
        device_tree.configure(yscrollcommand=device_scroll_bar.set)
        device_scroll_bar.pack(side=tk.RIGHT, fill=tk.Y)
        self.device_tree = device_tree

        device_tree.heading('name', text='Name', anchor=tk.W)
        device_tree.heading('loc', text=' Location', anchor=tk.W)
        device_tree.heading('conn', text='Connection string', anchor=tk.W)
        device_tree.column('#0', width=0, stretch=False)
        device_tree.column('name', anchor=tk.W, minwidth=int(160 * self.context.dpi_factor))
        device_tree.column('loc', anchor=tk.W, minwidth=int(90 * self.context.dpi_factor))
        device_tree.column('conn', anchor=tk.W, minwidth=int(220 * self.context.dpi_factor))

        device_tree.bind('<Double-1>', self.handle_device_tree_double_click)
        device_tree.bind('<Button-3>', self.handle_right_click)
        device_tree.bind('<<TreeviewSelect>>', self.handle_device_selected)
        device_tree.pack(fill=tk.BOTH, expand=True)
        device_tree_frame.pack(fill=tk.BOTH, expand=True)

        conn_row = ttk.Frame(left)
        ttk.Label(conn_row, text='Connection string:').pack(side=tk.LEFT)
        self.conn_string_var = tk.StringVar()
        self.conn_string_entry = ttk.Entry(conn_row, textvariable=self.conn_string_var)
        self.conn_string_entry.bind('<Return>', self.handle_entry_enter)
        self.conn_string_entry.bind('<KeyRelease>', self.handle_entry_typed)
        # ends where the list's box ends, left of its scrollbar
        self.conn_string_entry.pack(side=tk.LEFT, expand=True, fill=tk.X,
                                    padx=(5, device_scroll_bar.winfo_reqwidth()))
        conn_row.pack(fill=tk.X, pady=(6, 0))

        self._keep_open_var = tk.BooleanVar(value=False)
        ttk.Checkbutton(footer, text='Keep open after adding', variable=self._keep_open_var).pack(side=tk.LEFT)

        # right side: what is selected, and what to connect with
        border = tk.Frame(side, background=WHITE, highlightthickness=1, highlightbackground=PANEL_BORDER)
        border.pack(fill=tk.BOTH, expand=True)
        self.panel = ttk.Frame(border, style='AddDevicePanel.TFrame', padding=(0, 8, 0, 10))
        self.panel.pack(fill=tk.BOTH, expand=True)
        self.info_frame = ttk.Frame(self.panel, style='AddDevicePanel.TFrame')
        self.info_frame.pack(fill=tk.BOTH, expand=True)
        # labels that wrap to the panel's width, so a long name or connection string stays inside it
        self.wrapped = []
        self.header_parts = None
        # connection strings of the device lines opened while several devices are selected
        self.opened_lines = set()
        # AddDeviceContext set with a row's Config... while several devices are selected, by connection string;
        # Connect with below leaves them as they are
        self.own_configs = {}
        self.info_frame.bind('<Configure>', self.handle_info_resized)

        connect_frame = ttk.Frame(self.panel, style='AddDevicePanel.TFrame')
        # packed ahead of the details, so opened device lines never push Connect with out of the panel
        connect_frame.pack(side=tk.BOTTOM, fill=tk.X, padx=PANEL_PAD, pady=(8, 0), before=self.info_frame)
        ttk.Separator(connect_frame).pack(fill=tk.X, pady=(0, 8))
        ttk.Label(connect_frame, text='Connect with', style='AddDeviceSection.TLabel').pack(anchor=tk.W, pady=(0, 4))
        self.connect_rows_frame = ttk.Frame(connect_frame, style='AddDevicePanel.TFrame')
        self.connect_rows_frame.pack(fill=tk.X)
        self.connect_value = None
        self.connects_to = ttk.Label(connect_frame, style='AddDeviceMuted.TLabel')
        self.connects_to.pack(anchor=tk.W)

        actions_row = ttk.Frame(footer)
        actions_row.pack(side=tk.RIGHT)
        self.add_button = ttk.Button(actions_row, text='Add', command=lambda: self.process_add_device(False))
        self.add_button.pack(side=tk.RIGHT)
        self.config_button = ttk.Button(actions_row, text='Add with config…',
                                        command=lambda: self.process_add_device(True))
        self.config_button.pack(side=tk.RIGHT, padx=(0, 5))

        self.dialog_parent_device = self.resolve_parent_device(node)
        if self.dialog_parent_device is not None:
            self.title('Add device to {}'.format(self.dialog_parent_device.name))

        self.selection_draw()

    def resolve_parent_device(self, node):
        if node is not None and daq.IDevice.can_cast_from(node):
            return daq.IDevice.cast_from(node)
        return self.context.instance

    def initial_update(self):
        self.update_child_devices(self.device_tree, self.dialog_parent_device)
        self.conn_string_entry.focus_set()

    # MARK: - Selection
    def selected_infos(self):
        return [self.infos[iid] for iid in self.device_tree.selection() if iid in self.infos]

    def handle_device_selected(self, event):
        infos = self.selected_infos()
        if len(infos) == 1:
            self.conn_string_var.set(infos[0].connection_string)
        elif infos:
            self.conn_string_var.set('')
        self.selection_draw()

    def handle_entry_typed(self, event):
        if event.keysym in ('Return', 'Tab', 'Shift_L', 'Shift_R', 'Control_L', 'Control_R'):
            return
        typed = self.conn_string_var.get().strip()
        selected = self.selected_infos()
        if len(selected) == 1 and selected[0].connection_string == typed:
            return
        self.device_tree.selection_set(())
        self.selection_draw()

    def current_options(self):
        """One AddDeviceContext per selected device, or one for the typed connection string."""
        infos = self.selected_infos()
        if infos:
            return [self.own_configs.get(info.connection_string) or
                    AddDeviceContext(self.context, self.dialog_parent_device, info) for info in infos]
        typed = self.conn_string_var.get().strip()
        if not typed:
            return []
        info = self.find_available_device(self.dialog_parent_device, typed)
        if info is not None:
            return [AddDeviceContext(self.context, self.dialog_parent_device, info)]
        return [AddDeviceContext(self.context, self.dialog_parent_device, None, typed)]

    def shared_connect_labels(self):
        """The types every device without its own config can be connected with, in the first one's order."""
        rest = self.options_without_own()
        if not rest:
            return []
        first = rest[0].connect_options
        shared = [o for o in first if all(opt.connect_option(o.type_id) is not None for opt in rest[1:])]
        return shared

    def options_without_own(self):
        """The selected devices Connect with applies to: those not given their own config with Config..."""
        own = set(map(id, self.own_configs.values()))
        return [o for o in self.options if id(o) not in own]

    def selection_draw(self):
        self.wrapped = []
        self.header_parts = None
        for child in self.info_frame.winfo_children():
            child.destroy()
        infos = self.selected_infos()
        # several devices draw edge to edge, for the lines between their rows; the rest inside the margin
        if len(infos) > 1:
            self.content = self.info_frame
        else:
            self.content = ttk.Frame(self.info_frame, style='AddDevicePanel.TFrame', padding=(PANEL_PAD, 0))
            self.content.pack(fill=tk.BOTH, expand=True)
        try:
            self.options = self.current_options()
        except Exception as e:
            self.options = []
            ttk.Label(self.content, text=str(e), foreground=RED, wraplength=300, style='AddDevicePanel.TLabel').pack(anchor=tk.W)

        typed = self.conn_string_var.get().strip()
        if len(infos) > 1:
            self.info_many_draw(infos)
        elif len(self.options) == 1 and self.options[0].info is not None:
            self.info_one_draw(self.options[0].info)
        elif typed:
            self.info_typed_draw(typed)
        else:
            ttk.Label(self.content, text='Select one or more devices, or type a connection string',
                      style='AddDeviceMuted.TLabel', wraplength=300).pack(anchor=tk.W, pady=(8, 0))

        for child in self.connect_rows_frame.winfo_children():
            child.destroy()
        if len(self.options) == 1:
            rows = connect_row_data(self.options[0])
        else:
            rows = [(o.type_id, o.label, o.kind, '', '', None) for o in self.shared_connect_labels()]
            rest = self.options_without_own()
            if not rows and len(rest) > 1 and all(o.connect is not None for o in rest):
                # devices of different types can still be added, each over its own first type
                rows = [(EACH_DEFAULT, EACH_DEFAULT, '', '', '', None)]
        self.connect_value = rows[0][0] if rows else None
        # one choice: a readonly dropdown of the types; kind, reachability and levels are in the capabilities above.
        # With nothing to connect with it stays, empty and disabled, so the panel keeps its shape
        var = tk.StringVar(value=self.connect_value or '')
        combo = ttk.Combobox(self.connect_rows_frame, textvariable=var, values=[r[0] for r in rows],
                             state='readonly' if rows else 'disabled')
        combo.bind('<<ComboboxSelected>>', lambda e: self.handle_connect_selected(var.get()))
        combo.pack(fill=tk.X)
        self.handle_connect_selected(self.connect_value)

    def info_one_draw(self, info):
        frame = self.content
        # the connection string beside the name, or under it when both do not fit on one line
        header = ttk.Frame(frame, style='AddDevicePanel.TFrame')
        header.pack(fill=tk.X)
        self.header_parts = (ttk.Label(header, text=info.name, style='AddDeviceHeader.TLabel'),
                             ttk.Label(header, text=info.connection_string, font=('Consolas', 8),
                                       style='AddDevicePanel.TLabel'))
        self.header_layout(self.info_frame.winfo_width())
        self.info_details_draw(frame, info, self.options[0] if self.options else None)

    def info_details_draw(self, frame, info, options, indent=0):
        """The subtitle, facts and capabilities of one device, under its name."""
        # "manufacturer: model   Serial number ..." as one muted line under the name
        serial = info_value(info, 'serial_number')
        details = '   '.join(v for v in (info_value(info, 'model'), f'Serial number {serial}' if serial else '') if v)
        manufacturer = info_value(info, 'manufacturer')
        maker = f'{manufacturer}: {details}' if manufacturer and details else manufacturer or details
        if maker:
            self.wrap(ttk.Label(frame, text=maker, style='AddDeviceMuted.TLabel'), indent).pack(anchor=tk.W)
        ttk.Frame(frame, style='AddDevicePanel.TFrame', height=6).pack()

        rows = ttk.Frame(frame, style='AddDevicePanel.TFrame')
        rows.pack(fill=tk.X)
        facts = (('Location', 'location'),
                 ('Software revision', 'software_revision'), ('MAC address', 'mac_address'))
        row = 0
        for label, name in facts:
            value = info_value(info, name)
            if not value:
                continue
            ttk.Label(rows, text=label, style='AddDeviceMuted.TLabel').grid(row=row, column=0, sticky=tk.W, padx=(0, 12))
            ttk.Label(rows, text=value, style='AddDevicePanel.TLabel').grid(row=row, column=1, sticky=tk.W)
            row += 1

        capabilities = list(info.server_capabilities)
        if not capabilities:
            return
        ttk.Label(frame, text='Capabilities', style='AddDeviceSection.TLabel').pack(anchor=tk.W, pady=(10, 2))
        caps = ttk.Frame(frame, style='AddDevicePanel.TFrame')
        caps.pack(fill=tk.X)
        caps.columnconfigure(0, weight=1)
        # the kind of the device type each capability's prefix picks (Mirrored, Adapter)
        kinds = {o.capability.prefix: o.kind for o in options.connect_options
                 if o.capability is not None} if options else {}
        for index, capability in enumerate(options.capabilities if options else []):
            name = ttk.Frame(caps, style='AddDevicePanel.TFrame')
            name.grid(row=index * 2, column=0, sticky=tk.W)
            ttk.Label(name, text=capability.protocol_id, style='AddDevicePanel.TLabel').pack(side=tk.LEFT)
            if kinds.get(capability.prefix):
                ttk.Label(name, text=kinds[capability.prefix], style='AddDeviceMuted.TLabel').pack(side=tk.LEFT,
                                                                                                padx=(8, 0))
            if capability.reachable is not None:
                ttk.Label(caps, text='reachable' if capability.reachable else 'unreachable',
                          foreground=GREEN if capability.reachable else RED, style='AddDevicePanel.TLabel').grid(row=index * 2, column=1, sticky=tk.E)
            # the address and the accepted levels share one line across both columns; packed right first,
            # so a narrow panel cuts the address's end instead of the levels
            under = ttk.Frame(caps, style='AddDevicePanel.TFrame')
            under.grid(row=index * 2 + 1, column=0, columnspan=2, sticky=tk.EW, pady=(0, 4))
            ttk.Label(under, text='accepts ' + levels_text(capability.security_levels), style='AddDeviceMuted.TLabel',
                      font=('TkDefaultFont', 8)).pack(side=tk.RIGHT, padx=(8, 0))
            ttk.Label(under, text=capability.connection_string, style='AddDeviceMuted.TLabel',
                      font=('Consolas', 8)).pack(side=tk.LEFT)

    def wrap(self, label, indent=0):
        self.wrapped.append((label, indent))
        width = self.info_frame.winfo_width()
        if width > 1:
            label.configure(wraplength=width - WRAP_MARGIN - 2 * PANEL_PAD - indent)
        return label

    def handle_info_resized(self, event):
        # Tk breaks a word longer than the line inside it, so a connection string wraps too
        for label, indent in self.wrapped + [(self.connects_to, 0)]:
            if label.winfo_exists():
                label.configure(wraplength=event.width - WRAP_MARGIN - 2 * PANEL_PAD - indent)
        self.header_layout(event.width)

    def header_layout(self, width):
        if self.header_parts is None or not self.header_parts[0].winfo_exists():
            return
        name, second = self.header_parts
        for label in (name, second):
            if label is not None:
                label.grid_forget()
                label.configure(wraplength=0)
        room = width - WRAP_MARGIN - 2 * PANEL_PAD
        beside = second is not None and (width <= 1 or
                                        name.winfo_reqwidth() + 12 + second.winfo_reqwidth() <= room)
        name.grid(row=0, column=0, sticky=tk.SW)
        if second is not None and beside:
            # bottoms aligned, nudged up to sit near the name's baseline
            second.grid(row=0, column=1, sticky=tk.SW, padx=(12, 0), pady=(0, 2))
        elif second is not None:
            second.grid(row=1, column=0, sticky=tk.W)
        if not beside and width > 1:
            for label in (name, second):
                if label is not None:
                    label.configure(wraplength=room)

    def info_many_draw(self, infos):
        frame = self.content
        ttk.Label(frame, text=f'{len(infos)} devices selected', style='AddDeviceHeader.TLabel').pack(
            anchor=tk.W, padx=PANEL_PAD)
        rest = len(self.options_without_own())
        if rest == len(self.options):
            applies = 'Connect with below applies to each'
        elif rest:
            applies = f'Connect with below applies to the {rest} without their own config'
        else:
            applies = 'Every selected device has its own config'
        ttk.Label(frame, text=applies, style='AddDeviceMuted.TLabel').pack(
            anchor=tk.W, padx=PANEL_PAD, pady=(0, 6))
        rows = self.rows_scrolled_draw(frame)
        # one line per device; a click opens it to the details one selected device shows
        options = self.options if len(self.options) == len(infos) else [None] * len(infos)
        # a line above each device and one under the last, so every device reads as its own row
        for info, option in zip(infos, options):
            ttk.Separator(rows).pack(fill=tk.X)
            self.device_line_draw(rows, info, option)
        ttk.Separator(rows).pack(fill=tk.X)
        self.wheel_bind(rows)

    def rows_scrolled_draw(self, frame):
        """A frame that scrolls under the title, with a scrollbar only while its rows are taller than the room."""
        area = ttk.Frame(frame, style='AddDevicePanel.TFrame')
        area.pack(fill=tk.BOTH, expand=True)
        canvas = tk.Canvas(area, background=WHITE, highlightthickness=0, borderwidth=0)
        bar = ttk.Scrollbar(area, orient=tk.VERTICAL, command=canvas.yview)
        canvas.configure(yscrollcommand=bar.set)
        canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        rows = ttk.Frame(canvas, style='AddDevicePanel.TFrame')
        window = canvas.create_window(0, 0, window=rows, anchor=tk.NW)

        def fit(event=None):
            width = canvas.winfo_width()
            canvas.itemconfigure(window, width=width)
            canvas.configure(scrollregion=(0, 0, width, rows.winfo_reqheight()))
            needed = rows.winfo_reqheight() > canvas.winfo_height() > 1
            shown = bar.winfo_manager() == 'pack'
            if needed and not shown:
                bar.pack(side=tk.RIGHT, fill=tk.Y, before=canvas)
            elif not needed and shown:
                bar.pack_forget()
                canvas.yview_moveto(0)

        rows.bind('<Configure>', fit)
        canvas.bind('<Configure>', fit)
        self.rows_canvas, self.rows_bar = canvas, bar
        return rows

    def wheel_bind(self, widget):
        """The mouse wheel scrolls the rows wherever the pointer is on them, opened details included."""
        def wheel(event):
            if self.rows_bar.winfo_manager() == 'pack':
                self.rows_canvas.yview_scroll(int(-event.delta / 120), 'units')
        for child in [self.rows_canvas, widget] + self.descendants(widget):
            child.bind('<MouseWheel>', wheel, add='+')

    def descendants(self, widget):
        found = []
        for child in widget.winfo_children():
            found.append(child)
            found.extend(self.descendants(child))
        return found

    def device_line_draw(self, frame, info, options):
        own = options is not None and info is not None and self.own_configs.get(info.connection_string) is options
        line = ttk.Frame(frame, style='AddDevicePanel.TFrame', cursor='hand2',
                         padding=(PANEL_PAD, 9, PANEL_PAD, 2 if own else 9))
        line.pack(fill=tk.X)
        below = line
        if own:
            below = ttk.Label(frame, text=f'Own config, connects with {options.connect.type_id}', foreground=BLUE,
                              style='AddDevicePanel.TLabel', cursor='hand2')
            below.pack(anchor=tk.W, padx=(PANEL_PAD + DETAILS_INDENT, 0), pady=(0, 9))
        # a filled arrow, larger than the text, so the row reads as one that opens
        arrow = ttk.Label(line, width=2, style='AddDevicePanel.TLabel', font=('TkDefaultFont', 11))
        arrow.pack(side=tk.LEFT)
        ttk.Button(line, text='Config\u2026', cursor='arrow',
                   command=lambda: self.device_config_edit(info, options)).pack(side=tk.RIGHT)
        name = ttk.Label(line, text=info.name, style='AddDevicePanel.TLabel', font=('TkDefaultFont', 9, 'bold'))
        name.pack(side=tk.LEFT)
        string = ttk.Label(line, text=info.connection_string, style='AddDeviceMuted.TLabel', font=('Consolas', 8))
        string.pack(side=tk.LEFT, padx=(12, 0))
        details = ttk.Frame(frame, style='AddDevicePanel.TFrame', padding=(PANEL_PAD + DETAILS_INDENT, 4, PANEL_PAD, 14))
        # the scrollbar takes its width from the rows, so the details wrap short of it
        self.info_details_draw(details, info, options, DETAILS_INDENT + SCROLLBAR_ROOM)

        def show(opened):
            arrow.configure(text='▼' if opened else '▶')
            if opened:
                details.pack(fill=tk.X, after=below)
            else:
                details.pack_forget()

        def toggle(event):
            key = info.connection_string
            if key in self.opened_lines:
                self.opened_lines.discard(key)
            else:
                self.opened_lines.add(key)
            show(key in self.opened_lines)

        for widget in {line, arrow, name, string, below}:
            widget.bind('<Button-1>', toggle)
        # a line stays open while the selection changes around it
        show(info.connection_string in self.opened_lines)

    def device_config_edit(self, info, options):
        """Add with config for one of the selected devices, kept for the Add. The window works on a copy,
        so Cancel leaves the device as it was."""
        copy = AddDeviceContext(self.context, self.dialog_parent_device, info)
        if not copy.connect_options:
            return
        if options is not None and options.connect is not None and copy.copy_choices_from(options):
            copy.config.server_fingerprint = options.config.server_fingerprint  # the same host, so its pin still holds
        dialog = AddConfigDialog(self, self.context, [copy], confirm_text='OK')
        dialog.show()
        if not dialog.confirmed:
            return
        self.own_configs[info.connection_string] = copy
        self.selection_draw()

    def own_config_names(self):
        return [o.info.name for o in self.options if o.info is not None and
                self.own_configs.get(o.info.connection_string) is o]

    def info_typed_draw(self, typed):
        frame = self.content
        ttk.Label(frame, text='Typed connection string', style='AddDeviceHeader.TLabel').pack(anchor=tk.W)
        self.wrap(ttk.Label(frame, text=typed, font=('Consolas', 8), style='AddDevicePanel.TLabel')).pack(
            anchor=tk.W, pady=(0, 6))
        if stub.has_user_part(typed):
            ttk.Label(frame, text="A login in the connection string ('user:password@') is not allowed. "
                                  'Remove it and enter the login in Add with config', foreground=RED,
                      wraplength=300, style='AddDevicePanel.TLabel').pack(anchor=tk.W)
        elif self.options and self.options[0].connect_options:
            ttk.Label(frame, text='The prefix picks the type', style='AddDeviceMuted.TLabel').pack(anchor=tk.W)
        elif typed.startswith('daq://'):
            ttk.Label(frame, text="'daq://' is only for discovered devices", foreground=RED, style='AddDevicePanel.TLabel').pack(anchor=tk.W)
        else:
            ttk.Label(frame, text='No loaded module accepts this prefix', foreground=RED, style='AddDevicePanel.TLabel').pack(anchor=tk.W)

    def handle_connect_selected(self, type_id):
        self.connect_value = type_id
        own = self.own_config_names()
        own_ids = set(map(id, self.own_configs.values()))
        shared = [o for o in self.options if id(o) not in own_ids]
        own_text = f', {", ".join(own)} with {"its" if len(own) == 1 else "their"} own config' if own else ''
        if type_id == EACH_DEFAULT:
            for options in shared:
                options.select_connect(options.connect_options[0].type_id)
            self.connects_to.configure(text=f'{len(self.options)} devices, each over its first type{own_text}')
        elif type_id is not None:
            for options in shared:
                if options.connect.type_id != type_id:
                    options.select_connect(type_id)
            self.connects_to.configure(text=f'Connects to {self.options[0].connection_string()}'
                                       if len(self.options) == 1 else f'{len(self.options)} devices{own_text}')
        elif len(self.options) > 1 and not shared:
            self.connects_to.configure(text=f'{len(self.options)} devices, each with its own config')
        else:
            self.connects_to.configure(text='')
        all_own = len(self.options) > 1 and not shared
        # 0004: a connection string with a user part fails the add
        user_part = not self.selected_infos() and stub.has_user_part(self.conn_string_var.get().strip())
        self.add_button.state(['!disabled' if (type_id is not None or all_own) and not user_part else 'disabled'])
        # one config window needs one type to show the settings of
        self.config_button.state(['!disabled' if type_id not in (None, EACH_DEFAULT) and not user_part
                                  else 'disabled'])

    # MARK: - Adding
    def handle_device_tree_double_click(self, event):
        self.process_add_device(False)

    def handle_entry_enter(self, event):
        self.process_add_device(False)

    def process_add_device(self, open_config_dialog: bool):
        if not self.options or any(o.connect is None for o in self.options):
            return
        if open_config_dialog and self.connect_value == EACH_DEFAULT:
            return
        parent_top = self.parent.winfo_toplevel()
        self.configure(cursor='watch')
        parent_top.configure(cursor='watch')
        # Defer work so Tkinter finishes the current event (repaint cursor) before blocking
        self.after(0, lambda: self._run_add_device(open_config_dialog, parent_top))

    def _run_add_device(self, open_config_dialog: bool, parent_top):
        options = self.options
        if open_config_dialog:
            self.configure(cursor='')
            parent_top.configure(cursor='')
            rest = self.options_without_own()
            if not rest:
                return
            dialog = AddConfigDialog(self, self.context, rest)
            dialog.show()
            if not dialog.confirmed:
                return
            # a device given its own config with its row's Config... keeps it
            for other in rest[1:]:
                other.copy_choices_from(rest[0])
        self.add_devices(options)
        for o in options:
            if o.info is not None:
                self.own_configs.pop(o.info.connection_string, None)
        try:
            self.configure(cursor='')
        except tk.TclError:
            pass
        try:
            parent_top.configure(cursor='')
        except tk.TclError:
            pass

    def add_devices(self, options_list):
        """Adds every device with its config. The provider asks each login in turn, then the devices
        connect in parallel."""
        if self.dialog_parent_device is None:
            return
        parent_top = self.parent.winfo_toplevel()
        stub.set_credential_provider(GuiCredentialProvider(self, self.context))
        configs = {options.connection_string(): options.config for options in options_list}
        self.configure(cursor='watch')
        parent_top.configure(cursor='watch')
        self.update_idletasks()
        try:
            added, errors = stub.add_devices(self.dialog_parent_device, configs)
        finally:
            self.configure(cursor='')
            parent_top.configure(cursor='')
        failed = [f'{address}: {message}' for address, message in errors.items()]

        warnings = []
        for device in added.values():
            warnings += [f'{device.name}: {w}' for w in stub.WARNINGS.get(device.global_id, [])]
        if added:
            self.context.selected_node = list(added.values())[-1]
            self.event_port.emit()
        if failed:
            utils.show_error('Error adding device', '\n'.join(failed + warnings), self)
            return
        if warnings:
            messagebox.showwarning('Added with warnings', '\n'.join(warnings), parent=self)
        if not added:
            return
        if self._keep_open_var.get():
            self.update_child_devices(self.device_tree, self.dialog_parent_device)
        else:
            self.close()

    def handle_right_click(self, event):
        iid = self.device_tree.identify_row(event.y)
        if iid and iid not in self.device_tree.selection():
            utils.treeview_select_item(self.device_tree, event)

        menu = tk.Menu(self, tearoff=0)
        menu.add_command(label='Add',
                         image=self.context.menu_icon('add'), compound=tk.LEFT,
                         command=lambda: self.process_add_device(open_config_dialog=False))
        menu.add_command(label='Add with config',
                         image=self.context.menu_icon('settings'), compound=tk.LEFT,
                         command=lambda: self.process_add_device(open_config_dialog=True))
        menu.add_separator()
        menu.add_command(label='Device Info',
                         image=self.context.menu_icon('device'), compound=tk.LEFT,
                         command=self.handle_show_device_info)
        menu.tk_popup(event.x_root, event.y_root)

    def find_available_device(self, parent_device: daq.IDevice, connection_string):
        if parent_device is None or not connection_string:
            return None
        for info in self.infos.values():
            if info.connection_string == connection_string:
                return info
        return None

    def handle_show_device_info(self):
        infos = self.selected_infos()
        if len(infos) != 1:
            return
        DeviceInfoDialog(self, infos[0], self.context).show()

    # MARK: - Discovery
    def update_child_devices(self, tree, parent_device):
        self._search_generation += 1
        generation = self._search_generation

        tree.delete(*tree.get_children())
        self.infos = {}

        if parent_device is None:
            return

        tree.insert('', tk.END, iid='__loading__',
                    values=('Searching for devices…', ''))
        tree.configure(cursor='watch')

        def fetch():
            try:
                devices = [daq.IDeviceInfo.cast_from(d) for d in parent_device.available_devices]
            except Exception:
                devices = []
            self.after(0, lambda: self._on_devices_loaded(tree, devices, generation))

        threading.Thread(target=fetch, daemon=True).start()

    def _on_devices_loaded(self, tree, devices, generation):
        if generation != self._search_generation:
            return  # a newer search has started; discard stale results
        try:
            tree.delete(*tree.get_children())
            tree.configure(cursor='')
            for info in devices:
                conn = info.connection_string
                if conn in self.infos:
                    continue
                self.infos[conn] = info
                tree.insert('', tk.END, iid=conn, values=(info.name, info.location, conn))
        except tk.TclError:
            pass  # dialog was closed before results arrived
