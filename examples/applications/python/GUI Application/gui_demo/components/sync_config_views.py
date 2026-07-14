import tkinter as tk
from tkinter import ttk
from tkinter import messagebox

from .. import dummy_sync2component as sync_stub

TIME_PROTOCOL_NAMES = {
    sync_stub.TimeProtocol.Unknown: 'Unknown',
    sync_stub.TimeProtocol.Tai: 'TAI',
    sync_stub.TimeProtocol.Gps: 'GPS',
    sync_stub.TimeProtocol.Utc: 'UTC',
}

# Editable per-interface settings: (attribute, label, combobox choices);
# None means a free-text entry converted to the attribute's current type.
INTERFACE_FIELDS = {
    'PtpSyncInterface': [
        ('domain_number', 'Domain number', None),
        ('transport', 'Transport protocol', (sync_stub.PtpTransport.L2,
                                             sync_stub.PtpTransport.UDP_IPV4,
                                             sync_stub.PtpTransport.UDP_IPV6)),
        ('delay_mechanism', 'Delay mechanism', (sync_stub.PtpDelayMechanism.E2E,
                                                sync_stub.PtpDelayMechanism.P2P)),
        ('log_sync_interval', 'Log sync interval', None),
        ('log_announce_interval', 'Log announce interval', None),
        ('priority1', 'Priority 1', None),
        ('priority2', 'Priority 2', None),
    ],
    'NtpSyncInterface': [
        ('servers', 'Servers (comma separated)', None),
        ('domain_id', 'Domain id', None),
        ('min_poll', 'Min poll (log2 s)', None),
        ('max_poll', 'Max poll (log2 s)', None),
        ('iburst', 'Iburst', ('True', 'False')),
        ('version', 'NTP version', ('3', '4')),
    ],
    'IrigSyncInterface': [
        ('source_id', 'Source id', None),
    ],
}

# Same look as the Modules tab detail panel.
HEADER_FONT = ("TkDefaultFont", 13, "bold")
SECTION_FONT = ("TkDefaultFont", 10, "bold")
LABEL_WIDTH = 24


def display_value(value):
    if isinstance(value, list):
        return ', '.join(str(v) for v in value)
    return str(value)


def convert_like(current, raw):
    # Editor string back to the attribute's current type; setters validate.
    if isinstance(current, bool):
        return raw == 'True'
    if isinstance(current, list):
        return [p.strip() for p in str(raw).split(',') if p.strip()]
    if isinstance(current, int):
        stripped = str(raw).strip()
        if not stripped.lstrip('-').isdigit():
            raise ValueError(f'not a number: {raw!r}')
        return int(stripped)
    return str(raw)


class _DetailFrame(ttk.Frame):
    # Shared scaffolding: bold header, "Label:" rows, separated sections.
    # on_change tells the owning view to refresh after modifications.

    def __init__(self, parent, context, title, on_change, subtitle='',
                 **kwargs):
        ttk.Frame.__init__(self, parent, **kwargs)
        self.context = context
        self.on_change = on_change
        ttk.Label(self, text=title, font=HEADER_FONT).pack(
            anchor=tk.W, padx=10, pady=(10, 0 if subtitle else 5))
        if subtitle:
            ttk.Label(self, text=subtitle, foreground='gray',
                      wraplength=400).pack(anchor=tk.W, padx=10, pady=(0, 5))

    def _info_row(self, label, value, parent=None):
        row = ttk.Frame(parent or self)
        row.pack(fill=tk.X, padx=10, pady=1)
        ttk.Label(row, text=f'{label}:', width=LABEL_WIDTH, anchor=tk.NW).pack(
            side=tk.LEFT)
        ttk.Label(row, text=str(value), anchor=tk.W, justify=tk.LEFT,
                  wraplength=380).pack(side=tk.LEFT, fill=tk.X)
        return row

    def _editor_row(self, label, editor_factory, parent=None):
        row = ttk.Frame(parent or self)
        row.pack(fill=tk.X, padx=10, pady=2)
        ttk.Label(row, text=f'{label}:', width=LABEL_WIDTH, anchor=tk.W).pack(
            side=tk.LEFT)
        editor = editor_factory(row)
        editor.pack(side=tk.LEFT)
        return editor

    def _section(self, title, parent=None):
        host = parent or self
        ttk.Separator(host, orient=tk.HORIZONTAL).pack(
            fill=tk.X, padx=10, pady=(10, 5))
        ttk.Label(host, text=title, font=SECTION_FONT).pack(
            anchor=tk.W, padx=10, pady=(0, 5))

    def _changed(self):
        if self.on_change is not None:
            self.on_change()

    def _error(self, title, error):
        messagebox.showerror(title, str(error), parent=self)


