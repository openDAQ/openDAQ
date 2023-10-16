#include "py_core_types/py_core_types.h"

PyDaqIntf<daq::IIterator> declareIIterator(pybind11::module_ m)
{
    return wrapInterface<daq::IIterator>(m, "IIterator");
}

void defineIIterator(pybind11::module_ m, PyDaqIntf<daq::IIterator> cls)
{
    cls.doc() = "Interface to iterate through items of a container object.";

    cls.def("__next__",
        [](daq::IIterator* it)
        {
            const daq::ErrCode errCode = it->moveNext();
            if (errCode == OPENDAQ_NO_MORE_ITEMS)
                throw pybind11::stop_iteration();
            daq::checkErrorInfo(errCode);

            daq::ObjectPtr<daq::IBaseObject> objectPtr;
            daq::checkErrorInfo(it->getCurrent(&objectPtr));

            daq::IntfID interfaceId{};
            daq::checkErrorInfo(daq::ObjectPtr<daq::IListElementType>::Borrow(it)->getElementInterfaceId(&interfaceId));

            return baseObjectToPyObject(objectPtr, interfaceId);
        });
}
