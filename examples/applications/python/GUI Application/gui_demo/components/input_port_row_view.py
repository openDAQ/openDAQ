import tkinter as tk
from tkinter import ttk

import opendaq as daq

from .. import utils
from ..event_port import EventPort
from ..app_context import AppContext
from .attributes_dialog import AttributesDialog


class InputPortRowView(ttk.Frame):
    def __init__(self, parent, input_port, context=None, **kwargs):
        ttk.Frame.__init__(self, parent, **kwargs)
        self.parent = parent
        self.input_port = input_port
        self.selection = ''
        self.context = context
        self.event_port = EventPort(self)
        self._all_signals = []
        self._filtered_signals = []
        self._signal_display_to_signal = {}
        self._signal_name_to_signal = {}
        self._suggestions_popup = None
        self._suggestions_canvas = None
        self._suggestions_inner = None
        self._suggestion_rows = []
        self._active_suggestion = -1

        self.configure(padding=(10, 5))

        ttk.Label(self, text=input_port.name).grid(
            row=0, column=0, sticky=tk.W)
        self.input_var = tk.StringVar()
        self.dropdown = ttk.Combobox(self, textvariable=self.input_var)
        self.dropdown.grid(row=0, column=1, sticky=tk.EW)
        self.dropdown.bind('<<ComboboxSelected>>', self.handle_dropdown_select)
        self.dropdown.bind('<KeyRelease>', self.handle_dropdown_search)
        self.dropdown.bind('<Return>', self.handle_dropdown_enter)
        self.dropdown.bind('<FocusOut>', self.handle_dropdown_focus_out)
        self.dropdown.bind('<Down>', self.handle_dropdown_down)
        self.dropdown.bind('<Up>', self.handle_dropdown_up)
        self.dropdown.bind('<Escape>', self.handle_dropdown_escape)
        # Intercept clicks so our own two-color suggestions popup (name in
        # black, global id in gray) opens instead of the native combobox
        # dropdown, which can only paint every row a single color.
        self.dropdown.bind('<Button-1>', self.handle_dropdown_click)

        self.edit_icon = context.icons['settings'] if context and context.icons and 'settings' in context.icons else None

        self.edit_button = tk.Button(
            self, text='Edit', image=self.edit_icon, borderwidth=0, command=self.handle_edit_clicked)
        self.edit_button.grid(row=0, column=2, sticky=tk.E)

        # Let the signal selector take as much horizontal space as possible.
        self.grid_columnconfigure(0, weight=0, minsize=100)
        self.grid_columnconfigure(1, weight=1)
        self.grid_columnconfigure(2, weight=0, minsize=40)

        device = utils.root_device(input_port)
        device = device if device is not None and daq.IDevice.can_cast_from(
            device) else None

        if device is not None:
            self.device = daq.IDevice.cast_from(device)
            self.refresh()

    def refresh(self):
        if self.device is not None:
            self.context.update_signals_for_device(self.device)
            self.fill_dropdown()
        if self.input_port is not None and self.input_port.signal is not None:
            connected_signal = self.input_port.signal
            connected_name = self._name_text_for_signal(connected_signal)
            self.input_var.set(connected_name)
            self.dropdown.set(connected_name)

    def fill_dropdown(self):
        self._signal_display_to_signal = {}
        self._signal_name_to_signal = {}
        signals_dict = self.context.signals_for_device(self.device)
        signals = [signal_id for signal_id in signals_dict.keys()]
        if not self.context.view_hidden_components:
            signals = list(filter(lambda signal_id: signals_dict[signal_id].visible, signals))
        display_values = []
        for signal_id in signals:
            signal = signals_dict[signal_id]
            display = self._display_text_for_signal(signal)
            name_text = self._name_text_for_signal(signal)
            # Keep the first mapping in case of duplicate display text.
            if display not in self._signal_display_to_signal:
                self._signal_display_to_signal[display] = signal
                display_values.append(display)
            if name_text not in self._signal_name_to_signal:
                self._signal_name_to_signal[name_text] = signal
        signals = ['none'] + display_values

        self._all_signals = signals
        self._filtered_signals = self._all_signals
        self.dropdown['values'] = self._all_signals
        self.selection = ''

    def _display_text_for_signal(self, signal):
        signal_name = signal.name if signal.name is not None else ''
        short_id = self.context.short_id(signal.global_id)
        padded_name = signal_name.ljust(10)
        return f'{padded_name}  {short_id}'

    def _name_text_for_signal(self, signal):
        return signal.name if signal.name is not None else ''

    # Split a display string back into (name, global id) so a suggestion row
    # can show the name in black and the global id in gray.
    def _name_gid_for_display(self, display):
        signal = self._signal_display_to_signal.get(display)
        if signal is None:
            return display, ''
        name = signal.name if signal.name is not None else ''
        gid = self.context.short_id(signal.global_id)
        return name, gid

    def handle_dropdown_select(self, event):
        self.selection = self.input_var.get()
        self.handle_connect_clicked()

    def handle_dropdown_search(self, event):
        if event.keysym in ('Up', 'Down', 'Left', 'Right', 'Return', 'Escape', 'Tab'):
            return
        typed = self.input_var.get().strip().lower()
        if not typed:
            filtered = self._all_signals
        else:
            filtered = [s for s in self._all_signals if typed in s.lower()]
        self._filtered_signals = filtered
        self.dropdown['values'] = filtered
        self.selection = self.input_var.get()
        self.show_suggestions(filtered)
        # Keep typing uninterrupted: preserve focus/caret in the combobox entry.
        self.dropdown.focus_set()
        self.dropdown.icursor(tk.END)

    def handle_dropdown_enter(self, event):
        if self._suggestions_visible() and self._pick_active_suggestion():
            return 'break'
        selection = self.input_var.get().strip()
        if not selection:
            return
        candidates = [s for s in self._all_signals if s.lower() == selection.lower()]
        if not candidates:
            name_matches = [name for name in self._signal_name_to_signal.keys() if name.lower() == selection.lower()]
            if name_matches:
                signal = self._signal_name_to_signal[name_matches[0]]
                candidates = [self._display_text_for_signal(signal)]
        if candidates:
            self.input_var.set(candidates[0])
            self.selection = candidates[0]
            self.hide_suggestions()
            self.handle_connect_clicked()
            
    def handle_dropdown_escape(self, event):
        self.hide_suggestions()
        if self.input_port is not None and self.input_port.signal is not None:
            connected_name = self._name_text_for_signal(self.input_port.signal)
            self.input_var.set(connected_name)
        else:
            self.input_var.set('')
        self.selection = ''
        self._filtered_signals = self._all_signals
        self.dropdown['values'] = self._all_signals
        return 'break'

    def handle_dropdown_focus_out(self, event):
        self.after(50, self._hide_suggestions_if_focus_lost)

    def handle_dropdown_click(self, event):
        # Toggle our custom popup and prevent the native popdown from opening
        # (returning 'break' stops the combobox's built-in class binding).
        self.dropdown.focus_set()
        self.dropdown.icursor(tk.END)
        if self._suggestions_visible():
            self.hide_suggestions()
        else:
            self._filtered_signals = self._all_signals
            self.show_suggestions(self._all_signals)
        return 'break'

    def handle_dropdown_down(self, event):
        if not self._suggestions_visible():
            if self._filtered_signals:
                self.show_suggestions(self._filtered_signals)
            else:
                return 'break'
        self._move_active_suggestion(1)
        return 'break'

    def handle_dropdown_up(self, event):
        if self._suggestions_visible():
            self._move_active_suggestion(-1)
            return 'break'

    def _hide_suggestions_if_focus_lost(self):
        try:
            focus_widget = self.focus_get()
            if focus_widget == self.dropdown:
                return
            if self._focus_in_popup(focus_widget):
                return
        except Exception:
            pass
        self.hide_suggestions()
        if self.input_port is not None and self.input_port.signal is not None:
            connected_name = self._name_text_for_signal(self.input_port.signal)
            self.input_var.set(connected_name)
        else:
            self.input_var.set('')
        self.selection = ''
        self._filtered_signals = self._all_signals
        self.dropdown['values'] = self._all_signals

    def show_suggestions(self, values):
        if not values:
            self.hide_suggestions()
            return

        if self._suggestions_popup is None or not self._suggestions_popup.winfo_exists():
            self._suggestions_popup = tk.Toplevel(self)
            self._suggestions_popup.withdraw()
            self._suggestions_popup.overrideredirect(True)
            self._suggestions_popup.transient(self.winfo_toplevel())

            outer = tk.Frame(self._suggestions_popup, relief=tk.SOLID,
                             borderwidth=1, bg='white')
            outer.pack(fill=tk.BOTH, expand=True)

            canvas = tk.Canvas(outer, highlightthickness=0, bg='white', height=1)
            scrollbar = ttk.Scrollbar(
                outer, orient=tk.VERTICAL, command=canvas.yview)
            inner = tk.Frame(canvas, bg='white')
            inner.bind('<Configure>', lambda e: canvas.configure(
                scrollregion=canvas.bbox('all')))
            self._suggestions_window = canvas.create_window(
                (0, 0), window=inner, anchor='nw')
            canvas.bind('<Configure>', lambda e: canvas.itemconfigure(
                self._suggestions_window, width=e.width))
            canvas.configure(yscrollcommand=scrollbar.set)
            canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
            scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
            self._suggestions_canvas = canvas
            self._suggestions_inner = inner

        inner = self._suggestions_inner
        for child in inner.winfo_children():
            child.destroy()
        self._suggestion_rows = []
        self._active_suggestion = -1

        # Each row shows the signal name in black and the global id in gray.
        for display in values:
            name, gid = self._name_gid_for_display(display)
            r = tk.Frame(inner, bg='white')
            r.pack(fill=tk.X)
            ln = tk.Label(r, text=name, fg='black', bg='white', anchor='w')
            ln.pack(side=tk.LEFT, padx=(6, 0))
            widgets = [r, ln]
            if gid:
                li = tk.Label(r, text=gid, fg='#808080', bg='white', anchor='w')
                li.pack(side=tk.LEFT, padx=(8, 6))
                widgets.append(li)
            index = len(self._suggestion_rows)
            for w in widgets:
                w.bind('<ButtonRelease-1>',
                       lambda e, i=index: self._pick_suggestion_index(i))
                w.bind('<Enter>',
                       lambda e, i=index: self._set_active_suggestion(i))
                w.bind('<MouseWheel>', self._suggestions_wheel)
            self._suggestion_rows.append(
                {'display': display, 'widgets': tuple(widgets)})

        inner.update_idletasks()
        count = max(len(self._suggestion_rows), 1)
        per_row = max(inner.winfo_reqheight() // count, 20)
        visible = min(count, 10)
        canvas_height = per_row * visible
        self._suggestions_canvas.configure(height=canvas_height)

        x = self.dropdown.winfo_rootx()
        y = self.dropdown.winfo_rooty() + self.dropdown.winfo_height()
        width = self.dropdown.winfo_width()
        height = canvas_height + 2
        self._suggestions_popup.geometry(f'{width}x{height}+{x}+{y}')
        self._suggestions_popup.deiconify()
        self._suggestions_popup.lift()

    def hide_suggestions(self):
        if self._suggestions_popup is not None and self._suggestions_popup.winfo_exists():
            self._suggestions_popup.withdraw()
        self._active_suggestion = -1

    def _suggestions_visible(self):
        return (self._suggestions_popup is not None
                and self._suggestions_popup.winfo_exists()
                and self._suggestions_popup.winfo_ismapped())

    def _focus_in_popup(self, widget):
        if widget is None or self._suggestions_popup is None:
            return False
        w = widget
        while w is not None:
            if w == self._suggestions_popup:
                return True
            w = getattr(w, 'master', None)
        return False

    def _suggestions_wheel(self, event):
        if self._suggestions_canvas is not None:
            self._suggestions_canvas.yview_scroll(
                int(-1 * (event.delta / 120)), 'units')
        return 'break'

    def _highlight_suggestion_row(self, index, on):
        if not (0 <= index < len(self._suggestion_rows)):
            return
        bg = '#e4e4e4' if on else 'white'
        for w in self._suggestion_rows[index]['widgets']:
            try:
                w.configure(bg=bg)
            except Exception:
                pass

    def _set_active_suggestion(self, index):
        if 0 <= self._active_suggestion < len(self._suggestion_rows):
            self._highlight_suggestion_row(self._active_suggestion, False)
        self._active_suggestion = index
        if 0 <= index < len(self._suggestion_rows):
            self._highlight_suggestion_row(index, True)
            self._scroll_active_into_view()

    def _move_active_suggestion(self, delta):
        if not self._suggestion_rows:
            return
        new_index = self._active_suggestion + delta
        if new_index < 0:
            new_index = 0
        if new_index >= len(self._suggestion_rows):
            new_index = len(self._suggestion_rows) - 1
        self._set_active_suggestion(new_index)

    def _scroll_active_into_view(self):
        if not self._suggestion_rows or self._suggestions_canvas is None:
            return
        total = len(self._suggestion_rows)
        index = self._active_suggestion
        self._suggestions_canvas.update_idletasks()
        top, bottom = self._suggestions_canvas.yview()
        frac = index / total
        row_frac = 1.0 / total
        if frac < top:
            self._suggestions_canvas.yview_moveto(frac)
        elif frac + row_frac > bottom:
            self._suggestions_canvas.yview_moveto(
                max(0.0, frac + row_frac - (bottom - top)))

    def _pick_active_suggestion(self):
        if 0 <= self._active_suggestion < len(self._suggestion_rows):
            self._commit_suggestion(
                self._suggestion_rows[self._active_suggestion]['display'])
            return True
        return False

    def _pick_suggestion_index(self, index):
        if 0 <= index < len(self._suggestion_rows):
            self._commit_suggestion(self._suggestion_rows[index]['display'])

    def _commit_suggestion(self, value):
        self.input_var.set(value)
        self.selection = value
        self.hide_suggestions()
        self.dropdown.focus_set()
        self.dropdown.icursor(tk.END)
        self.handle_connect_clicked()

    def handle_edit_clicked(self):
        if self.input_port is not None:
            AttributesDialog(self, 'Attributes', self.input_port, self.context).show()

    def handle_connect_clicked(self):
        if self.selection == 'none':
            self.input_port.disconnect()
            self.input_var.set('')
            self.event_port.emit()
            return

        if self.selection == '':
            return

        selected_signal = self._signal_display_to_signal.get(self.selection)
        if selected_signal is None:
            selected_signal = self._signal_name_to_signal.get(self.selection)
        if selected_signal is None:
            return

        self.input_port.connect(selected_signal)
        self.input_var.set(self._name_text_for_signal(selected_signal))
        self.event_port.emit()
