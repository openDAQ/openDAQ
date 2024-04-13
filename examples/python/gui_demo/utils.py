import opendaq as daq
import tkinter as tk
from tkinter import ttk

yes_no = ['No','Yes']

yes_no_inv = {
    'No':  False,
    'Yes': True,
}


def find_component(id, parent=None, convert_id=True):
    if convert_id:
        split_id = id.split('/')
        id = '/'.join(split_id[2:])
    return parent.find_component(id)


def treeview_get_first_selection(treeview):
    sel = treeview.selection()
    if len(sel) == 0:
        return None
    return sel[0]


def get_nearest_device(component: daq.IComponent, default=None):
    while component:
        if daq.IDevice.can_cast_from(component):
            return daq.IDevice.cast_from(component)
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


def show_selection(title, current_value, values: daq.IList):
    global result
    result = current_value
    top = tk.Toplevel()
    top.resizable(False, False)

    tk.Label(top, text=title).pack()

    def select_value(v):
        global result
        top.destroy()
        result = v

    def make_closure(v):
        return lambda: select_value(v)

    def fill_buttons(idx, value):
        if current_value == idx:
            sel_text = "* "
        else:
            sel_text = ""
        button = tk.Button(top, text=sel_text + value,
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
    top.geometry("+{}+{}".format(pr, pd))

    show_modal(top)
    return result

def root_device(node):
    while node.parent:
        node = node.parent
    return node
