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


def make_property_view(prop) -> PropertyView:
    t = prop.value_type
    if t == CoreType.ctObject:
        return ObjectPropertyView(prop)
    if t == CoreType.ctDict:
        return DictPropertyView(prop)
    if t == CoreType.ctList:
        return ListPropertyView(prop)
    if t == CoreType.ctFunc:
        return FuncPropertyView(prop)

    if t == CoreType.ctBool:
        return BoolPropertyView(prop)
    if t == CoreType.ctInt:
        return IntPropertyView(prop)
    if t == CoreType.ctFloat:
        return FloatPropertyView(prop)
    if t == CoreType.ctString:
        return StringPropertyView(prop)

    return PropertyView(prop)
