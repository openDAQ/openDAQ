from opendaq import IPropertyObject
from .base import PropertyView


class ObjectPropertyView(PropertyView):
    def editable(self) -> bool:
        return False

    def format_value(self) -> str:
        return f"IPropertyObject {{{self.prop.name}}}"

    def build_tree(self, tree, parent_id):
        """Build tree for nested IPropertyObject"""
        try:
            nested_obj = IPropertyObject.cast_from(self.prop.value)
            tree.build_tree(parent_id, nested_obj)
        except Exception:
            pass
