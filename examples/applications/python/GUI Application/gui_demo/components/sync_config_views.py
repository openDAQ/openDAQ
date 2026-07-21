import tkinter as tk
from tkinter import ttk
from tkinter import messagebox

from .. import utils
from .. import dummy_sync2component as sync_stub

TIME_PROTOCOL_NAMES = {
    sync_stub.TimeProtocol.Unknown: 'Unknown',
    sync_stub.TimeProtocol.Tai: 'TAI',
    sync_stub.TimeProtocol.Gps: 'GPS',
    sync_stub.TimeProtocol.Utc: 'UTC',
}

# Per-interface settings shown in the read-only property tables.
INTERFACE_FIELDS = {
    'PtpSyncInterface': [
        ('domain_number', 'Domain number'),
        ('transport', 'Transport protocol'),
        ('delay_mechanism', 'Delay mechanism'),
        ('log_sync_interval', 'Log sync interval'),
        ('log_announce_interval', 'Log announce interval'),
        ('priority1', 'Priority 1'),
        ('priority2', 'Priority 2'),
    ],
    'NtpSyncInterface': [
        ('servers', 'Servers'),
        ('domain_id', 'Domain id'),
        ('min_poll', 'Min poll (log2 s)'),
        ('max_poll', 'Max poll (log2 s)'),
        ('iburst', 'Iburst'),
        ('version', 'NTP version'),
    ],
    'IrigSyncInterface': [
        ('source_id', 'Source id'),
    ],
}

KIND_INTERFACES = {
    'ptp': 'PtpSyncInterface',
    'ntp': 'NtpSyncInterface',
    'gps': 'GpsSyncInterface',
    'irig': 'IrigSyncInterface',
}


def display_value(value):
    if isinstance(value, list):
        return ', '.join(str(v) for v in value)
    return str(value)


def status_color(source_status):
    # Sync source status mapped to the block views' status square colors.
    return {
        sync_stub.SourceStatus.Synced: utils.StatusColor.OK,
        sync_stub.SourceStatus.Listening: utils.StatusColor.WARNING,
        sync_stub.SourceStatus.Calibrating: utils.StatusColor.WARNING,
        sync_stub.SourceStatus.Error: utils.StatusColor.ERROR,
    }.get(source_status, utils.StatusColor.NOT_SET)