class GroupConfigView(_DetailFrame):
    # One source applied to the whole group. Elected mode lets BMCA decide
    # (prefer_auto); designated mode makes one device's PTP output-only and
    # sets everyone else to Input.

    def __init__(self, parent, context, group_name, members, master_gid,
                 on_change, **kwargs):
        # members: list of (device, sync) pairs.
        _DetailFrame.__init__(self, parent, context, group_name, on_change,
                              subtitle='Synchronization group', **kwargs)
        self.members = members

        master_name = 'none (external or local reference)'
        for device, sync in members:
            if device.global_id == master_gid:
                master_name = device.name
                break
        self._info_row('Devices', ', '.join(d.name for d, s in members))
        self._info_row('Master', master_name)

        self._section('Group configuration')

        # Only sources every device offers (and that aren't output-only).
        common = None
        for device, sync in members:
            names = {name for name, iface in sync.get_available_sources().items()
                     if not iface.get_output_only()}
            common = names if common is None else common & names
        self.common_sources = sorted(common or [])

        default = 'PtpSyncInterface' if 'PtpSyncInterface' in self.common_sources \
            else (self.common_sources[0] if self.common_sources else '')
        self.source_var = tk.StringVar(value=default)

        def make_source_box(row):
            box = ttk.Combobox(row, textvariable=self.source_var,
                               values=self.common_sources, state='readonly',
                               width=24)
            box.bind('<<ComboboxSelected>>', lambda e: self._update_ptp_frame())
            return box

        self._editor_row('Source for the group', make_source_box)

        # PTP-only options; hidden for other sources.
        self.ptp_frame = ttk.Frame(self)
        self.domain_var = tk.StringVar(value='0')
        row = ttk.Frame(self.ptp_frame)
        row.pack(fill=tk.X, padx=10, pady=2)
        ttk.Label(row, text='PTP domain number:', width=LABEL_WIDTH,
                  anchor=tk.W).pack(side=tk.LEFT)
        ttk.Entry(row, textvariable=self.domain_var, width=6).pack(side=tk.LEFT)

        self.master_mode_var = tk.StringVar(value='designated' if master_gid
                                            else 'elected')
        mode_row = ttk.Frame(self.ptp_frame)
        mode_row.pack(fill=tk.X, padx=10, pady=2)
        ttk.Label(mode_row, text='Master:', width=LABEL_WIDTH,
                  anchor=tk.W).pack(side=tk.LEFT)
        ttk.Radiobutton(mode_row, text='Elected (BMCA)',
                        variable=self.master_mode_var, value='elected',
                        command=self._update_ptp_frame).pack(side=tk.LEFT)
        ttk.Radiobutton(mode_row, text='Designated',
                        variable=self.master_mode_var, value='designated',
                        command=self._update_ptp_frame).pack(side=tk.LEFT,
                                                             padx=10)

        default_master = members[0][0].name
        for device, sync in members:
            if device.global_id == master_gid:
                default_master = device.name
                break
        self.master_var = tk.StringVar(value=default_master)
        self.master_row = ttk.Frame(self.ptp_frame)
        ttk.Label(self.master_row, text='Master device:', width=LABEL_WIDTH,
                  anchor=tk.W).pack(side=tk.LEFT)
        ttk.Combobox(self.master_row, textvariable=self.master_var,
                     values=[d.name for d, s in members], state='readonly',
                     width=24).pack(side=tk.LEFT)

        self.apply_button = ttk.Button(self, text='Apply to group',
                                       command=self._apply)
        self.apply_button.pack(anchor=tk.W, padx=10, pady=(8, 10))
        self._update_ptp_frame()

    def _update_ptp_frame(self):
        if self.source_var.get() == 'PtpSyncInterface':
            self.ptp_frame.pack(fill=tk.X, before=self.apply_button)
        else:
            self.ptp_frame.pack_forget()
            return
        if self.master_mode_var.get() == 'designated':
            self.master_row.pack(fill=tk.X, padx=10, pady=2)
        else:
            self.master_row.pack_forget()

    def _apply(self):
        source_name = self.source_var.get()
        if not source_name:
            return
        is_ptp = source_name == 'PtpSyncInterface'
        designated = is_ptp and self.master_mode_var.get() == 'designated'
        errors = []

        if is_ptp:
            raw = self.domain_var.get().strip()
            if not raw.isdigit():
                self._error('Configure group', 'PTP domain number must be a number.')
                return
            domain = int(raw)

        # Master first, so its domain is up before others point at it.
        pairs = list(self.members)
        if designated:
            pairs.sort(key=lambda p: p[0].name != self.master_var.get())

        for device, sync in pairs:
            iface = sync.get_sync_interfaces().get(source_name)
            if iface is None:
                errors.append(f'{device.name}: no {source_name}')
                continue
            try:
                if is_ptp:
                    iface.domain_number = domain
                if designated and device.name == self.master_var.get():
                    # Output-only is rejected on the selected source, so the
                    # master's source moves to its local clock first.
                    if sync.get_source() is iface:
                        clock = sync.get_sync_interfaces()['ClockSyncInterface']
                        sync.set_source(clock, prefer_auto=False)
                    if not iface.get_output_only():
                        iface.set_output_only(True)
                else:
                    if iface.get_output_only():
                        iface.set_output_only(False)
                    sync.set_source(iface, prefer_auto=not designated)
            except ValueError as e:
                errors.append(f'{device.name}: {e}')

        if errors:
            self._error('Configure group', '\n'.join(errors))
        self._changed()


