from .base import PropertyView


class FuncPropertyView(PropertyView):
    def editable(self) -> bool:
        return False

    def format_value(self) -> str:
        return "Function"
