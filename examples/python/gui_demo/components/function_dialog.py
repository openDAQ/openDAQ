import tkinter as tk
from tkinter import ttk

import opendaq as daq

from .dialog import Dialog


class FunctionDialog(Dialog):

    def __init__(self, parent, node: daq.IProperty,
                 function, context, **kwargs):
        Dialog.__init__(self, parent, 'Execute Function', context, **kwargs)
        self.node = node
        self.function = function

        self.arguments = {}

        self.minsize(600, 200)

        # block with arguments
        ttk.Label(self, text='Arguments', anchor=tk.W).pack(
            fill=tk.X,
            pady=5,
            padx=5)
        self.arguments_frame = ttk.Frame(self, border=1, relief=tk.GROOVE)
        self.arguments_frame.pack(fill=tk.X)

        if self.node.callable_info.arguments:
            for argument in self.node.callable_info.arguments:
                self.arguments[argument.Name] = tk.StringVar(self)
                frame = ttk.Frame(self.arguments_frame)
                frame.pack(fill=tk.X)
                ttk.Label(
                    frame,
                    text=f'{argument.Name}:',
                    anchor=tk.W).pack(
                    fill=tk.X,
                    padx=5,
                    pady=(0, 5))
                ttk.Entry(frame, textvariable=self.arguments[argument.Name]).pack(
                    fill=tk.X, padx=5, pady=(0, 5))
        else:
            ttk.Label(
                self.arguments_frame,
                text='No arguments').pack(
                side=tk.LEFT,
                expand=True,
                pady=20)

        # block with return value
        ttk.Label(
            self,
            text='Return value',
            anchor=tk.W).pack(
            side=tk.TOP,
            fill=tk.X,
            padx=5,
            pady=5)
        self.return_value_frame = ttk.Frame(self, border=1, relief=tk.GROOVE)
        self.return_value_frame.pack(side=tk.TOP, fill=tk.X)
        self.return_value = tk.StringVar(self)
        ttk.Entry(
            self.return_value_frame,
            textvariable=self.return_value,
            state=tk.DISABLED).pack(side=tk.LEFT, fill=tk.X, padx=5, pady=10, expand=True)
        ttk.Button(self.return_value_frame, text='Copy', command=self.copy_return_value).pack(
            side=tk.LEFT,
            padx=(0, 5),
            pady=10)

        # buttons
        self.buttons_frame = ttk.Frame(self)
        self.buttons_frame.pack(side=tk.BOTTOM, fill=tk.X)
        self.exec_button = ttk.Button(
            self.buttons_frame,
            text='Execute',
            command=self.exec_clicked)
        self.exec_button.pack(side=tk.RIGHT, padx=5, pady=5)
        self.cancel_button = ttk.Button(
            self.buttons_frame,
            text='Close',
            command=self.cancel_clicked)
        self.cancel_button.pack(side=tk.RIGHT, padx=5, pady=5)

    def create_argument(self, value_type_num: int, *args):
        value_type = daq.CoreType(value_type_num)
        if value_type == daq.CoreType.ctBool:
            return bool(*args)
        if value_type == daq.CoreType.ctInt:
            return int(*args)
        if value_type == daq.CoreType.ctFloat:
            return float(*args)
        if value_type == daq.CoreType.ctString:
            return str(*args)
        if value_type == daq.CoreType.ctList:
            return list(*args)
        return None

    def exec_clicked(self):
        ret = None
        try:
            args = []
            if self.node.callable_info.arguments:
                for argument in self.node.callable_info.arguments:
                    args.append(self.create_argument(argument.Type,
                                self.arguments[argument.Name].get()))

            ret = self.function(*args)
        except (Exception, ValueError) as e:
            ret = e
        self.return_value.set(str(ret))

    def cancel_clicked(self):
        self.destroy()

    def copy_return_value(self):
        self.clipboard_clear()
        self.clipboard_append(self.return_value.get())
