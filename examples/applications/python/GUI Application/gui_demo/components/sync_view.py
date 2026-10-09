import tkinter as tk
from tkinter import ttk

import opendaq as daq

from .. import utils
from ..app_context import AppContext
from .block_view import BlockView
from .generic_properties_treeview import PropertiesTreeview

ALL = 'All'
NO_DOMAIN = 'No reference domain'
LOCAL_CLOCKS = 'Local clocks'
SOURCE_STATUS = 'SynchronizationSourceStatus'
ROLE_STATUS = 'SynchronizationRoleStatus'

STATUS_COLORS = {
    'Synced': str(utils.StatusColor.OK),
    'Listening': str(utils.StatusColor.WARNING),
    'Calibrating': str(utils.StatusColor.WARNING),
    'Error': str(utils.StatusColor.ERROR),
    'Off': 'gray',
    'Unknown': 'gray',
}


def source_status(interface):
    return interface.status_container.get_status(SOURCE_STATUS).name


def role_status(interface):
    return interface.status_container.get_status(ROLE_STATUS).name


class SyncRecord:
    def __init__(self, device: daq.IDevice):
        self.device = device
        self.synchronization = device.synchronization
        self.source = self.synchronization.source
        self.interfaces = dict(self.synchronization.interfaces.items())
        self.domain = self.source.reference_domain_id or NO_DOMAIN
        self.status = source_status(self.source)

    def outputs(self, domain):
        return any(interface.reference_domain_id == domain and role_status(interface) == 'Output'
                   for interface in self.interfaces.values())


