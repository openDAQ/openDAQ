from .base import PropertyView


class FloatPropertyView(PropertyView):
    def parse(self, text: str):
        return float(text.strip())