class _DetailFrame(ttk.Frame):
    # Panels styled like the block views: a header row with the name and a
    # clickable status container, and a read-only property table below.
    # on_change tells the owning view to refresh after modifications.

    def __init__(self, parent, context, title, on_change, **kwargs):
        ttk.Frame.__init__(self, parent, **kwargs)
        self.context = context
        self.on_change = on_change
        self.header = ttk.Frame(self)
        self.header.pack(fill=tk.X, padx=10, pady=(10, 5))
        ttk.Label(self.header, text=title,
                  font=('TkDefaultFont', 11, 'bold')).pack(side=tk.LEFT)

    def _header_status(self, color, text, on_click=None):
        # Colored square + message, like the block view status container;
        # clicking opens the status dialog.
        ttk.Label(self.header, text=' | ').pack(side=tk.LEFT)
        cursor = 'hand2' if on_click else ''
        frame = tk.Frame(self.header, cursor=cursor)
        frame.pack(side=tk.LEFT)
        square = tk.Frame(frame, width=10, height=10, bg=str(color),
                          cursor=cursor)
        square.pack_propagate(False)
        square.pack(side=tk.LEFT, padx=(0, 4))
        message = tk.Label(frame, text=text, cursor=cursor)
        message.pack(side=tk.LEFT)

        if on_click is None:
            return

        def _enter(e):
            for w in (frame, message):
                w.configure(bg='#e0e0e0')

        def _leave(e):
            bg = self.header.winfo_rgb(ttk.Style().lookup('TFrame', 'background'))
            bg_hex = '#{:04x}{:04x}{:04x}'.format(*bg)
            for w in (frame, message):
                w.configure(bg=bg_hex)

        for widget in (frame, square, message):
            widget.bind('<Button-1>', lambda e: on_click())
            widget.bind('<Enter>', _enter)
            widget.bind('<Leave>', _leave)

    def _header_text(self, text):
        ttk.Label(self.header, text=f' | {text}').pack(side=tk.LEFT)

    def _property_table(self, rows):
        # Read-only two-column table, like the properties view; read-only
        # values are gray, matching the property view's readonly tag.
        frame = ttk.Frame(self)
        frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=5)
        tree = ttk.Treeview(frame, columns=('value',), show='tree headings')
        tree.heading('#0', anchor=tk.W, text='Property name')
        tree.heading('value', anchor=tk.W, text='Value')
        dpi = self.context.dpi_factor
        tree.column('#0', anchor=tk.W, minwidth=50, width=int(200 * dpi),
                    stretch=True)
        tree.column('value', anchor=tk.W, minwidth=50, width=int(200 * dpi),
                    stretch=True)
        tree.tag_configure('readonly', foreground='gray')
        scroll = ttk.Scrollbar(frame, orient=tk.VERTICAL, command=tree.yview)
        tree.configure(yscroll=scroll.set)
        tree.pack(fill=tk.BOTH, expand=True, side=tk.LEFT)
        scroll.pack(fill=tk.Y, side=tk.RIGHT)
        for label, value in rows:
            tree.insert('', tk.END, text=label, values=(value,),
                        tags=('readonly',))
        _bind_copy(tree)
        return tree

    def _show_status_dialog(self, title, statuses, changes):
        # Statuses and the latest changes, like the block views' status
        # window. Ctrl+C copies the selected row.
        dpi = self.context.dpi_factor if self.context else 1.0
        w, h = int(600 * dpi), int(300 * dpi)
        window = tk.Toplevel(self)
        window.withdraw()
        window.title(title)
        window.attributes('-topmost', True)
        window.transient(self)
        window.bind('<Escape>', lambda e: window.destroy())

        status_tree = ttk.Treeview(window, columns=('Name', 'Status', 'Message'),
                                   show='headings', height=len(statuses))
        for col in ('Name', 'Status', 'Message'):
            status_tree.heading(col, text=col, anchor=tk.W)
        for name, value, message in statuses:
            status_tree.insert('', tk.END, values=(name, value, message))
        status_tree.pack(fill=tk.X, padx=5, pady=(5, 0))
        _bind_copy(status_tree)

        ttk.Label(window, text='Latest changes').pack(anchor=tk.W, padx=5,
                                                      pady=(8, 0))
        frame = ttk.Frame(window)
        frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        change_tree = ttk.Treeview(frame, columns=('Time', 'Change'),
                                   show='headings')
        change_tree.heading('Time', text='Time', anchor=tk.W)
        change_tree.heading('Change', text='Change', anchor=tk.W)
        change_tree.column('Time', width=int(90 * dpi), stretch=False)
        scroll = ttk.Scrollbar(frame, orient=tk.VERTICAL,
                               command=change_tree.yview)
        change_tree.configure(yscroll=scroll.set)
        change_tree.pack(fill=tk.BOTH, expand=True, side=tk.LEFT)
        scroll.pack(fill=tk.Y, side=tk.RIGHT)
        for stamp, sender, prop, value in changes:
            change_tree.insert('', tk.END, values=(
                f'{stamp:%H:%M:%S}', f'{sender}: {prop} -> {value}'))
        _bind_copy(change_tree)

        main = self.winfo_toplevel()
        window.update_idletasks()
        x = main.winfo_rootx() + main.winfo_width() // 2 - w // 2
        y = main.winfo_rooty() + main.winfo_height() // 2 - h // 2
        window.geometry(f'{w}x{h}+{x}+{y}')
        window.deiconify()

    def _changed(self):
        if self.on_change is not None:
            self.on_change()

    def _error(self, title, error):
        messagebox.showerror(title, str(error), parent=self)