class SyncView(ttk.Frame):
    def __init__(self, parent, context: AppContext, **kwargs):
        super().__init__(parent, **kwargs)
        self.context = context
        self.records = []
        self.rows = {}
        self.dot_colors = {}
        self.dots = []
        self.editors = {}
        self.combos = []
        self.active_combo = None
        self.selected = None
        self.properties = None
        self.refresh_job = None

        style = ttk.Style(self)
        style.configure('SyncHeader.TLabel', font=('TkDefaultFont', 12, 'bold'))
        style.configure('SyncGroup.TLabel', font=('TkDefaultFont', 9, 'bold'))

        panes = ttk.PanedWindow(self, orient=tk.HORIZONTAL)
        panes.pack(fill=tk.BOTH, expand=True)

        left = ttk.Frame(panes)
        self.details = ttk.Frame(panes)
        panes.add(left, weight=3)
        panes.add(self.details, weight=2)
        panes.bind('<Map>', lambda e: self.after_idle(lambda: panes.sashpos(0, int(panes.winfo_width() * 0.62))))

        self.filters_create(left)
        self.tree_create(left)

        self.core_event_handler = daq.QueuedEventHandler(self.on_core_event)
        self.context.instance.context.on_core_event + self.core_event_handler
        self.bind('<Destroy>', self.on_destroy)

        self.refresh()

    def filters_create(self, parent):
        frame = ttk.Frame(parent)
        frame.pack(fill=tk.X, pady=(0, 4))

        self.type_filter = tk.StringVar(value=ALL)
        self.status_filter = tk.StringVar(value=ALL)

        ttk.Label(frame, text='Type').pack(side=tk.LEFT, padx=(4, 2))
        self.type_combo = ttk.Combobox(frame, textvariable=self.type_filter, state='readonly', width=12)
        self.type_combo.pack(side=tk.LEFT)

        ttk.Label(frame, text='Status').pack(side=tk.LEFT, padx=(10, 2))
        status_combo = ttk.Combobox(frame, textvariable=self.status_filter, state='readonly', width=12,
                                    values=[ALL, *daq.SyncSourceStatus.__members__])
        status_combo.pack(side=tk.LEFT)

        for combo in (self.type_combo, status_combo):
            combo.bind('<<ComboboxSelected>>', lambda e: self.tree_fill())

    def tree_create(self, parent):
        frame = ttk.Frame(parent)
        frame.pack(fill=tk.BOTH, expand=True)

        self.dot_size = int(10 * self.context.dpi_factor)

        columns = ('source', 'mode', 'role', 'status', 'dot')
        tree = ttk.Treeview(frame, columns=columns, show='tree headings', selectmode=tk.BROWSE)
        tree.heading('#0', anchor=tk.W, text='Name')
        tree.column('#0', anchor=tk.W, width=int(260 * self.context.dpi_factor), stretch=False)
        for column, width, heading in zip(columns, (170, 80, 65, 75), ('Source', 'Mode', 'Role', 'Status')):
            tree.heading(column, anchor=tk.W, text=heading)
            tree.column(column, anchor=tk.W, width=int(width * self.context.dpi_factor), stretch=column == 'source')
        tree.column('dot', width=self.dot_size + 6, minwidth=self.dot_size + 6, stretch=False)

        scroll_bar = ttk.Scrollbar(frame, orient=tk.VERTICAL, command=tree.yview)
        tree.configure(yscrollcommand=lambda *a: (scroll_bar.set(*a), self.after_idle(self.overlays_sync)))
        scroll_bar.pack(side=tk.RIGHT, fill=tk.Y)
        tree.pack(fill=tk.BOTH, expand=True)

        tree.tag_configure('group', font=('TkDefaultFont', 9, 'bold'))

        style = ttk.Style(self)
        self.row_background = style.lookup('Treeview', 'background') or 'white'
        self.selected_background = style.lookup('Treeview', 'background', ['selected']) or '#0078d7'
        style.configure('Selection.TCombobox', fieldbackground='white', background='white', foreground='#1a1a1a',
                        selectbackground='white', selectforeground='#1a1a1a')

        tree.bind('<<TreeviewSelect>>', self.on_select)
        for sequence in ('<Configure>', '<<TreeviewOpen>>', '<<TreeviewClose>>', '<ButtonRelease-1>'):
            tree.bind(sequence, lambda e: self.after_idle(self.overlays_sync), add='+')
        self.tree = tree

    def refresh(self):
        self.refresh_job = None
        root = self.context.instance
        devices = [root, *self.devices_of(root)]
        self.records = [SyncRecord(device) for device in devices if device.synchronization is not None]

        types = sorted({record.source.sync_type for record in self.records})
        self.type_combo.configure(values=[ALL, *types])
        if self.type_filter.get() not in (ALL, *types):
            self.type_filter.set(ALL)

        self.tree_fill()

    def devices_of(self, device):
        for child in device.devices:
            yield child
            yield from self.devices_of(child)

    def groups(self):
        domains = {}
        for record in self.records:
            domains.setdefault(record.domain, []).append(record)

        local = [members[0] for domain, members in domains.items()
                 if domain.startswith('local:') and len(members) == 1]
        if local:
            yield LOCAL_CLOCKS, LOCAL_CLOCKS, sorted(local, key=self.tree_order)

        for domain in sorted(domains, key=lambda d: (d == NO_DOMAIN, d)):
            members = sorted(domains[domain], key=self.tree_order)
            if members[0] in local:
                continue
            owner = None if domain == NO_DOMAIN else self.clock_owner(domain, members)
            if owner is None:
                yield domain, domain if domain == NO_DOMAIN else f'{domain} (external clock)', members
            else:
                yield domain, domain, [owner, *(record for record in members if record is not owner)]

    @staticmethod
    def tree_order(record):
        return record.device.global_id.count('/'), record.device.name

    def clock_owner(self, domain, members):
        outputting = [record for record in self.records if record.outputs(domain)]
        if outputting:
            return outputting[0]
        local = [record for record in members if record.source.sync_type == 'local']
        return local[0] if local else None

    def matches(self, record):
        return (self.type_filter.get() in (ALL, record.source.sync_type)
                and self.status_filter.get() in (ALL, record.status))

    def tree_fill(self):
        opened = {iid for iid in self.rows if self.tree.exists(iid) and self.tree.item(iid, 'open')}
        self.tree.delete(*self.tree.get_children())
        self.rows = {}
        self.dot_colors = {}
        self.editors = {}

        for key, title, members in self.groups():
            visible = [record for record in members if self.matches(record)]
            if not visible:
                continue

            count = f'{len(visible)} device' if len(visible) == 1 else f'{len(visible)} devices'
            group = self.tree.insert('', tk.END, iid=key, text=f'{title} \u00b7 {count}', open=True, tags=('group',))
            self.rows[group] = (visible[0], None)

            for record in visible:
                self.device_row_insert(group, record)

        for iid in opened & self.rows.keys():
            self.tree.item(iid, open=True)

        if self.selected in self.rows:
            self.tree.selection_set(self.selected)
            self.tree.see(self.selected)
        elif self.rows:
            self.tree.selection_set(self.tree.get_children(self.tree.get_children()[0])[0])
        else:
            self.details_draw(None, None)

        self.after_idle(self.overlays_sync)

    def device_row_insert(self, group, record):
        iid = f'{group}|{record.device.global_id}'
        sources = {name: name for name in record.synchronization.available_sources.keys()}
        source = self.editor_register(iid, 'source', record, record.source.id, sources,
                                      lambda name: setattr(record.synchronization, 'source', name))
        mode = self.mode_editor_register(iid, record, record.source)
        self.tree.insert(group, tk.END, iid=iid, text=record.device.name,
                         values=(source, mode, role_status(record.source), record.status))
        self.rows[iid] = (record, record.source)
        self.dot_colors[iid] = STATUS_COLORS[record.status]

        for name, interface in record.interfaces.items():
            status = source_status(interface)
            text = f'{name} (source)' if name == record.source.id else name
            child = f'{iid}|{name}'
            mode = self.mode_editor_register(child, record, interface)
            self.tree.insert(iid, tk.END, iid=child, text=text,
                             values=('', mode, role_status(interface), status))
            self.rows[child] = (record, interface)
            self.dot_colors[child] = STATUS_COLORS[status]

    def mode_editor_register(self, iid, record, interface):
        modes = {name: daq.SyncMode(value) for value, name in interface.available_modes.items()}
        return self.editor_register(iid, 'mode', record, interface.mode.name, modes,
                                    lambda mode: setattr(interface, 'mode', mode))

    def editor_register(self, iid, column, record, current, options, apply):
        if len(options) < 2:
            return current
        self.editors[(iid, column)] = (record, current, options, apply)
        return current

    def combo_create(self):
        combo = ttk.Combobox(self.tree, state='readonly', style='Selection.TCombobox', takefocus=False)
        combo.bind('<<ComboboxSelected>>', lambda e: self.combo_selected(combo))
        combo.bind('<Button-1>', lambda e: self.combo_toggle(combo))
        combo.bind('<Escape>', lambda e: self.combo_release(combo))
        combo.bind('<FocusOut>', lambda e: self.combo_release(combo))
        combo.bind('<KeyPress>', lambda e: 'break')
        utils.bind_mousewheel_to(combo, self.tree, self.overlays_sync)
        return combo

    def combo_toggle(self, combo):
        if self.active_combo is combo:
            self.tk.call('ttk::combobox::Unpost', combo)
            self.active_combo = None
            return 'break'
        self.active_combo = combo
        combo.focus_set()
        combo.after_idle(lambda: self.tk.call('ttk::combobox::Post', combo))
        return 'break'

    def combo_release(self, combo):
        if self.active_combo is combo:
            self.active_combo = None

    def combo_selected(self, combo):
        self.combo_release(combo)
        record, current, options, apply = self.editors[combo.key]
        self.change(record, apply, options[combo.get()])

    def visible_rows(self, parent=''):
        for iid in self.tree.get_children(parent):
            yield iid
            if self.tree.item(iid, 'open'):
                yield from self.visible_rows(iid)

    def dot_create(self, parent, color, background):
        dot = tk.Canvas(parent, width=self.dot_size, height=self.dot_size,
                        highlightthickness=0, bd=0, background=background)
        dot.create_oval(0, 0, self.dot_size - 1, self.dot_size - 1, outline='', fill=color, tags='dot')
        return dot

    def overlays_sync(self):
        if not self.tree.winfo_exists():
            return
        self.dots_sync()
        self.combos_sync()

    def combos_sync(self):
        inset = max(1, int(self.context.dpi_factor))
        cells = [(key, self.tree.bbox(*key)) for key in self.editors]
        cells = [(key, box) for key, box in cells if box and box[1] + box[3] <= self.tree.winfo_height()]

        while len(self.combos) < len(cells):
            self.combos.append(self.combo_create())

        for combo in self.combos[len(cells):]:
            combo.place_forget()

        for combo, (key, (x, y, width, height)) in zip(self.combos, cells):
            record, current, options, apply = self.editors[key]
            combo.key = key
            combo.configure(values=list(options))
            combo.set(current)
            combo.place(x=x, y=y + inset, width=width, height=height - 2 * inset)

    def dots_sync(self):
        rows = [(iid, self.tree.bbox(iid, 'dot')) for iid in self.visible_rows() if iid in self.dot_colors]
        rows = [(iid, box) for iid, box in rows if box]

        while len(self.dots) < len(rows):
            dot = self.dot_create(self.tree, '', self.row_background)
            dot.bind('<Button-1>', lambda e, d=dot: self.tree.selection_set(d.iid))
            self.dots.append(dot)

        for dot in self.dots[len(rows):]:
            dot.place_forget()

        selection = self.tree.selection()
        for dot, (iid, (x, y, width, height)) in zip(self.dots, rows):
            dot.iid = iid
            dot.itemconfigure('dot', fill=self.dot_colors[iid])
            dot.configure(background=self.selected_background if iid in selection else self.row_background)
            dot.place(x=x + 2, y=y + (height - self.dot_size) // 2)

    def on_select(self, event):
        selection = self.tree.selection()
        if not selection:
            return
        self.selected = selection[0]
        record, interface = self.rows[self.selected]
        self.details_draw(record, interface or record.source)
        self.after_idle(self.overlays_sync)

    def details_draw(self, record, interface):
        for widget in list(self.details.children.values()):
            widget.destroy()
        self.properties = None

        if record is None:
            ttk.Label(self.details, text='No devices expose synchronization').pack(anchor=tk.W, padx=8, pady=8)
            return

        header = ttk.Frame(self.details)
        header.pack(fill=tk.X, padx=(8, 0), pady=(4, 6))
        background = ttk.Style(self).lookup('TFrame', 'background')
        ttk.Label(header, text=record.device.name, style='SyncHeader.TLabel').pack(side=tk.LEFT)
        ttk.Label(header, text=f' | {record.domain} | ').pack(side=tk.LEFT)

        container = interface.status_container
        statuses = tk.Frame(header, cursor='hand2', background=background)
        statuses.pack(side=tk.LEFT)
        labels = {SOURCE_STATUS: '', ROLE_STATUS: 'Role'}
        values = dict(container.statuses.items())
        shown = [(name, values[name]) for name in labels if name in values]
        shown += [(name, value) for name, value in values.items() if name not in labels]
        for index, (name, value) in enumerate(shown):
            if index:
                tk.Label(statuses, text=' | ', background=background).pack(side=tk.LEFT)
            label = labels.get(name, name)
            if name == SOURCE_STATUS:
                self.dot_create(statuses, STATUS_COLORS[value.name], background).pack(side=tk.LEFT, padx=(4, 4))
            tk.Label(statuses, background=background,
                     text=f'{label}: {value.name}' if label else value.name).pack(side=tk.LEFT, padx=(0, 4))

        widgets = (statuses, *statuses.winfo_children())
        for widget in widgets:
            widget.configure(cursor='hand2')
            widget.bind('<Button-1>', lambda e: BlockView.show_all_statuses(self, container))
            widget.bind('<Enter>', lambda e: [w.configure(background='#e0e0e0') for w in widgets])
            widget.bind('<Leave>', lambda e: [w.configure(background=background) for w in widgets])

        ttk.Label(self.details, text=f'{interface.id} configuration', style='SyncGroup.TLabel').pack(anchor=tk.W, padx=(8, 0))
        frame = ttk.Frame(self.details)
        frame.pack(fill=tk.BOTH, expand=True)
        self.properties = PropertiesTreeview(frame, interface.configuration, self.context)

    def change(self, record, apply, value):
        try:
            apply(value)
        except RuntimeError as e:
            utils.show_error('Synchronization change failed', f'{record.device.name}: {e}', self)
        self.refresh()

    def on_core_event(self, sender, args: daq.IEventArgs):
        if args.event_name not in ('PropertyValueChanged', 'StatusChanged') or self.refresh_job is not None:
            return
        self.refresh_job = self.after(100, self.refresh)

    def on_destroy(self, event):
        if event.widget is not self:
            return
        if self.refresh_job is not None:
            self.after_cancel(self.refresh_job)
        self.context.instance.context.on_core_event - self.core_event_handler
