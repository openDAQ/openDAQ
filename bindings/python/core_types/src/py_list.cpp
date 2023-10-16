#include "py_core_types/py_core_types.h"

PyDaqIntf<daq::IList> declareIList(pybind11::module_ m)
{
    return wrapInterface<daq::IList>(m, "IList");
}

void defineIList(pybind11::module_ m, PyDaqIntf<daq::IList> cls)
{
    cls.doc() = "Represents a heterogeneous collection of objects that can be individually accessed by index.";

    m.def("List", &daq::List_Create);

    cls.def("__getitem__",
        [](daq::IList* list, ptrdiff_t index)
        {
            const auto listPtr = daq::ListPtr<daq::IBaseObject>::Borrow(list);
            size_t ind;
            if (index >= 0)
                ind = static_cast<size_t>(index);
            else
                ind = listPtr.getCount() + index;
            const auto item = listPtr.getItemAt(ind);
            return baseObjectToPyObject(item, listPtr.getElementInterfaceId());
        });
    cls.def("__getitem__",
        [](daq::IList* list, const py::slice& slice)
        {
            auto listPtr = daq::ListPtr<daq::IBaseObject>::Borrow(list);
            size_t start = 0, stop = 0, step = 0, sliceLength = 0;
            if (!slice.compute(listPtr.getCount(), &start, &stop, &step, &sliceLength))
                throw py::error_already_set();

            daq::ListPtr<daq::IBaseObject> newList(daq::ListWithElementType_Create(listPtr.getElementInterfaceId()));
            for (size_t i = start; i < stop; i += step)
                newList.pushBack(listPtr[i]);

            return newList.detach();
        });
    cls.def("__setitem__",
        [](daq::IList* list, size_t index, const py::object& pyObject)
        {
            auto listPtr = daq::ListPtr<daq::IBaseObject>::Borrow(list);
            listPtr.setItemAt(index, pyObjectToBaseObject(pyObject));
        });
    cls.def("__len__",
        [](daq::IList* list)
        {
            const auto listPtr = daq::ListPtr<daq::IBaseObject>::Borrow(list);
            return listPtr.getCount();
        });
    cls.def("__repr__",
        [](daq::IList* list)
        {
            std::ostringstream str;
            str << "daq_[";
            const auto listPtr = daq::ListPtr<daq::IBaseObject>::Borrow(list);
            for (size_t i = 0; i < listPtr.getCount(); i++)
            {
                auto item = listPtr.getItemAt(i);
                std::string s = daq::getString(item.getObject());
                if (i > 0)
                    str << ", ";
                str << s;
            }
            str << "]";
            return str.str();
        });
    cls.def("__str__",
        [](daq::IList* list)
        {
            std::ostringstream str;
            str << "[";
            const auto listPtr = daq::ListPtr<daq::IBaseObject>::Borrow(list);
            for (size_t i = 0; i < listPtr.getCount(); i++)
            {
                auto item = listPtr.getItemAt(i);
                std::string s = daq::getString(item.getObject());
                if (i > 0)
                    str << ", ";
                str << s;
            }
            str << "]";
            return str.str();
        });
    cls.def("__iter__",
        [](daq::IList* list)
        {
            const auto iterablePtr = daq::ObjectPtr<daq::IIterable>::Borrow(list);
            daq::ObjectPtr<daq::IIterator> it;
            daq::checkErrorInfo(iterablePtr->createStartIterator(&it));

            auto pythonIterator = baseObjectToPyObjectUsingType<daq::IIterator>(it);

            return pythonIterator;
        });

    cls.def("pushBack",  // this should be .append()
        [](daq::IList* list, const py::object& pyObject)
        {
            auto listPtr = daq::ListPtr<daq::IBaseObject>::Borrow(list);
            listPtr.pushBack(pyObjectToBaseObject(pyObject));
        });
    cls.def("pushFront",
        [](daq::IList* list, const py::object& pyObject)
        {
            auto listPtr = daq::ListPtr<daq::IBaseObject>::Borrow(list);
            listPtr.pushFront(pyObjectToBaseObject(pyObject));
        });
    cls.def("popFront",
        [](daq::IList* list)
        {
            auto listPtr = daq::ListPtr<daq::IBaseObject>::Borrow(list);
            return baseObjectToPyObject(listPtr.popFront(), listPtr.getElementInterfaceId());
        });
    cls.def("popBack",
        [](daq::IList* list)
        {
            auto listPtr = daq::ListPtr<daq::IBaseObject>::Borrow(list);
            return baseObjectToPyObject(listPtr.popBack(), listPtr.getElementInterfaceId());
        });
    cls.def("append",
        [](daq::IList* list, const py::object& pyObject)
        {
            auto listPtr = daq::ListPtr<daq::IBaseObject>::Borrow(list);
            listPtr.pushBack(pyObjectToBaseObject(pyObject));
        });
    cls.def("clear",
        [](daq::IList* object)
        {
            return daq::ListPtr<daq::IBaseObject>::Borrow(object).clear();
        },
        "Removes all elements from the list.");
}
