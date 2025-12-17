from opendaq import IPropertyObject
from .base import PropertyView


class NestedObjectPropertyView(PropertyView):
    """Property view for nested IPropertyObject properties.

    Note: This is NOT the same as PropertyObjectView (which is a full ttk.Treeview widget).
    This class just recursively adds nested object's properties to the parent tree.
    """
    def editable(self) -> bool:
        return False

    def format_value(self) -> str:
        return f"IPropertyObject {{{self.prop.name}}}"

    def build_tree(self, tree, parent_id, current_path: str = ""):
        """Build tree for nested IPropertyObject"""
        try:
            nested_obj = IPropertyObject.cast_from(self.prop.value)
            tree.build_tree(parent_id, nested_obj, current_path)
        except Exception:
            pass