class DeviceConfigView(_DetailFrame):
    # Source selection, reference domain info, and recent property changes.

    def __init__(self, parent, context, device, sync, event_log, on_change,
                 reported_ref_id='', **kwargs):
        _DetailFrame.__init__(self, parent, context, device.name, on_change,
                              subtitle='Device synchronization', **kwargs)
        self.device = device
        self.sync = sync

        info = sync.get_reference_domain_info()
        if reported_ref_id:
            self._info_row('Reported by device', reported_ref_id)
        self._info_row('Applied reference domain',
                       info.get_source_reference_domain_id())
        self._info_row('All reference domains',
                       '\n'.join(info.get_reference_domain_ids()))
        self._info_row('Domain offset (ticks)',
                       str(info.get_reference_domain_offset()))
        self._info_row('Time protocol', TIME_PROTOCOL_NAMES.get(
            info.get_reference_time_protocol(), '?'))

        self._section('Source')
        sources = [name for name, iface in sync.get_available_sources().items()
                   if not iface.get_output_only()]
        self.source_var = tk.StringVar(value=sync.get_source().name)

        def make_source_box(row):
            box = ttk.Combobox(row, textvariable=self.source_var,
                               values=sources, state='readonly', width=24)
            box.bind('<<ComboboxSelected>>', lambda e: self._commit_source())
            return box

        self._editor_row('Selected source', make_source_box)

        changes = [entry for entry in reversed(event_log)
                   if entry[1] == device.global_id][:8]
        if changes:
            self._section('Recent changes')
            for stamp, gid, sender, prop, value in changes:
                ttk.Label(self,
                          text=f'{stamp:%H:%M:%S}  {sender}: {prop} -> {value}',
                          foreground='gray').pack(anchor=tk.W, padx=10)

    def _commit_source(self):
        iface = self.sync.get_available_sources().get(self.source_var.get())
        if iface is None or iface is self.sync.get_source():
            return
        try:
            self.sync.set_source(iface)
        except ValueError as e:
            self._error('Select source', e)
        self._changed()


