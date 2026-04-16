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
        self._suggestions_list = None

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

        self.dropdown.bind('<MouseWheel>', lambda e: 'break')
        self.dropdown.bind('<Button-4>', lambda e: 'break')
        self.dropdown.bind('<Button-5>', lambda e: 'break')

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
        return f'{padded_name} | {short_id}'

    def _name_text_for_signal(self, signal):
        return signal.name if signal.name is not None else ''

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

    def handle_dropdown_focus_out(self, event):
        self.after(50, self._hide_suggestions_if_focus_lost)

    def handle_dropdown_down(self, event):
        if self._filtered_signals:
            self.show_suggestions(self._filtered_signals)
            if self._suggestions_list is not None:
                self._suggestions_list.focus_set()
                if self._suggestions_list.size() > 0:
                    self._suggestions_list.selection_clear(0, tk.END)
                    self._suggestions_list.selection_set(0)
                    self._suggestions_list.activate(0)
            return 'break'

    def _hide_suggestions_if_focus_lost(self):
        try:
            focus_widget = self.focus_get()
            if self._suggestions_list is not None and focus_widget == self._suggestions_list:
                return
            if focus_widget == self.dropdown:
                return
        except Exception:
            pass
        self.hide_suggestions()

    def show_suggestions(self, values):
        if not values:
            self.hide_suggestions()
            return

        if self._suggestions_popup is None or not self._suggestions_popup.winfo_exists():
            self._suggestions_popup = tk.Toplevel(self)
            self._suggestions_popup.withdraw()
            self._suggestions_popup.overrideredirect(True)
            self._suggestions_popup.transient(self.winfo_toplevel())

            self._suggestions_list = tk.Listbox(self._suggestions_popup, exportselection=False)
            self._suggestions_list.pack(fill=tk.BOTH, expand=True)
            self._suggestions_list.bind('<ButtonRelease-1>', self.handle_suggestion_pick)
            self._suggestions_list.bind('<Return>', self.handle_suggestion_pick)
            self._suggestions_list.bind('<FocusOut>', lambda e: self.after(50, self._hide_suggestions_if_focus_lost))

            self._suggestions_list.bind('<MouseWheel>', lambda e: 'break')
            self._suggestions_list.bind('<Button-4>', lambda e: 'break')  # Linux scroll up
            self._suggestions_list.bind('<Button-5>', lambda e: 'break')  # Linux scroll down

        self._suggestions_list.delete(0, tk.END)
        for item in values:
            self._suggestions_list.insert(tk.END, item)

        x = self.dropdown.winfo_rootx()
        y = self.dropdown.winfo_rooty() + self.dropdown.winfo_height()
        width = self.dropdown.winfo_width()
        height = min(max(len(values), 1), 10) * 20 + 4
        self._suggestions_popup.geometry(f'{width}x{height}+{x}+{y}')
        self._suggestions_popup.deiconify()
        self._suggestions_popup.lift()

    def hide_suggestions(self):
        if self._suggestions_popup is not None and self._suggestions_popup.winfo_exists():
            self._suggestions_popup.withdraw()

    def handle_suggestion_pick(self, event):
        if self._suggestions_list is None:
            return
        sel = self._suggestions_list.curselection()
        if not sel:
            return
        value = self._suggestions_list.get(sel[0])
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
