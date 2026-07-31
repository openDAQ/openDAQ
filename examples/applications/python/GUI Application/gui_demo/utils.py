import os
import tkinter as tk
import enum
from tkinter import ttk
from tkinter import messagebox
from datetime import datetime, timedelta

import opendaq as daq

yes_no = ['No', 'Yes']

yes_no_inv = {
    'yes': True,
    'no': False,
    'y': True,
    'n': False,
    'none': False,
    'true': True,
    'false': False,
    'on': True,
    'off': False,
    '1': True,
    '0': False
}

class StatusColor(enum.Enum):
    OK = 'olive drab'
    WARNING = 'orange'
    ERROR = 'red'
    NOT_SET = 'light blue'
    def __str__(self):
        return self.value

# display order of log levels in comboboxes
LOG_LEVELS = {
    'Trace': daq.LogLevel.Trace,
    'Debug': daq.LogLevel.Debug,
    'Info': daq.LogLevel.Info,
    'Warn': daq.LogLevel.Warn,
    'Error': daq.LogLevel.Error,
    'Critical': daq.LogLevel.Critical,
    'Off': daq.LogLevel.Off,
    'Default': daq.LogLevel.Default,
}

def log_level_name(level):
    for name, value in LOG_LEVELS.items():
        if value == level:
            return name
    return 'Default'

def find_component(id, parent=None, convert_id=True):
    # find_component wants a path relative to `parent`, while a global id is
    # absolute: '/root_local_id/rest/of/path'. Splitting on '/' therefore yields
    # an empty leading element and the root's own id, and dropping both is what
    # 'split_id[2:]' is for. Pass convert_id=False when the caller already holds
    # a relative path, or the first two real segments would be eaten instead.
    if convert_id:
        split_id = id.split('/')
        id = '/'.join(split_id[2:])
    return None if parent is None else parent.find_component(id)


def treeview_get_first_selection(treeview):
    sel = treeview.selection()
    if len(sel) == 0:
        return None
    return sel[0]


def treeview_select_item(treeview, event):
    item = treeview.identify_row(event.y)
    treeview.selection_set(item)


def get_nearest_device(component, default=None):
    while component:
        if daq.IDevice.can_cast_from(component):
            return daq.IDevice.cast_from(component)
        component = component.parent

    return default


def get_nearest_fb(component, default=None):
    while component:
        if daq.IFunctionBlock.can_cast_from(component):
            return daq.IFunctionBlock.cast_from(component)
        component = component.parent
    return default


def get_nearest_named_parent_folder(component, name):
    while component:
        if daq.IFolderConfig.can_cast_from(component):
            folder = daq.IFolderConfig.cast_from(component)
            if folder.name == name:
                return folder
        component = component.parent
    return None


def show_modal(window):
    window.wait_visibility()
    window.grab_set()
    window.wait_window(window)


def show_selection(title, current_value, values):
    global result
    result = current_value
    top = tk.Toplevel()
    top.resizable(False, False)

    ttk.Label(top, text=title).pack()

    def select_value(v):
        global result
        top.destroy()
        result = v

    def make_closure(v):
        return lambda: select_value(v)

    def fill_buttons(idx, value):
        if current_value == idx:
            sel_text = '* '
        else:
            sel_text = ''
        button = ttk.Button(top, text=sel_text + str(value),
                            command=make_closure(idx))
        button.pack(expand=True, fill=tk.BOTH)

    if daq.IDict.can_cast_from(values):
        for idx, value in daq.IDict.cast_from(values).items():
            fill_buttons(idx, value)
    else:
        i = 0
        for value in daq.IList.cast_from(values):
            fill_buttons(i, value)
            i = i + 1

    # center window on screen
    ww = top.winfo_reqwidth()
    wh = top.winfo_reqheight()
    pr = int(top.winfo_screenwidth() / 2 - ww / 2)
    pd = int(top.winfo_screenheight() / 2 - wh / 2)
    top.geometry('+{}+{}'.format(pr, pd))

    top.attributes("-topmost", True)

    show_modal(top)
    return result


def component_tags(component):
    """Tag strings of a component, empty when it has none. Both the tree search
    and the input port dropdown fold these into their search text; components
    that predate tags (or are already gone) must not raise here."""
    try:
        tags = component.tags
    except (AttributeError, RuntimeError):
        return []
    if tags is None:
        return []
    try:
        return [str(tag) for tag in tags.list]
    except (AttributeError, RuntimeError):
        return []