def _bind_copy(tree):
    # Ctrl+C copies the selected row as tab-separated text.
    def copy(event=None):
        selection = tree.selection()
        if not selection:
            return
        item = tree.item(selection[0])
        parts = ([item['text']] if item['text'] else []) + \
            [str(v) for v in item['values']]
        tree.clipboard_clear()
        tree.clipboard_append('\t'.join(parts))
        return 'break'

    tree.bind('<Control-c>', copy)


def _device_changes(event_log, gid, sender=None):
    # Newest first; entries are (stamp, gid, sender, prop, value).
    changes = []
    for stamp, entry_gid, entry_sender, prop, value in reversed(event_log):
        if entry_gid != gid:
            continue
        if sender is not None and entry_sender != sender:
            continue
        changes.append((stamp, entry_sender, prop, value))
    return changes


class GroupDetailView(_DetailFrame):
    # Read-only: a working group is not something to reconfigure. Any change
    # breaks the group apart until the network re-forms, and partial
    # application would leave members with different settings.

    def __init__(self, parent, context, group_name, members, master_gid,
                 kind, reference_id, **kwargs):
        # members: list of (device, sync) pairs, master first.
        _DetailFrame.__init__(self, parent, context, group_name, None,
                              **kwargs)
        master_name = 'none (external or local reference)'
        for device, sync in members:
            if device.global_id == master_gid:
                master_name = device.name
                break

        rows = [
            ('Master', master_name),
            ('Devices', ', '.join(d.name for d, s in members)),
        ]
        if reference_id:
            rows.append(('Reference domain', reference_id))

        iface = self._reference_interface(members, master_gid, kind)
        if iface is not None and kind in ('ptp', 'ntp', 'irig'):
            for attr, label in INTERFACE_FIELDS.get(type(iface).__name__, []):
                rows.append((label, display_value(getattr(iface, attr))))

        self._property_table(rows)

    def _reference_interface(self, members, master_gid, kind):
        name = KIND_INTERFACES.get(kind)
        if name is None:
            return None
        ordered = sorted(members, key=lambda p: p[0].global_id != master_gid)
        for device, sync in ordered:
            iface = sync.get_sync_interfaces().get(name)
            if iface is not None:
                return iface
        return None


