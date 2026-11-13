import tkinter as tk
from tkinter import ttk, filedialog, messagebox

from .. import AEC_stub as stub
from ..app_context import AppContext
from .dialog import Dialog


def file_types(field):
    """The file dialog filter for a FilePath field: Extensions is comma-separated, without the dot."""
    extensions = [e.strip().lstrip('.') for e in field.metadata.get('Extensions', '').split(',') if e.strip()]
    patterns = [(field.name, ' '.join(f'*.{e}' for e in extensions))] if extensions else []
    return patterns + [('All files', '*.*')]


class LoginDialog(Dialog):
    """Asks for the fields of one login method, with the connection string and method first."""

    def __init__(self, parent, context: AppContext, request):
        super().__init__(parent, 'Login', context)
        self.result = None
        method = request.authentication_method
        name = f'{request.display_name}  ' if request.display_name else ''
        ttk.Label(self, text=f'{name}{request.connection_string}', font=('TkDefaultFont', 10, 'bold')).pack(
            anchor=tk.W, pady=(5, 0))
        ttk.Label(self, text=f'{request.component_type_id} asks for {method.description}', foreground='#868e96').pack(
            anchor=tk.W, pady=(0, 8))

        body = ttk.Frame(self)
        body.pack(fill=tk.X)
        self.vars = {}
        first = None
        for row, field in enumerate(method.fields):
            text = field.name if field.required else f'{field.name} (optional)'
            ttk.Label(body, text=text).grid(row=row, column=0, sticky=tk.W, padx=(0, 12), pady=2)
            var = tk.StringVar()
            entry = ttk.Entry(body, textvariable=var, width=36, show='*' if field.kind == 'Secret' else '')
            entry.grid(row=row, column=1, sticky=tk.W, pady=2)
            if field.kind == 'FilePath':
                ttk.Button(body, text='…', width=3, command=lambda v=var, f=field: self.browse(v, f)).grid(
                    row=row, column=2, padx=(4, 0))
            self.vars[field.id] = var
            first = first or entry

        buttons = ttk.Frame(self, padding=(0, 10, 0, 4))
        buttons.pack(fill=tk.X)
        ttk.Button(buttons, text='Log in', command=self.ok).pack(side=tk.RIGHT)
        ttk.Button(buttons, text='Cancel', command=self.close).pack(side=tk.RIGHT, padx=(0, 8))
        self.bind('<Return>', lambda e: self.ok())
        if first is not None:
            self.after_idle(first.focus_set)

    def browse(self, var, field):
        path = filedialog.askopenfilename(parent=self, title=field.name, filetypes=file_types(field))
        if path:
            var.set(path)

    def ok(self):
        self.result = {field_id: var.get() for field_id, var in self.vars.items() if var.get()}
        self.destroy()

    def close(self):
        self.result = None
        self.destroy()


class GuiCredentialProvider(stub.CredentialProvider):
    """0004 and 0005 credential provider of the GUI: a login dialog, and the SSH-style question for a
    server nothing verified."""

    def __init__(self, parent, context: AppContext):
        self.parent = parent
        self.context = context

    def request_credentials(self, request):
        dialog = LoginDialog(self.parent, self.context, request)
        dialog.show()
        if dialog.result is None:
            raise stub.CancelledException('Cancelled')
        return dialog.result

    def confirm_server(self, request, fingerprint):
        name = f' ({request.display_name})' if request.display_name else ''
        answer = messagebox.askyesnocancel(
            'Unknown server',
            f'The authenticity of {request.connection_string}{name} cannot be established.\n\n'
            f'Certificate fingerprint is {fingerprint}.\n\nContinue connecting?',
            icon=messagebox.WARNING, parent=self.parent)
        if answer is None:
            raise stub.CancelledException('Cancelled')
        return answer
