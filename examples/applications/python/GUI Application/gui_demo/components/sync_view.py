import tkinter as tk
from tkinter import ttk

import opendaq as daq

from .. import utils
from ..app_context import AppContext
from .block_view import BlockView
from .generic_properties_treeview import PropertiesTreeview

NO_DOMAIN = 'No reference domain'
LOCAL_CLOCKS = 'Local clocks'
SOURCE_STATUS = 'SyncSourceStatus'
ROLE_STATUS = 'SyncRoleStatus'

STATUS_COLORS = {
    'Synced': str(utils.StatusColor.OK),
    'Listening': str(utils.StatusColor.WARNING),
    'Calibrating': str(utils.StatusColor.WARNING),
    'Error': str(utils.StatusColor.ERROR),
    'Off': 'gray',
    'Unknown': 'gray',
}

HEADINGS = {'#0': 'Name', 'source': 'Source', 'mode': 'Mode', 'role': 'Role', 'status': 'Status'}
SORT_KEYS = {
    '#0': lambda record: record.device.name.lower(),
    'source': lambda record: record.source.id,
    'mode': lambda record: record.source.mode.name,
    'role': lambda record: role_status(record.source),
    'status': lambda record: record.status,
}


def status_name(interface, name):
    statuses = interface.status_container.statuses
    return statuses[name].name if name in statuses else 'Unknown'


def source_status(interface):
    return status_name(interface, SOURCE_STATUS)


