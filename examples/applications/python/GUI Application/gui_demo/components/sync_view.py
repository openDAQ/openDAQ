import tkinter as tk
from tkinter import ttk
from tkinter import messagebox
from datetime import datetime

from ..app_context import AppContext
from .. import dummy_sync2component as sync_stub
from .sync_graph_view import SyncGraphWindow
from .sync_config_views import (GroupConfigView, DeviceConfigView,
                                InterfaceConfigView)

try:
    import opendaq as daq
except ImportError:
    # Importable without the SDK for smoke tests.
    daq = None

# Colors of the circular status indicator at the end of each row.
STATUS_COLORS = {
    sync_stub.SourceStatus.Synced: '#2e9e4f',
    sync_stub.SourceStatus.Listening: '#d9a400',
    sync_stub.SourceStatus.Calibrating: '#d9a400',
    sync_stub.SourceStatus.Error: '#cc3333',
    sync_stub.SourceStatus.Off: '#9a9a9a',
    sync_stub.SourceStatus.Unknown: '#9a9a9a',
}

STATUS_FILTER_VALUES = ('All', sync_stub.SourceStatus.Synced,
                        sync_stub.SourceStatus.Listening,
                        sync_stub.SourceStatus.Calibrating,
                        sync_stub.SourceStatus.Error,
                        sync_stub.SourceStatus.Off,
                        sync_stub.SourceStatus.Unknown)

SOURCE_FILTER_VALUES = ('All', 'clock-sync', 'ptp', 'ntp', 'gps', 'irig',
                        'device')

INTERFACE_LABELS = {
    'ClockSyncInterface': 'Interface Clock',
    'PtpSyncInterface': 'Interface PTP',
    'NtpSyncInterface': 'Interface NTP',
    'GpsSyncInterface': 'Interface GPS',
    'IrigSyncInterface': 'Interface IRIG',
}


def _resolve_color(widget, color, fallback):
    # Resolves platform color names ('SystemWindow') into #rrggbb.
    if not color:
        return fallback
    try:
        r, g, b = widget.winfo_rgb(color)
    except tk.TclError:
        return fallback
    return f'#{r >> 8:02x}{g >> 8:02x}{b >> 8:02x}'


def _ptp_group_key(iface):
    # PTP ids end in a clock identity that differs between master and
    # followers, so PTP membership is keyed on the domain number instead.
    return f'ptp:{iface.domain_number}'


def compute_group_keys(sync):
    # Key of the applied reference plus one per active output.
    source = sync.get_source()
    applied = sync.get_reference_domain_info().get_source_reference_domain_id()
    if applied.startswith('ptp:') and isinstance(source, sync_stub.PtpSyncInterface):
        applied_key = _ptp_group_key(source)
    else:
        applied_key = applied

    keys = [applied_key]
    for iface in sync.get_sync_interfaces().values():
        if iface is source:
            continue
        role = iface.get_status_container().get_status('RoleStatus')
        if role != sync_stub.RoleStatus.Output:
            continue
        if isinstance(iface, sync_stub.PtpSyncInterface):
            keys.append(_ptp_group_key(iface))
        else:
            output_id = iface.get_reference_domain_id()
            if output_id:
                keys.append(output_id)
    return applied_key, keys


def _norm_id(ref_id):
    # 2ccf67.fffe.08ac39 and 2ccf67fffe08ac39 must compare equal.
    return ''.join(ch for ch in str(ref_id).lower() if ch.isalnum())


def real_reference_domain_id(device):
    # The id a connected device reports on its device domain; takes
    # precedence over the dummy component.
    try:
        domain = device.domain
        info = domain.reference_domain_info if domain is not None else None
        ref_id = str(info.reference_domain_id) if info is not None else ''
        return '' if ref_id == 'None' else ref_id
    except Exception:
        return ''