class InterfaceConfigView(_DetailFrame):
    # Mode and output-only through the public API, the interface's protocol
    # settings, and the stub-only knobs under iface.simulate.

    def __init__(self, parent, context, device, sync, iface, on_change,
                 **kwargs):
        _DetailFrame.__init__(self, parent, context, iface.name, on_change,
                              subtitle=f'Sync interface on {device.name}',
                              **kwargs)
        self.sync = sync
        self.iface = iface

        status = iface.get_status_container()
        source_status = status.get_status('SourceStatus')
        message = status.get_status_message('SourceStatus')
        role = status.get_status('RoleStatus')
        self._info_row('Status', source_status + (f' ({message})' if message
                                                  else ''))
        self._info_row('Role', role)
        self._info_row('Selected source',
                       'Yes' if iface is sync.get_source() else 'No')

        self._section('Configuration')

        # Input/Auto select the interface as source; Output = output-only;
        # Off releases output-only (a non-source interface is Off anyway).
        modes = [sync_stub.MODE_NAMES[m] for m in iface.get_available_modes()]
        self.mode_var = tk.StringVar(value=sync_stub.MODE_NAMES[iface.get_mode()])

        def make_mode_box(row):
            box = ttk.Combobox(row, textvariable=self.mode_var, values=modes,
                               state='readonly', width=10)
            box.bind('<<ComboboxSelected>>', lambda e: self._commit_mode())
            return box

        self._editor_row('Source mode', make_mode_box)

        self.output_var = tk.BooleanVar(value=iface.get_output_only())
        if sync_stub.SyncMode.Output in iface.get_available_modes():
            self._editor_row(
                'Output only',
                lambda row: ttk.Checkbutton(row,
                                            text='distribute this clock',
                                            variable=self.output_var,
                                            command=self._commit_output_only))

        self._fields = {}
        fields = INTERFACE_FIELDS.get(type(iface).__name__, [])
        for attr, label, choices in fields:
            var = tk.StringVar(value=display_value(getattr(iface, attr)))

            def make_editor(row, var=var, attr=attr, choices=choices):
                if choices is not None:
                    editor = ttk.Combobox(row, textvariable=var, values=choices,
                                          state='readonly', width=16)
                    editor.bind('<<ComboboxSelected>>',
                                lambda e, a=attr: self._commit_field(a))
                else:
                    editor = ttk.Entry(row, textvariable=var, width=18)
                    editor.bind('<Return>', lambda e, a=attr: self._commit_field(a))
                return editor

            self._editor_row(label, make_editor)
            self._fields[attr] = var

        if fields:
            ttk.Button(self, text='Apply settings', command=self._commit_all).pack(
                anchor=tk.W, padx=10, pady=(6, 2))

        self._section('Simulate (stub-only)')
        reachable = 'reachable' if iface.simulate.upstream_reachable else 'unreachable'
        self._info_row('Upstream time source', reachable)
        buttons = ttk.Frame(self)
        buttons.pack(fill=tk.X, padx=10, pady=2)
        ttk.Button(buttons, text='Upstream loss',
                   command=lambda: self._simulate(iface.simulate.upstream_loss)).pack(
            side=tk.LEFT)
        ttk.Button(buttons, text='Upstream restored',
                   command=lambda: self._simulate(iface.simulate.upstream_restored)).pack(
            side=tk.LEFT, padx=5)
        if sync_stub.SyncMode.Auto in iface.get_available_modes():
            self.role_var = tk.StringVar(value=iface.simulate.auto_role)

            def make_role_box(row):
                box = ttk.Combobox(row, textvariable=self.role_var,
                                   values=(sync_stub.RoleStatus.Input,
                                           sync_stub.RoleStatus.Output,
                                           sync_stub.RoleStatus.Unknown),
                                   state='readonly', width=10)
                box.bind('<<ComboboxSelected>>',
                         lambda e: self._simulate(self._commit_auto_role))
                return box

            self._editor_row('Negotiated role while Auto', make_role_box)

    # MARK: - Commits

    def _commit_mode(self):
        wanted = self.mode_var.get()
        try:
            if wanted == 'Auto':
                self.sync.set_source(self.iface, prefer_auto=True)
            elif wanted == 'Input':
                self.sync.set_source(self.iface, prefer_auto=False)
            elif wanted == 'Output':
                self.iface.set_output_only(True)
            elif wanted == 'Off':
                if self.iface is self.sync.get_source():
                    raise ValueError('the selected source cannot be turned off; '
                                     'select another source first')
                self.iface.set_output_only(False)
        except ValueError as e:
            self._error('Source mode', e)
        self._changed()

    def _commit_output_only(self):
        try:
            self.iface.set_output_only(self.output_var.get())
        except ValueError as e:
            self._error('Output only', e)
        self._changed()

    def _commit_field(self, attr):
        try:
            current = getattr(self.iface, attr)
            setattr(self.iface, attr, convert_like(current, self._fields[attr].get()))
        except ValueError as e:
            self._error('Invalid value', f'{attr}: {e}')
        self._changed()

    def _commit_all(self):
        errors = []
        for attr, var in self._fields.items():
            try:
                current = getattr(self.iface, attr)
                converted = convert_like(current, var.get())
                if converted != current:
                    setattr(self.iface, attr, converted)
            except ValueError as e:
                errors.append(f'{attr}: {e}')
        if errors:
            self._error('Invalid values', '\n'.join(errors))
        self._changed()

    def _commit_auto_role(self):
        self.iface.simulate.auto_role = self.role_var.get()

    def _simulate(self, action):
        action()
        self._changed()