def tooltip_dismiss(widget):
    """Cancel and hide a widget's tooltip. Call this when moving the widget: a
    tooltip belongs to a pointer resting on something, and a widget that was
    just repositioned under the pointer is not that."""
    dismiss = getattr(widget, 'tooltip_dismiss', None)
    if dismiss is not None:
        dismiss()


# 500 ms: long enough that sweeping the pointer across a row of buttons shows
# nothing, short enough to feel like an answer when someone stops to ask.
def attach_tooltip(widget, text, delay_ms=500):
    """Label an icon-only button. `text` may be a callable, for buttons whose
    meaning changes with the row they are floating over.

    The tip is placed below the widget rather than at the pointer: the tree's
    floating action buttons decide whether they are still hovered by asking
    what is under the pointer, and a window there would look like a third
    widget and make them flicker.

    <Leave> is treated as a hint, not the truth. A popup menu's grab swallows it,
    an un-placed widget never sends it, and a widget moved under a resting
    pointer gets <Enter> without ever having been pointed at - so while a tip is
    up, the pointer position is rechecked and the tip withdrawn once it is no
    longer over the widget."""
    # How long a tip can outlive the pointer that earned it. Every path that
    # loses <Leave> - a menu grab, an un-placed widget, a widget moved under a
    # resting pointer - is caught by this poll, so it is also the worst-case
    # lifetime of a stale tip. Only runs while a tip is on screen.
    WATCH_MS = 150
    state = {'window': None, 'job': None, 'watch': None}

    def cancel(key):
        if state[key] is not None:
            try:
                widget.after_cancel(state[key])
            except Exception:
                pass
            state[key] = None

    def hide(_event=None):
        cancel('job')
        cancel('watch')
        if state['window'] is not None:
            try:
                state['window'].destroy()
            except Exception:
                pass
            state['window'] = None

    # Whether the pointer really is over the widget, rather than whether Tk last
    # said so. Also false for a widget that exists but is not on screen.
    def pointer_on_widget():
        try:
            if not widget.winfo_exists() or not widget.winfo_ismapped():
                return False
            px, py = widget.winfo_pointerxy()
            x, y = widget.winfo_rootx(), widget.winfo_rooty()
            return (x <= px < x + widget.winfo_width()
                    and y <= py < y + widget.winfo_height())
        except Exception:
            return False

    def watch():
        state['watch'] = None
        if not pointer_on_widget():
            hide()
            return
        state['watch'] = widget.after(WATCH_MS, watch)

    def show():
        state['job'] = None
        if not pointer_on_widget():
            return
        label = text() if callable(text) else text
        if not label:
            return
        top = tk.Toplevel(widget)
        top.withdraw()
        top.overrideredirect(True)
        try:
            top.attributes('-topmost', True)
        except tk.TclError:
            pass
        tk.Label(top, text=label, justify=tk.LEFT, background='#ffffe1',
                 relief=tk.SOLID, borderwidth=1, padx=4, pady=1).pack()
        top.update_idletasks()
        top.geometry('+{}+{}'.format(
            widget.winfo_rootx(),
            widget.winfo_rooty() + widget.winfo_height() + 2))
        top.deiconify()
        state['window'] = top
        state['watch'] = widget.after(WATCH_MS, watch)

    def schedule(_event=None):
        hide()
        state['job'] = widget.after(delay_ms, show)

    widget.bind('<Enter>', schedule, add='+')
    widget.bind('<Leave>', hide, add='+')
    widget.bind('<Button-1>', hide, add='+')
    widget.bind('<Destroy>', hide, add='+')

    # A widget that is moved under a resting pointer gets <Enter> without the
    # user having pointed at anything, and no <Leave> when it moves away again.
    # Whoever moves the widget has to say so; see tooltip_dismiss.
    widget.tooltip_dismiss = hide


def root_device(node):
    while node.parent:
        node = node.parent
    return node


def list_all_subdevices(node: daq.IComponent):
    result = []
    if node is not None and daq.IFolder.can_cast_from(node):
        node = daq.IFolder.cast_from(node)
        for item in node.items:
            if daq.IDevice.can_cast_from(item):
                result.append(daq.IDevice.cast_from(item))
        for item in node.items:
            result.extend(list_all_subdevices(item))
    return result


