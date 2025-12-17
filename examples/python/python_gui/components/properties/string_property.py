from .base import PropertyView


class StringPropertyView(PropertyView):
    def format_value(self) -> str:
        v = self.prop.value
        return "" if v is None else str(v)

    def parse(self, text: str):
        return text
