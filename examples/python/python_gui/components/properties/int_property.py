import tkinter as tk
from tkinter import ttk
from opendaq import IDict
from .base import PropertyView


class IntPropertyView(PropertyView):
    def __init__(self, prop):
        super().__init__(prop)
        self.selection_map = None

    def create_editor(self, parent) -> tk.Widget:
        sv = self._get_selection_values()
        if not sv:
            return super().create_editor(parent)

        cb = self._create_selection_combobox(parent, sv)
        cb.after(1, lambda: self._open_combobox(cb))
        return cb

    def _get_selection_values(self):
        """Get selection values from property if available"""
        sv = getattr(self.prop, "selection_values", None)
        return sv if sv and len(sv) > 0 else None

    def _create_selection_combobox(self, parent, sv) -> ttk.Combobox:
        """Create combobox from selection values (dict or list)"""
        self.selection_map = {}
        values = []
        cur_value = int(self.prop.value)
        cur_label = None

        if IDict.can_cast_from(sv):
            # Dict: keys are selection values, values are labels
            for k, v in sv.items():
                key = int(str(k), 0)
                label = str(v)
                self.selection_map[label] = key
                values.append(label)
                if key == cur_value:
                    cur_label = label
        else:
            # List: index is the selection value, item is the label
            values = [str(v) for v in sv]
            for idx, label in enumerate(values):
                self.selection_map[label] = idx
            if 0 <= cur_value < len(values):
                cur_label = values[cur_value]

        cb = ttk.Combobox(parent, values=values, state="readonly")
        cb.set(cur_label if cur_label else (values[0] if values else ""))
        return cb

    @staticmethod
    def _open_combobox(cb):
        """Open combobox dropdown programmatically"""
        try:
            cb.tk.call('ttk::combobox::Post', cb._w)
        except Exception:
            pass

    def parse(self, text: str):
        if self.selection_map:
            v = self.selection_map.get(text)
            if v is not None:
                return int(v)
        return int(text.strip(), 0)

    def format_value(self) -> str:
        sv = self._get_selection_values()
        if not sv:
            return str(self.prop.value)

        cur = int(self.prop.value)

        if IDict.can_cast_from(sv):
            for k, v in sv.items():
                if int(str(k), 0) == cur:
                    return str(v)

        if 0 <= cur < len(sv):
            return str(list(sv)[cur])

        return str(self.prop.value)