def get_files_in_directory(directory):
    files = []
    for file in os.listdir(directory):
        if os.path.isfile(os.path.join(directory, file)) and file.endswith('.png'):
            files.append(file)
    return files


def load_icon(filename, scale=1):
    """Load a pre-rendered icon PNG. For scale>1 the _x2 variant is used when available,
    otherwise the 1x version is pixel-doubled via zoom()."""
    if scale > 1:
        hires = filename.replace('.png', '_x2.png')
        if os.path.exists(hires):
            return tk.PhotoImage(file=hires)
        img = tk.PhotoImage(file=filename)
        return img.zoom(scale, scale)
    return tk.PhotoImage(file=filename)

def signal_time_domain_check(sig):
    desc = sig.descriptor
    if desc is not None and desc.tick_resolution is not None:
        unit = desc.unit
        if (unit is not None and unit.quantity.casefold() == "time".casefold()) and (unit.symbol.casefold() == "s".casefold()):
            if len(desc.origin) != 0:
                return desc.origin

    return None

def parse_iso_string(date_string: str) -> datetime:
    '''
    Handles '1970-01-01T00:00:00Z' format
    '''
    try:
        return datetime.strptime(date_string, '%Y-%m-%dT%H:%M:%S%z')
    except ValueError as e:
        raise RuntimeError(f'Failed to parse date: {e}')

def get_last_value_for_signal(output_signal):
    last_value = 'N/A'
    if output_signal is not None and daq.ISignal.can_cast_from(output_signal):
        try:
            sig = daq.ISignal.cast_from(output_signal)
            last_value = sig.last_value
            desc = sig.descriptor
            unit_symbol = ''
            if desc is not None and desc.unit is not None and desc.unit.symbol is not None:
                unit_symbol = str(desc.unit.symbol)
            origin_str = signal_time_domain_check(sig)
            if origin_str is not None:
                try:
                    origin = datetime.fromisoformat(origin_str)
                except ValueError:
                    origin = parse_iso_string(origin_str)
                if last_value is not None:
                    tick_value = int(last_value)
                    last_value_in_seconds = (
                        tick_value
                        * desc.tick_resolution.numerator
                        / desc.tick_resolution.denominator
                    )
                    last_value = origin + timedelta(seconds=last_value_in_seconds)

            if isinstance(last_value, float):
                # Keep enough precision but avoid long noisy tails.
                value_str = f'{last_value:.6g}'
                last_value = f'{value_str} {unit_symbol}'.strip() if unit_symbol else value_str
            elif isinstance(last_value, int):
                if unit_symbol:
                    last_value = f'{last_value} {unit_symbol}'
            elif isinstance(last_value, datetime):
                # Human-readable timestamp for time-domain signals.
                last_value = last_value.strftime('%Y-%m-%d %H:%M:%S.%f').rstrip('0').rstrip('.')
            elif last_value is None:
                last_value = 'N/A'

        except RuntimeError as e:
            print(f'Error reading last value: {e}')
    return last_value


def str_to_num_or_eval(num_str: str):
    '''
    Converts a string either to an int or a float or an daq.EvalValue if the string starts with 'eval:' prefix.
    The prefix is removed before conversion.
    '''
    try:
        return int(num_str)
    except ValueError:
        try:
            return float(num_str)
        except ValueError:
            if num_str.startswith('eval:'):
                return daq.EvalValue(num_str[5:].strip())
            return num_str


def value_to_coretype(value, coretype: daq.CoreType):
    # removing unit symbols
    if coretype in (daq.CoreType.ctBool, daq.CoreType.ctInt, daq.CoreType.ctFloat):
        value = str.split(str.strip(value), ' ')[0]
    if coretype == daq.CoreType.ctBool:
        value = value.lower()
        if value in yes_no_inv.keys():
            return daq.Boolean(yes_no_inv[value])
        else:
            return daq.Boolean(bool(value))
    if coretype == daq.CoreType.ctInt:
        return daq.Integer(int(value))
    if coretype == daq.CoreType.ctFloat:
        return daq.Float(float(value))
    if coretype == daq.CoreType.ctString:
        return daq.String(str(value))
    raise ValueError(f'Unsupported core type: {coretype}')


def get_item_path(tree, item_id):
    path = []
    while item_id:
        item_text = tree.item(item_id, 'text').strip()
        path.insert(0, item_text)
        item_id = tree.parent(item_id)
    return path