def owns_reference_domain(device, ref_id):
    # The device whose serial number or MAC appears in the id produces it.
    # PTP clock identities are EUI-64: the MAC with fffe in the middle.
    norm = _norm_id(ref_id)
    if not norm:
        return False
    candidates = {norm, norm.replace('fffe', '', 1)}

    tokens = set()
    try:
        info = device.info
    except Exception:
        info = None
    for attr in ('serial_number', 'mac_address'):
        value = getattr(info, attr, '') if info is not None else ''
        token = _norm_id(value) if value else ''
        if len(token) >= 6:
            tokens.add(token)
    local_id = _norm_id(getattr(device, 'local_id', '') or '')
    if len(local_id) >= 6:
        tokens.add(local_id)

    return any(c in t or t in c for c in candidates for t in tokens)


def device_group_keys(device, sync):
    # Returns (applied id for display, applied group key, all keys, owns).
    applied = sync.get_reference_domain_info().get_source_reference_domain_id()
    applied_key, keys = compute_group_keys(sync)
    owns = False
    real_id = real_reference_domain_id(device)
    if real_id:
        real_key = 'dev-ref:' + _norm_id(real_id)
        keys = keys + [real_key]
        applied = real_id
        applied_key = real_key
        owns = owns_reference_domain(device, real_id)
    return applied, applied_key, keys, owns


def source_kind(applied):
    # Reference type for the Source filter; EUI-64 ids count as PTP.
    scheme = applied.split(':', 1)[0]
    if scheme in ('clock-sync', 'ptp', 'ntp', 'gps', 'irig'):
        return scheme
    if 'fffe' in _norm_id(applied):
        return 'ptp'
    return 'device'


def build_sync_groups(rows):
    # Union-find over each device's group keys; returns group dicts with
    # key, member gids (master first), master_gid, root, local.
    parent = {}

    def find(x):
        parent.setdefault(x, x)
        while parent[x] != x:
            parent[x] = parent[parent[x]]
            x = parent[x]
        return x

    def union(a, b):
        parent[find(a)] = find(b)

    for row in rows:
        keys = sorted({k for k in row['all_keys'] if k})
        for other in keys[1:]:
            union(keys[0], other)

    clusters = {}
    for row in rows:
        clusters.setdefault(find(row['applied_key']), []).append(row)

    groups = []
    local_members = []
    for members in clusters.values():
        members.sort(key=lambda r: (not (r['has_output'] or r['owns_applied']),
                                    r['gid']))
        if (len(members) == 1 and members[0]['is_local']
                and not members[0]['has_output']):
            local_members.append(members[0])
            continue
        first = members[0]
        master_gid = first['gid'] if (first['has_output']
                                      or first['owns_applied']) else None
        root = ''
        for row in members:
            if not row['applied_key'].startswith('clock-sync:'):
                root = row['applied_key']
                break
        groups.append({
            'key': master_gid or root or members[0]['gid'],
            'members': [r['gid'] for r in members],
            'master_gid': master_gid,
            'root': root,
            'local': False,
        })

    if local_members:
        groups.append({
            'key': 'local',
            'members': [r['gid'] for r in local_members],
            'master_gid': None,
            'root': '',
            'local': True,
        })
    return groups


