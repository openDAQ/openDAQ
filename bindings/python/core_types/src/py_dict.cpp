#include "py_core_types/py_core_types.h"

PyDaqIntf<daq::IDict> declareIDict(pybind11::module_ m)
{
    return wrapInterface<daq::IDict>(m, "IDict");
}

void defineIDict(pybind11::module_ m, PyDaqIntf<daq::IDict> cls)
{
    cls.doc() = "Represents a heterogeneous dictionary of objects.";

    m.def("Dict", &daq::Dict_Create);

    cls.def("__getitem__",
        [](daq::IDict* dict, const py::object& key)
        {
            const auto dictPtr = daq::DictPtr<daq::IBaseObject, daq::IBaseObject>::Borrow(dict);
            auto keyObject = pyObjectToBaseObject(key);
            auto item = dictPtr.get(keyObject);
            return baseObjectToPyObject(item, dictPtr.getValueInterfaceId());
        });
    cls.def("__setitem__",
        [](daq::IDict* dict, const py::object& key, const py::object& value)
        {
            auto dictPtr = daq::DictPtr<daq::IBaseObject, daq::IBaseObject>::Borrow(dict);
            auto keyObject = pyObjectToBaseObject(key);
            auto valueObject = pyObjectToBaseObject(value);
            dictPtr.set(keyObject, valueObject);
        });
    cls.def("__delitem__",
        [](daq::IDict* dict, const py::object& key)
        {
            auto dictPtr = daq::DictPtr<daq::IBaseObject, daq::IBaseObject>::Borrow(dict);
            auto keyObject = pyObjectToBaseObject(key);
            dictPtr.remove(keyObject);
        });
    cls.def("pop",
        [](daq::IDict* dict, const py::object& key)
        {
            auto dictPtr = daq::DictPtr<daq::IBaseObject, daq::IBaseObject>::Borrow(dict);
            auto keyObject = pyObjectToBaseObject(key);
            return dictPtr.remove(keyObject).detach();
        });
    cls.def("keys",
        [](daq::IDict* dict)
        {
            const auto dictPtr = daq::DictPtr<daq::IBaseObject, daq::IBaseObject>::Borrow(dict);
            auto keysList = dictPtr.getKeys();
            return keysList.detach();
        });
    cls.def("values",
        [](daq::IDict* dict)
        {
            const auto dictPtr = daq::DictPtr<daq::IBaseObject, daq::IBaseObject>::Borrow(dict);
            auto valuesList = dictPtr.getValues();
            return valuesList.detach();
        });
    cls.def("items",
        [](daq::IDict* dict)
        {
            const auto dictPtr = daq::DictPtr<daq::IBaseObject, daq::IBaseObject>::Borrow(dict);

            py::list list;

            for (const auto& elem: dictPtr)
            {
                auto item = py::make_tuple(baseObjectToPyObject(elem.first, dictPtr.getKeyInterfaceId()),
                                           baseObjectToPyObject(elem.second, dictPtr.getValueInterfaceId()));
                list.append(item);
            }

            return list;
        });
    cls.def("clear",
        [](daq::IDict* dict)
        {
            auto dictPtr = daq::DictPtr<daq::IBaseObject, daq::IBaseObject>::Borrow(dict);
            dictPtr.clear();
        });
    cls.def("__len__",
        [](daq::IDict* dict)
        {
            const auto dictPtr = daq::DictPtr<daq::IBaseObject, daq::IBaseObject>::Borrow(dict);
            return dictPtr.getCount();
        });
    cls.def("__iter__",
        [](daq::IDict* dict)
        {
            const auto dictPtr = daq::DictPtr<daq::IBaseObject, daq::IBaseObject>::Borrow(dict);
            const auto iterablePtr = dictPtr.getKeys();

            daq::ObjectPtr<daq::IIterator> it;
            daq::checkErrorInfo(iterablePtr->createStartIterator(&it));

            auto pythonIterator = baseObjectToPyObjectUsingType<daq::IIterator>(it);

            return pythonIterator;
        });
}