def role_status(interface):
    return status_name(interface, ROLE_STATUS)


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
        self.sort = ('#0', False)
        self.refresh_job = None

        style = ttk.Style(self)
        style.configure('SyncHeader.TLabel', font=('TkDefaultFont', 12, 'bold'))
        style.configure('SyncGroup.TLabel', font=('TkDefaultFont', 9, 'bold'))
        self.arrows = {descending: self.arrow_create(descending) for descending in (False, True)}

        panes = ttk.PanedWindow(self, orient=tk.HORIZONTAL)
        panes.pack(fill=tk.BOTH, expand=True)

        left = ttk.Frame(panes)
        self.details = ttk.Frame(panes)
        self.details_header = ttk.Frame(self.details)
        self.details_header.pack(fill=tk.X)
        self.details_body = ttk.Frame(self.details)
        self.details_body.pack(fill=tk.BOTH, expand=True)
        self.details_key = None
        panes.add(left, weight=3)
        panes.add(self.details, weight=2)
        panes.bind('<Map>', lambda e: self.after_idle(lambda: panes.sashpos(0, int(panes.winfo_width() * 0.62))))

        self.tree_create(left)

        self.core_event_handler = daq.QueuedEventHandler(self.on_core_event)
        self.context.instance.context.on_core_event + self.core_event_handler
        self.bind('<Destroy>', self.on_destroy)

        self.refresh()

    def tree_create(self, parent):
        frame = ttk.Frame(parent)
        frame.pack(fill=tk.BOTH, expand=True)

        self.dot_size = int(10 * self.context.dpi_factor)

        columns = ('source', 'mode', 'role', 'status', 'dot')
        tree = ttk.Treeview(frame, columns=columns, show='tree headings', selectmode=tk.BROWSE)
        tree.column('#0', anchor=tk.W, width=int(260 * self.context.dpi_factor), stretch=False)
        for column, width in zip(columns, (170, 80, 65, 75)):
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
        self.headings_draw()

    def arrow_create(self, descending):
        width = int(9 * self.context.dpi_factor) | 1
        height = width // 2 + 1
        image = tk.PhotoImage(master=self, width=width, height=height)
        for row in range(height):
            reach = height - 1 - row if descending else row
            image.put('#8a8a8a', to=(width // 2 - reach, row, width // 2 + reach + 1, row + 1))
        return image

    def headings_draw(self):
        column, descending = self.sort
        for key, title in HEADINGS.items():
            image = self.arrows[descending] if key == column else ''
            self.tree.heading(key, anchor=tk.W, text=title, image=image, command=lambda key=key: self.sort_by(key))

    def sort_by(self, column):
        self.sort = (column, self.sort == (column, False))
        self.headings_draw()
        self.tree_fill()

    def sorted(self, records):
        column, descending = self.sort
        return sorted(records, key=lambda record: (SORT_KEYS[column](record), record.device.name), reverse=descending)

    def refresh(self):
        self.refresh_job = None
        root = self.context.instance
        devices = [root, *self.devices_of(root)]
        self.records = [record for record in map(self.record_create, devices) if record is not None]
        self.tree_fill()

    @staticmethod
    def record_create(device):
        try:
            return SyncRecord(device) if device.synchronization is not None else None
        except RuntimeError as e:
            print(f'Synchronization of {device.name} could not be read:', e)
            return None

    def devices_of(self, device):
        for child in device.devices:
            yield child
            yield from self.devices_of(child)

    def groups(self):
        domains = {}
        for record in self.records:
            domains.setdefault(record.domain, []).append(record)

        groups = []
        for domain in sorted(domains, key=lambda d: (d == NO_DOMAIN, d)):
            members = sorted(domains[domain], key=self.tree_order)
            owner = None if domain == NO_DOMAIN else self.clock_owner(domain, members)
            groups.append((domain, owner, members))

        feeding = [owner for domain, owner, members in groups if owner is not None and owner not in members]
        alone = [members[0] for domain, owner, members in groups if domain.startswith('local:') and len(members) == 1]
        local = [record for record in alone if record not in feeding]
        if local:
            yield LOCAL_CLOCKS, LOCAL_CLOCKS, None, local

        for domain, owner, members in groups:
            if members[0] in alone and len(members) == 1:
                continue
            if owner is None:
                yield domain, domain if domain == NO_DOMAIN else f'{domain} (external clock)', None, members
            else:
                yield domain, domain, owner, [record for record in members if record is not owner]

    @staticmethod
    def tree_order(record):
        return record.device.global_id.count('/'), record.device.name

    def clock_owner(self, domain, members):
        outputting = [record for record in self.records if record.outputs(domain)]
        if outputting:
            return outputting[0]
        local = [record for record in members if record.source.sync_type == 'local']
        return local[0] if local else None

    def tree_fill(self):
        opened = {iid for iid in self.rows if self.tree.exists(iid) and self.tree.item(iid, 'open')}
        self.tree.delete(*self.tree.get_children())
        self.rows = {}
        self.dot_colors = {}
        self.editors = {}

        for key, title, owner, members in self.groups():
            visible = [owner] * (owner is not None) + self.sorted(members)
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
        self.dot_colors[iid] = STATUS_COLORS.get(record.status, 'gray')

        for name, interface in record.interfaces.items():
            status = source_status(interface)
            text = f'{name} (source)' if name == record.source.id else name
            child = f'{iid}|{name}'
            mode = self.mode_editor_register(child, record, interface)
            self.tree.insert(iid, tk.END, iid=child, text=text,
                             values=('', mode, role_status(interface), status))
            self.rows[child] = (record, interface)
            self.dot_colors[child] = STATUS_COLORS.get(status, 'gray')

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
        for widget in list(self.details_header.children.values()):
            widget.destroy()

        # A refresh redraws the status header but keeps an open property editor
        key = record.device.global_id if record is not None else None
        if key != self.details_key:
            for widget in list(self.details_body.children.values()):
                widget.destroy()
            self.properties = None
        self.details_key = key

        if record is None:
            ttk.Label(self.details_header, text='No devices expose synchronization').pack(anchor=tk.W, padx=8, pady=8)
            return

        header = ttk.Frame(self.details_header)
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
                self.dot_create(statuses, STATUS_COLORS.get(value.name, 'gray'), background).pack(side=tk.LEFT, padx=(4, 4))
            tk.Label(statuses, background=background,
                     text=f'{label}: {value.name}' if label else value.name).pack(side=tk.LEFT, padx=(0, 4))

        widgets = (statuses, *statuses.winfo_children())
        for widget in widgets:
            widget.configure(cursor='hand2')
            widget.bind('<Button-1>', lambda e: BlockView.show_all_statuses(self, container))
            widget.bind('<Enter>', lambda e: [w.configure(background='#e0e0e0') for w in widgets])
            widget.bind('<Leave>', lambda e: [w.configure(background=background) for w in widgets])

        if self.properties is not None:
            return
        frame = ttk.Frame(self.details_body)
        frame.pack(fill=tk.BOTH, expand=True)
        self.properties = PropertiesTreeview(frame, daq.IPropertyObject.cast_from(record.synchronization), self.context)

    def change(self, record, apply, value):
        try:
            apply(value)
        except RuntimeError as e:
            utils.show_error('Synchronization change failed', f'{record.device.name}: {e}', self)
        self.refresh()

    def on_core_event(self, sender, args: daq.IEventArgs):
        if args.event_name not in ('PropertyValueChanged', 'StatusChanged') or self.refresh_job is not None:
            return
        if not daq.ICoreEventArgs.can_cast_from(args):
            return
        # Only events from a synchronization, whose path starts at the device's daqSynchronization
        params = daq.ICoreEventArgs.cast_from(args).parameters
        if 'Path' not in params or not str(params['Path']).startswith('daqSynchronization'):
            return
        self.refresh_job = self.after(100, self.refresh)

    def on_destroy(self, event):
        if event.widget is not self:
            return
        if self.refresh_job is not None:
            self.after_cancel(self.refresh_job)
        self.context.instance.context.on_core_event - self.core_event_handler
