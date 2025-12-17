from opendaq import CoreType
from .base import PropertyView
from .string_property import StringPropertyView
from .int_property import IntPropertyView
from .float_property import FloatPropertyView
from .bool_property import BoolPropertyView
from .object_property import ObjectPropertyView
from .dict_property import DictPropertyView
from .list_property import ListPropertyView
from .func_property import FuncPropertyView
from .struct_property import StructPropertyView


def make_property_view(prop, context=None) -> PropertyView:
    t = prop.value_type
    if t == CoreType.ctObject:
        return ObjectPropertyView(prop, context)
    if t == CoreType.ctDict:
        return DictPropertyView(prop, context)
    if t == CoreType.ctList:
        return ListPropertyView(prop, context)
    if t == CoreType.ctFunc:
        return FuncPropertyView(prop, context)
    if t == CoreType.ctStruct:
        return StructPropertyView(prop, context)
    if t == CoreType.ctBool:
        return BoolPropertyView(prop, context)
    if t == CoreType.ctInt:
        return IntPropertyView(prop, context)
    if t == CoreType.ctFloat:
        return FloatPropertyView(prop, context)
    if t == CoreType.ctString:
        return StringPropertyView(prop, context)

    return PropertyView(prop, context)
