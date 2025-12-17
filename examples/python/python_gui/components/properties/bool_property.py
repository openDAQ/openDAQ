from .base import PropertyView


class BoolPropertyView(PropertyView):
    def editable(self) -> bool:
        return not self.prop.read_only

    def format_value(self) -> str:
        return "True" if bool(self.prop.value) else "False"

    def toggle(self):
        self.prop.value = not bool(self.prop.value)

    def handle_double_click(self, tree, item_id, column):
        """Toggle boolean value on double click instead of opening editor"""
        try:
            self.toggle()
            tree.set(item_id, "value", self.format_value())
        except Exception:
            pass
        # Return None to prevent default editor from opening
        return None