class DeviceConfigView(_DetailFrame):
    # Header status container (click for statuses and latest changes), a
    # configurable source, and the sync component's state as a read-only
    # property table.

    def __init__(self, parent, context, device, sync, event_log, on_change,
                 real_info=None, **kwargs):
        _DetailFrame.__init__(self, parent, context, device.name, on_change,
                              **kwargs)
        self.device = device
        self.sync = sync
        self.event_log = event_log

        source = sync.get_source()
        status = source.get_status_container()
        source_status = status.get_status('SourceStatus')
        message = status.get_status_message('SourceStatus')
        self._header_status(status_color(source_status),
                            source_status + (f' - {message}' if message else ''),
                            on_click=self._show_statuses)

        row = ttk.Frame(self)
        row.pack(fill=tk.X, padx=10, pady=(4, 2))
        ttk.Label(row, text='Source:').pack(side=tk.LEFT, padx=(0, 5))
        sources = [name for name, iface in sync.get_available_sources().items()
                   if not iface.get_output_only()]
        self.source_var = tk.StringVar(value=source.name)
        box = ttk.Combobox(row, textvariable=self.source_var, values=sources,
                           state='readonly', width=24)
        box.pack(side=tk.LEFT)
        box.bind('<<ComboboxSelected>>', lambda e: self._commit_source())

        # A connected device reports its own reference domain info; only
        # devices that report nothing fall back to the dummy component.
        if real_info is not None:
            rows = [
                ('Applied reference domain', real_info['id']),
                ('Domain offset (ticks)', real_info['offset']),
                ('Time protocol', real_info['protocol']),
            ]
        else:
            info = sync.get_reference_domain_info()
            rows = [
                ('Applied reference domain',
                 info.get_source_reference_domain_id()),
                ('All reference domains',
                 ', '.join(info.get_reference_domain_ids())),
                ('Domain offset (ticks)',
                 str(info.get_reference_domain_offset())),
                ('Time protocol', TIME_PROTOCOL_NAMES.get(
                    info.get_reference_time_protocol(), '?')),
            ]
        self._property_table(rows)

    def _show_statuses(self):
        source = self.sync.get_source()
        container = source.get_status_container()
        statuses = []
        for name in ('SourceStatus', 'RoleStatus'):
            statuses.append((f'{source.name}.{name}',
                             container.get_status(name),
                             container.get_status_message(name)))
        self._show_status_dialog(f'Statuses - {self.device.name}', statuses,
                                 _device_changes(self.event_log,
                                                 self.device.global_id))

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
    # Status and role in the header container (click for the latest
    # changes), the stub-only simulate controls aligned at the top, and the
    # configuration as a read-only property table.

    def __init__(self, parent, context, device, sync, iface, on_change,
                 event_log=None, **kwargs):
        _DetailFrame.__init__(self, parent, context,
                              f'{iface.name} ({device.name})', on_change,
                              **kwargs)
        self.device = device
        self.sync = sync
        self.iface = iface
        self.event_log = event_log if event_log is not None else []

        status = iface.get_status_container()
        source_status = status.get_status('SourceStatus')
        message = status.get_status_message('SourceStatus')
        self._header_status(status_color(source_status),
                            source_status + (f' - {message}' if message else ''),
                            on_click=self._show_statuses)
        if iface is sync.get_source():
            self._header_text('selected source')

        # Stub-only simulation controls, aligned at the top.
        sim = ttk.Frame(self)
        sim.pack(fill=tk.X, padx=10, pady=(0, 5))
        ttk.Label(sim, text='Simulate:').pack(side=tk.LEFT, padx=(0, 5))
        ttk.Button(sim, text='Upstream loss',
                   command=lambda: self._simulate(iface.simulate.upstream_loss)).pack(
            side=tk.LEFT)
        ttk.Button(sim, text='Upstream restored',
                   command=lambda: self._simulate(iface.simulate.upstream_restored)).pack(
            side=tk.LEFT, padx=5)
        if sync_stub.SyncMode.Auto in iface.get_available_modes():
            ttk.Label(sim, text='Auto role').pack(side=tk.LEFT, padx=(10, 2))
            self.role_var = tk.StringVar(value=iface.simulate.auto_role)
            role_box = ttk.Combobox(sim, textvariable=self.role_var,
                                    values=(sync_stub.RoleStatus.Input,
                                            sync_stub.RoleStatus.Output,
                                            sync_stub.RoleStatus.Unknown),
                                    state='readonly', width=8)
            role_box.pack(side=tk.LEFT)
            role_box.bind('<<ComboboxSelected>>',
                          lambda e: self._simulate(self._commit_auto_role))

        rows = [
            ('Mode', sync_stub.MODE_NAMES[iface.get_mode()]),
            ('Available modes', ', '.join(
                sync_stub.MODE_NAMES[m] for m in iface.get_available_modes())),
            ('Output only', str(iface.get_output_only())),
            ('Reference domain', iface.get_reference_domain_id() or '(none)'),
        ]
        for attr, label in INTERFACE_FIELDS.get(type(iface).__name__, []):
            rows.append((label, display_value(getattr(iface, attr))))
        self._property_table(rows)

    def _show_statuses(self):
        container = self.iface.get_status_container()
        statuses = []
        for name in ('SourceStatus', 'RoleStatus'):
            statuses.append((name, container.get_status(name),
                             container.get_status_message(name)))
        self._show_status_dialog(f'Statuses - {self.iface.name}', statuses,
                                 _device_changes(self.event_log,
                                                 self.device.global_id,
                                                 sender=self.iface.name))

    def _commit_auto_role(self):
        self.iface.simulate.auto_role = self.role_var.get()

    def _simulate(self, action):
        action()
        self._changed()
