#include "py_core_objects/py_core_objects.h"

#include "py_core_types/py_converter.h"

void wrapDaqComponentCoreObjects(pybind11::module_ m)
{
    auto classIArgumentInfo = declareIArgumentInfo(m);
    auto classICallableInfo = declareICallableInfo(m);
    auto classICoercer = declareICoercer(m);
    auto classIEvalValue = declareIEvalValue(m);
    auto classIOwnable = declareIOwnable(m);
    auto classIProperty = declareIProperty(m);
    auto classIPropertyBuilder = declareIPropertyBuilder(m);
    auto classIPropertyObject = declareIPropertyObject(m);
    auto classIPropertyObjectClass = declareIPropertyObjectClass(m);
    auto classIPropertyObjectClassBuilder = declareIPropertyObjectClassBuilder(m);
    auto classIPropertyObjectProtected = declareIPropertyObjectProtected(m);
    auto classIPropertyValueEventArgs = declareIPropertyValueEventArgs(m);
    auto classIValidator = declareIValidator(m);
    auto classIUnit = declareIUnit(m);
    auto classIUnitBuilder = declareIUnitBuilder(m);
    auto classIComponentType = declareIComponentType(m);

    defineIArgumentInfo(m, classIArgumentInfo);
    defineICallableInfo(m, classICallableInfo);
    defineICoercer(m, classICoercer);
    defineIEvalValue(m, classIEvalValue);
    defineIOwnable(m, classIOwnable);
    defineIProperty(m, classIProperty);
    defineIPropertyBuilder(m, classIPropertyBuilder);
    defineIPropertyObject(m, classIPropertyObject);
    defineIPropertyObjectClass(m, classIPropertyObjectClass);
    defineIPropertyObjectClassBuilder(m, classIPropertyObjectClassBuilder);
    defineIPropertyObjectProtected(m, classIPropertyObjectProtected);
    defineIPropertyValueEventArgs(m, classIPropertyValueEventArgs);
    defineIValidator(m, classIValidator);
    defineIUnit(m, classIUnit);
    defineIUnitBuilder(m, classIUnitBuilder);
    defineIComponentType(m, classIComponentType);
}
