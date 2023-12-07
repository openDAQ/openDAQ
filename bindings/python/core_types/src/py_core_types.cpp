#include "py_core_types/py_core_types.h"

void wrapDaqComponentCoreTypes(pybind11::module_ m)
{
    py::enum_<daq::CoreType>(m, "CoreType")
        .value("ctBool", daq::CoreType::ctBool)
        .value("ctInt", daq::CoreType::ctInt)
        .value("ctFloat", daq::CoreType::ctFloat)
        .value("ctString", daq::CoreType::ctString)
        .value("ctList", daq::CoreType::ctList)
        .value("ctDict", daq::CoreType::ctDict)
        .value("ctRatio", daq::CoreType::ctRatio)
        .value("ctProc", daq::CoreType::ctProc)
        .value("ctObject", daq::CoreType::ctObject)
        .value("ctBinaryData", daq::CoreType::ctBinaryData)
        .value("ctFunc", daq::CoreType::ctFunc)
        .value("ctComplexNumber", daq::CoreType::ctComplexNumber)
        .value("ctStruct", daq::CoreType::ctStruct)
        .value("ctUndefined", daq::CoreType::ctUndefined);

    declareAndDefineIBaseObject(m);

    auto classIInteger = declareIInteger(m);
    auto classIFloat = declareIFloat(m);
    auto classIBoolean = declareIBoolean(m);
    auto classIString = declareIString(m);
    auto classIRatio = declareIRatio(m);
    auto classIComplexNumber = declareIComplexNumber(m);
    auto classINumber = declareINumber(m);

    auto classIIterable = declareIIterable(m);
    auto classIIterator = declareIIterator(m);
    auto classIList = declareIList(m);
    auto classIDict = declareIDict(m);
    auto classIProcedure = declareIProcedure(m);
    auto classIFunction = declareIFunction(m);

    auto classIEventArgs = declareIEventArgs(m);
    auto classIType = declareIType(m);
    auto classISimpleType = declareISimpleType(m);
    auto classITypeManager = declareITypeManager(m);
    auto classIStructType = declareIStructType(m);
    auto classIStruct = declareIStruct(m);
    auto classIStructBuilder = declareIStructBuilder(m);

    defineIInteger(m, classIInteger);
    defineIFloat(m, classIFloat);
    defineIBoolean(m, classIBoolean);
    defineIString(m, classIString);
    defineIRatio(m, classIRatio);
    defineIComplexNumber(m, classIComplexNumber);
    defineINumber(m, classINumber);

    defineIIterable(m, classIIterable);
    defineIIterator(m, classIIterator);
    defineIList(m, classIList);
    defineIDict(m, classIDict);
    defineIProcedure(m, classIProcedure);
    defineIFunction(m, classIFunction);

    defineIEventArgs(m, classIEventArgs);
    defineIType(m, classIType);
    defineISimpleType(m, classISimpleType);
    defineITypeManager(m, classITypeManager);
    defineIStructType(m, classIStructType);
    defineIStruct(m, classIStruct);
    defineIStructBuilder(m, classIStructBuilder);
}