class SyncView(ttk.Frame):
    # Synchronization tab: left a tree of groups (named by their master),
    # devices, and interfaces; right a config panel filled by the selection.

    def __init__(self, parent, context: AppContext, **kwargs):
        ttk.Frame.__init__(self, parent, **kwargs)
        self.context = context

        # Cached on the context so state survives tab switches.
        if not hasattr(context, 'sync_components'):
            context.sync_components = {}
            context.sync_event_log = []

        self._devices = {}
        self._rows = {}
        self._groups = {}
        self._built = False
        self._detail_key = None
        self._search_var = tk.StringVar()
        self._source_filter_var = tk.StringVar(value='All')
        self._state_filter_var = tk.StringVar(value='All')

        self._build_panes()
        self._collect_devices()
        self.refresh()

        # Debounced search.
        self._search_after_id = None
        self._search_var.trace_add('write', self._on_search_changed)

    # MARK: - Data access

    def _sync_for(self, device):
        gid = device.global_id
        if gid not in self.context.sync_components:
            sync = sync_stub.get_synchronization(device)
            self.context.sync_components[gid] = sync

            # Subscribed once per component; touches only the context log.
            log = self.context.sync_event_log

            def record(sender, prop, value, _gid=gid):
                log.append((datetime.now(), _gid, sender.name, prop, str(value)))
                del log[:-200]

            sync.on_property_value_changed(record)
            for iface in sync.get_sync_interfaces().values():
                iface.on_property_value_changed(record)
        return self.context.sync_components[gid]

    def _collect_devices(self):
        self._devices = {}
        instance = self.context.instance
        if daq is None:
            return

        def walk(component):
            if daq.IDevice.can_cast_from(component):
                device = daq.IDevice.cast_from(component)
                self._devices[device.global_id] = device
            if daq.IFolder.can_cast_from(component):
                for item in daq.IFolder.cast_from(component).items:
                    walk(item)

        walk(instance)

    def _device_location(self, device):
        try:
            info = device.info
            return str(info.location) if info and info.location else ''
        except Exception:
            return ''

    def _device_row(self, device):
        sync = self._sync_for(device)
        source = sync.get_source()
        status = source.get_status_container().get_status('SourceStatus')
        applied, applied_key, all_keys, owns = device_group_keys(device, sync)
        has_output = False
        for iface in sync.get_sync_interfaces().values():
            role = iface.get_status_container().get_status('RoleStatus')
            if role == sync_stub.RoleStatus.Output:
                has_output = True
                break
        return {
            'gid': device.global_id,
            'device': device,
            'sync': sync,
            'name': device.name,
            'local_id': getattr(device, 'local_id', '') or '',
            'location': self._device_location(device),
            'applied': applied,
            'applied_key': applied_key,
            'all_keys': all_keys,
            'is_local': applied.startswith('clock-sync:') or owns,
            'owns_applied': owns,
            'has_output': has_output,
            'source': source.name,
            'status': status,
            'color': STATUS_COLORS.get(status, '#9a9a9a'),
        }

    # MARK: - Toolbar

    def _build_toolbar(self, parent):
        small_font = ('TkDefaultFont', 7)
        ttk.Entry(parent, textvariable=self._search_var).pack(
            fill=tk.X, padx=(2, 8), pady=(2, 3))

        bar = ttk.Frame(parent)
        bar.pack(fill=tk.X, padx=(2, 8), pady=(0, 4))
        ttk.Label(bar, text='Source', font=small_font).pack(side=tk.LEFT)
        source_box = ttk.Combobox(bar, textvariable=self._source_filter_var,
                                  values=SOURCE_FILTER_VALUES, state='readonly',
                                  width=8)
        source_box.pack(side=tk.LEFT, padx=(2, 8))
        source_box.bind('<<ComboboxSelected>>', lambda e: self.refresh())

        ttk.Label(bar, text='State', font=small_font).pack(side=tk.LEFT)
        state_box = ttk.Combobox(bar, textvariable=self._state_filter_var,
                                 values=STATUS_FILTER_VALUES, state='readonly',
                                 width=8)
        state_box.pack(side=tk.LEFT, padx=(2, 0))
        state_box.bind('<<ComboboxSelected>>', lambda e: self.refresh())

        ttk.Button(bar, text='Refresh', command=self.full_refresh).pack(
            side=tk.RIGHT)
        ttk.Button(bar, text='Graph', command=self._show_graph).pack(
            side=tk.RIGHT, padx=4)

    def _on_search_changed(self, *args):
        if self._search_after_id is not None:
            self.after_cancel(self._search_after_id)
        self._search_after_id = self.after(200, self.refresh)

    # MARK: - Layout

    def _build_panes(self):
        paned = ttk.PanedWindow(self, orient=tk.HORIZONTAL)
        paned.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

        scale = self.context.ui_scaling_factor * self.context.dpi_factor
        left = ttk.Frame(paned)
        paned.add(left, weight=30)

        self._build_toolbar(left)

        columns = ('status', 'indicator')
        tree = ttk.Treeview(left, columns=columns, selectmode=tk.BROWSE,
                            show='tree')
        # Only the name column absorbs pane resizing.
        tree.column('#0', width=int(170 * scale))
        tree.column('status', width=int(80 * scale), anchor=tk.E, stretch=False)
        tree.column('indicator', width=int(24 * scale), stretch=False)

        scroll = ttk.Scrollbar(left, orient=tk.VERTICAL, command=tree.yview)
        # ttk can't color a single cell, so the status dots are drawn on a
        # canvas laid over the indicator column.
        strip = tk.Canvas(tree, highlightthickness=0, borderwidth=0,
                          background=self._strip_background())
        self._row_colors = {}
        self._strip_job = None

        tree.configure(yscroll=lambda *a: (scroll.set(*a),
                                           self._schedule_strip_redraw()))
        tree.pack(fill=tk.BOTH, expand=True, side=tk.LEFT)
        scroll.pack(fill=tk.Y, side=tk.RIGHT)

        tree.bind('<<TreeviewSelect>>', self._on_selection_changed)
        tree.bind('<Button-3>', self._on_right_click)
        for event in ('<<TreeviewOpen>>', '<<TreeviewClose>>', '<Configure>',
                      '<<TreeviewSelect>>', '<ButtonRelease-1>'):
            tree.bind(event, lambda e: self._schedule_strip_redraw(), add='+')
        # The overlay behaves like the cells it covers.
        strip.bind('<Button-1>', self._on_strip_click)
        strip.bind('<MouseWheel>',
                   lambda e: tree.yview_scroll(-1 * (e.delta // 120), 'units'))
        strip.bind('<Button-4>', lambda e: tree.yview_scroll(-1, 'units'))
        strip.bind('<Button-5>', lambda e: tree.yview_scroll(1, 'units'))
        self.tree = tree
        self.strip = strip

        self.detail_host = ttk.Frame(paned)
        paned.add(self.detail_host, weight=70)
        self._show_detail_placeholder()

        # Initial sash at ~30% of the tab; draggable afterwards.
        self._sash_placed = False

        def place_sash(event=None):
            if self._sash_placed:
                return
            width = paned.winfo_width()
            if width > 1:
                paned.sashpos(0, int(width * 0.30))
                self._sash_placed = True

        paned.bind('<Map>', place_sash, add='+')
        paned.after_idle(place_sash)

    def _show_detail_placeholder(self):
        for child in self.detail_host.winfo_children():
            child.destroy()
        ttk.Label(self.detail_host, foreground='#777777',
                  text='Select a group, device, or interface to configure it.'
                  ).pack(padx=10, pady=10, anchor=tk.NW)

    # MARK: - Indicator strip

    def _on_strip_click(self, event):
        iid = self.tree.identify_row(event.y + self.strip.winfo_y())
        if iid:
            self.tree.selection_set(iid)

    def _strip_background(self):
        style = ttk.Style(self)
        return _resolve_color(
            self,
            style.lookup('Treeview', 'fieldbackground')
            or style.lookup('Treeview', 'background'),
            '#ffffff')

    def _schedule_strip_redraw(self):
        # One repaint per event burst.
        if self._strip_job is None:
            self._strip_job = self.after_idle(self._strip_redraw)

    def _strip_redraw(self):
        self._strip_job = None
        strip = self.strip
        tree = self.tree

        # Anchor to the indicator column of the first visible row.
        first = ''
        probe = 1
        height = tree.winfo_height()
        while not first and probe < height:
            first = tree.identify_row(probe)
            probe += 4
        cell = tree.bbox(first, 'indicator') if first else None
        if not cell:
            strip.place_forget()
            return
        col_x, top, col_w, row_height = cell
        strip.place(x=col_x, y=top, width=col_w, height=height - top)
        strip.delete('all')
        radius = max(4, int(col_w * 0.22))

        style = ttk.Style(self)
        selected_bg = _resolve_color(
            self, style.lookup('Treeview', 'background', ['selected']), '#4a6984')
        selection = set(tree.selection())

        y = top
        while y < height:
            iid = tree.identify_row(y + 2)
            if not iid:
                break
            cy = y - top
            if iid in selection:
                # Replicate the selection highlight under the overlay.
                strip.create_rectangle(0, cy, col_w, cy + row_height,
                                       fill=selected_bg, outline=selected_bg)
            color = self._row_colors.get(iid)
            if color is not None:
                mid = cy + row_height // 2
                strip.create_oval(col_w // 2 - radius, mid - radius,
                                  col_w // 2 + radius, mid + radius,
                                  fill=color, outline=color)
            y += row_height

    # MARK: - Tree building

    def full_refresh(self):
        self._collect_devices()
        self.refresh()

    def refresh(self):
        # Full rebuild preserving expansion and selection.
        open_iids = {iid for iid in self._all_iids() if self.tree.item(iid, 'open')}
        selected = set(self.tree.selection())
        self.tree.delete(*self.tree.get_children())
        self._row_colors = {}

        self._rows = {}
        for gid, device in self._devices.items():
            row = self._device_row(device)
            if self._row_passes_filters(row):
                self._rows[gid] = row

        self._groups = {}
        groups = build_sync_groups(list(self._rows.values()))
        groups.sort(key=lambda g: (g['local'], self._group_display(g)))
        for group in groups:
            iid = f'grp|{group["key"]}'
            self._groups[iid] = group
            self.tree.insert('', tk.END, iid=iid, text=self._group_display(group),
                             open=not self._built or iid in open_iids,
                             values=('', ''))
            for gid in group['members']:
                self._insert_device_rows(iid, self._rows[gid], group, open_iids)

        self._built = True
        still_there = [iid for iid in selected if self.tree.exists(iid)]
        if still_there:
            self.tree.selection_set(still_there)
        elif self._detail_key is not None:
            self._detail_key = None
            self._show_detail_placeholder()
        self._schedule_strip_redraw()

    def _insert_device_rows(self, group_iid, row, group, open_iids):
        gid = row['gid']
        marker = ' (master)' if gid == group['master_gid'] else ''
        self.tree.insert(group_iid, tk.END, iid=gid, text=f'{row["name"]}{marker}',
                         open=gid in open_iids,
                         values=(row['status'], ''))
        self._row_colors[gid] = row['color']

        sync = row['sync']
        for name, iface in sync.get_sync_interfaces().items():
            iface_iid = f'{gid}|{name}'
            status = iface.get_status_container().get_status('SourceStatus')
            label = INTERFACE_LABELS.get(name, name)
            source_marker = ' (source)' if iface is sync.get_source() else ''
            self.tree.insert(gid, tk.END, iid=iface_iid,
                             text=f'  {label}{source_marker}',
                             values=(status, ''))
            self._row_colors[iface_iid] = STATUS_COLORS.get(status, '#9a9a9a')

    def _group_display(self, group):
        if group['local']:
            return 'Local clocks'
        if group['master_gid'] is not None:
            master = self._rows.get(group['master_gid'])
            if master is not None:
                return f'Group {master["name"]}'
        # No member master: name the group by the reference it follows.
        applied = ''
        root = group['root']
        for gid in group['members']:
            row = self._rows.get(gid)
            if row is not None and not row['applied_key'].startswith('clock-sync:'):
                applied = row['applied']
                break
        parts = applied.split(':')
        if parts[0] == 'gps':
            return 'Group GPS'
        if parts[0] == 'ptp':
            domain = root.split(':')[-1] if root.startswith('ptp:') else '?'
            identity = parts[3] if len(parts) >= 4 else '?'
            return f'Group PTP master {identity} (domain {domain})'
        if parts[0] == 'ntp':
            return f'Group NTP {parts[-1]}'
        if parts[0] == 'irig':
            return f'Group IRIG {parts[-1]}'
        return f'Group {applied}' if applied else 'Group'

    def _all_iids(self, parent=''):
        iids = []
        for iid in self.tree.get_children(parent):
            iids.append(iid)
            iids.extend(self._all_iids(iid))
        return iids

    def _row_passes_filters(self, row):
        source_filter = self._source_filter_var.get()
        if source_filter != 'All':
            if source_kind(row['applied']) != source_filter:
                return False

        state_filter = self._state_filter_var.get()
        if state_filter != 'All' and row['status'] != state_filter:
            return False

        needle = self._search_var.get().strip().lower()
        if needle:
            haystack = ' '.join((row['name'], row['local_id'], row['location'],
                                 row['applied'], row['source'])).lower()
            if needle not in haystack:
                return False
        return True

    # MARK: - Selection and detail panel

    def _split_iid(self, iid):
        # 'grp|<key>' for groups, '<gid>' for devices, '<gid>|<iface>'.
        if iid.startswith('grp|'):
            return None, None, iid
        parts = iid.split('|')
        gid = parts[0]
        iface_name = parts[1] if len(parts) > 1 else None
        return gid, iface_name, None

    def _on_selection_changed(self, event=None):
        selection = self.tree.selection()
        key = selection[0] if selection else None
        if key == self._detail_key:
            return
        self._detail_key = key
        self._rebuild_detail()

    def _on_model_changed(self):
        # Detail rebuild deferred: the callback may run inside a widget
        # this destroys.
        self.refresh()
        self.after_idle(self._rebuild_detail)

    def _rebuild_detail(self):
        if not self.winfo_exists():
            return
        for child in self.detail_host.winfo_children():
            child.destroy()

        key = self._detail_key
        view = None
        if key is not None and self.tree.exists(key):
            gid, iface_name, group_iid = self._split_iid(key)
            if group_iid is not None:
                group = self._groups.get(group_iid)
                if group is not None:
                    members = [(self._rows[m]['device'], self._rows[m]['sync'])
                               for m in group['members'] if m in self._rows]
                    if members:
                        view = GroupConfigView(
                            self.detail_host, self.context,
                            self._group_display(group), members,
                            group['master_gid'], self._on_model_changed)
            elif gid in self._rows:
                row = self._rows[gid]
                if iface_name is None:
                    reported = row['applied'] \
                        if row['applied_key'].startswith('dev-ref:') else ''
                    view = DeviceConfigView(
                        self.detail_host, self.context, row['device'],
                        row['sync'], self.context.sync_event_log,
                        self._on_model_changed, reported_ref_id=reported)
                else:
                    iface = row['sync'].get_sync_interfaces().get(iface_name)
                    if iface is not None:
                        view = InterfaceConfigView(
                            self.detail_host, self.context, row['device'],
                            row['sync'], iface, self._on_model_changed)

        if view is None:
            self._show_detail_placeholder()
        else:
            view.pack(fill=tk.BOTH, expand=True)

    # MARK: - Context menu

    def _on_right_click(self, event):
        iid = self.tree.identify_row(event.y)
        if not iid:
            return
        self.tree.selection_set(iid)
        gid, iface_name, group_iid = self._split_iid(iid)
        if group_iid is not None or gid not in self._rows or iface_name is None:
            return
        sync = self._rows[gid]['sync']
        iface = sync.get_sync_interfaces().get(iface_name)
        if iface is None:
            return

        popup = tk.Menu(self.tree, tearoff=0)
        if iface is not sync.get_source() and iface_name in sync.get_available_sources():
            popup.add_command(label='Select as source',
                              command=lambda: self._select_as_source(sync, iface))
            popup.add_separator()
        popup.add_command(label='Simulate upstream loss',
                          command=lambda: self._simulate(iface.simulate.upstream_loss))
        popup.add_command(label='Simulate upstream restored',
                          command=lambda: self._simulate(iface.simulate.upstream_restored))
        try:
            popup.tk_popup(event.x_root, event.y_root, 0)
        finally:
            popup.grab_release()

    def _select_as_source(self, sync, iface):
        try:
            sync.set_source(iface)
        except ValueError as e:
            messagebox.showerror('Select as source', str(e), parent=self)
        self._on_model_changed()

    def _simulate(self, action):
        action()
        self._on_model_changed()

    # MARK: - Graph view

    def _show_graph(self):
        # Same group keys as the tree so both views agree.
        entries = []
        for device in self._devices.values():
            applied, applied_key, all_keys, owns = device_group_keys(
                device, self._sync_for(device))
            entries.append((device.name, applied_key, all_keys))
        SyncGraphWindow(self, self.context, entries)