def get_property_for_path(context, path: list, property_object: daq.IPropertyObject) -> daq.IProperty:
    if context and path and property_object:
        for property in context.properties_of_component(property_object):
            if property.name == path[0]:
                if len(path) == 1:
                    return property
                if daq.IPropertyObject.can_cast_from(property.value):
                    casted_property = daq.IPropertyObject.cast_from(
                        property.value)
                    return get_property_for_path(context, path[1:], casted_property)
    return None


def get_attributes_of_node(node):

    # to filter callables
    def is_callable(obj, key):
        try:
            return callable(getattr(obj, key))
        except Exception:
            return True

    try:
        attributes = dir(node)
        attributes = list(
            filter(lambda x: not x.startswith('_'), attributes))
        attributes = list(
            filter(lambda x: not is_callable(node, x), attributes))
    except Exception:
        attributes = []
    return attributes


def show_error(title, message, parent=None):
    messagebox.showerror(title, message, parent=parent)


def snake_case_to_title(snake_case: str):
    return snake_case.replace('_', ' ').title()


def title_to_snake_case(title: str):
    return title.lower().replace(' ', '_')


def prettify_unit(unit: daq.IStruct):
    return unit.symbol if unit is not None and unit.symbol is not None else ''


def prettify_bool(value):
    return yes_no[value]


metadata_converters = {
    'is_referenced': prettify_bool,
    'unit': prettify_unit,
    'read_only': prettify_bool,
    'visible': prettify_bool
}

def is_device_connected(device: daq.IDevice):
    status_container = device.status_container
    try:
        connection_status = status_container.get_status("ConnectionStatus")
        return connection_status.name == "Connected"
    except Exception:
        # No ConnectionStatus at all, which is the normal case for anything not
        # reached over a network: the local instance and the reference devices
        # never publish one. Treating those as disconnected would mark the whole
        # local tree as such, so absence of a status means nothing is wrong.
        return True

def update_properties(target: daq.IPropertyObject, source: daq.IPropertyObject):
    """Update target properties with values from source IPropertyObject.
    Assuming both have the same structure, this should result in target
    being identical copy of source."""
    for property in target.all_properties:
        prop_name = property.name
        if not source.has_property(prop_name):
            continue

        source_prop = source.get_property(prop_name)
        target_prop = target.get_property(prop_name)

        if source_prop.value_type != target_prop.value_type:
            continue

        if target_prop.read_only:
            continue

        if source_prop.value_type == daq.CoreType.ctObject:
            update_properties(target_prop.value, source_prop.value)
        else:
            target.set_property_value(prop_name, source_prop.value)

def make_banner(parent, text):
    """Shared section-header banner used across signal, input, and recorder views."""
    _banner_bg = '#afafaf'
    _banner_fg = 'white'
    bar = tk.Frame(parent, bg=_banner_bg, bd=0, highlightthickness=0)
    bar.pack(fill=tk.X, pady=(8))
    tk.Label(bar, text=text, bg=_banner_bg, fg=_banner_fg,
             font=('TkDefaultFont', 10, 'bold')).pack(
        side=tk.LEFT, padx=6, pady=2)
    return bar


def poll_signal_rows(widget, rows, interval_ms=200, _job_attr='_signal_refresh_job'):
    """Schedules a recurring refresh of OutputSignalRow widgets.
    
    Stores the after-job ID on `widget` under `_job_attr` so callers can
    cancel it on destroy.  Automatically stops when the widget is gone.
    """
    setattr(widget, _job_attr, None)

    def _tick():
        setattr(widget, _job_attr, None)
        if not widget.winfo_exists():
            return
        if widget.winfo_ismapped():
            for row in list(rows):
                try:
                    if row.winfo_exists():
                        row.refresh()
                except Exception:
                    pass
        setattr(widget, _job_attr, widget.after(interval_ms, _tick))

    setattr(widget, _job_attr, widget.after(interval_ms, _tick))
    
def parse_origin(origin_str):
    """Try fromisoformat first, fall back to strptime for the trailing Z format.
    Returns None if the string can't be parsed at all."""
    if not origin_str:
        return None
    dt = None
    try:
        dt = datetime.fromisoformat(origin_str)
    except ValueError:
        pass
    if dt is None:
        try:
            dt = datetime.strptime(origin_str, '%Y-%m-%dT%H:%M:%S%z')
        except ValueError:
            pass
    return dt