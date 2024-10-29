import os
import tkinter as tk
from tkinter import ttk

import opendaq as daq

yes_no = ['No', 'Yes']

yes_no_inv = {
    'No':  False,
    'Yes': True,
}


def find_component(id, parent=None, convert_id=True):
    if convert_id:
        split_id = id.split('/')
        id = '/'.join(split_id[2:])
    return None if parent is None else parent.find_component(id)


def treeview_get_first_selection(treeview):
    sel = treeview.selection()
    if len(sel) == 0:
        return None
    return sel[0]


def get_nearest_device(component, default=None):
    while component:
        if daq.IDevice.can_cast_from(component):
            return daq.IDevice.cast_from(component)
        component = component.parent

    return default


def get_nearest_fb(component):
    while component:
        if daq.IFunctionBlock.can_cast_from(component):
            return daq.IFunctionBlock.cast_from(component)
        component = component.parent
    return None


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

    show_modal(top)
    return result


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


def load_and_resize_image(filename, x_subsample=10, y_subsample=10):
    image = tk.PhotoImage(file=filename)
    return image.subsample(x_subsample, y_subsample)


def get_last_value_for_signal(output_signal):
    last_value = 'N/A'
    if output_signal is not None and daq.ISignal.can_cast_from(output_signal):
        try:
            last_value = daq.ISignal.cast_from(output_signal).last_value
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
    if coretype == daq.CoreType.ctBool:
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
        item_text = tree.item(item_id, 'text')
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
