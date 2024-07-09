/*
 * Copyright 2022-2024 openDAQ d. o. o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
#pragma once

#include <pybind11/pybind11.h>

#include "py_base_object.h"

DECLARE_OPENDAQ_INTERFACE(IPyList, daq::IBaseObject)
{
    virtual daq::ErrCode INTERFACE_FUNC getPyObject(pybind11::list & pyObj) = 0;
};

template <class F>
class PyListImpl : public PyObjectImpl<pybind11::list, IPyList, daq::IList, daq::IIterable, daq::ICoreType>
{
public:
    explicit PyListImpl(const pybind11::list& obj);
    explicit PyListImpl(pybind11::list&& obj);

protected:
    daq::ErrCode INTERFACE_FUNC getItemAt(daq::SizeT index, IBaseObject** item) override;
    daq::ErrCode INTERFACE_FUNC getCount(daq::SizeT* size) override;

    daq::ErrCode INTERFACE_FUNC setItemAt(daq::SizeT index, IBaseObject* obj) override;

    daq::ErrCode INTERFACE_FUNC pushBack(daq::IBaseObject* obj) override;
    daq::ErrCode INTERFACE_FUNC pushFront(daq::IBaseObject* obj) override;

    daq::ErrCode INTERFACE_FUNC moveBack(daq::IBaseObject* obj) override;
    daq::ErrCode INTERFACE_FUNC moveFront(daq::IBaseObject* obj) override;

    daq::ErrCode INTERFACE_FUNC popBack(daq::IBaseObject** obj) override;
    daq::ErrCode INTERFACE_FUNC popFront(daq::IBaseObject** obj) override;

    daq::ErrCode INTERFACE_FUNC insertAt(daq::SizeT index, IBaseObject* obj) override;
    daq::ErrCode INTERFACE_FUNC removeAt(daq::SizeT index, IBaseObject** obj) override;

    daq::ErrCode INTERFACE_FUNC deleteAt(daq::SizeT index) override;

    daq::ErrCode INTERFACE_FUNC clear() override;

    // IIterable, IList
    daq::ErrCode INTERFACE_FUNC createStartIterator(daq::IIterator** iterator) override;
    daq::ErrCode INTERFACE_FUNC createEndIterator(daq::IIterator** iterator) override;

    // ICoreType
    daq::ErrCode INTERFACE_FUNC getCoreType(daq::CoreType* coreType) override;
};

template <class F>
daq::ErrCode PyListImpl<F>::getCoreType(daq::CoreType* coreType)
{
    if (coreType == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *coreType = daq::ctList;
    return OPENDAQ_SUCCESS;
}

template <class F>
PyListImpl<F>::PyListImpl(const pybind11::list& obj)
    : PyObjectImpl(obj)
{
}

template <class F>
PyListImpl<F>::PyListImpl(pybind11::list&& obj)
    : PyObjectImpl(std::move(obj))
{
}

template <class F>
daq::ErrCode PyListImpl<F>::getItemAt(daq::SizeT index, daq::IBaseObject** item)
{
    if (item == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    if (index >= pyObject.size())
        return OPENDAQ_ERR_OUTOFRANGE;

    pybind11::object obj = pyObject[index];

    auto daqObj = F::ToBaseObject(obj);

    *item = daqObj.detach();

    return OPENDAQ_SUCCESS;
}

template <class F>
daq::ErrCode PyListImpl<F>::getCount(daq::SizeT* size)
{
    if (size == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *size = pyObject.size();
    return OPENDAQ_SUCCESS;
}

template <class F>
daq::ErrCode PyListImpl<F>::setItemAt(daq::SizeT index, daq::IBaseObject* obj)
{
    auto po = F::ToPyObject(obj);
    pyObject[index] = po;
    return OPENDAQ_SUCCESS;
}

template <class F>
daq::ErrCode PyListImpl<F>::pushBack(daq::IBaseObject* obj)
{
    auto po = F::ToPyObject(obj);
    pyObject.append(std::move(po));
    return OPENDAQ_SUCCESS;
}

template <class F>
inline daq::ErrCode PyListImpl<F>::pushFront(daq::IBaseObject* obj)
{
    auto po = F::ToPyObject(obj);
    pyObject.insert(0, std::move(po));
    return OPENDAQ_SUCCESS;
}

template <class F>
daq::ErrCode PyListImpl<F>::moveBack(daq::IBaseObject* obj)
{
    return pushBack(obj);
}

template <class F>
daq::ErrCode PyListImpl<F>::moveFront(daq::IBaseObject* obj)
{
    return pushFront(obj);
}

template <class F>
daq::ErrCode PyListImpl<F>::popBack(daq::IBaseObject** obj)
{
    if (obj == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    pybind11::object pyobj = pyObject[pyObject.size() - 1];

    auto daqObj = F::ToBaseObject(pyobj);

    *obj = daqObj.detach();

    return OPENDAQ_SUCCESS;
}

template <class F>
daq::ErrCode PyListImpl<F>::popFront(daq::IBaseObject** obj)
{
    if (obj == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    pybind11::object pyobj = pyObject[0];

    auto daqObj = F::ToBaseObject(pyobj);

    *obj = daqObj.detach();

    return OPENDAQ_SUCCESS;
}

template <class F>
daq::ErrCode PyListImpl<F>::insertAt(daq::SizeT index, daq::IBaseObject* obj)
{
    auto po = F::ToPyObject(obj);
    pyObject.insert(index, std::move(po));
    return OPENDAQ_SUCCESS;
}

template <class F>
daq::ErrCode PyListImpl<F>::removeAt(daq::SizeT index, daq::IBaseObject** obj)
{
    pybind11::object po = pyObject.attr("pop")(index);
    auto daqObj = F::ToBaseObject(po);

    *obj = daqObj.detach();
    return OPENDAQ_SUCCESS;
}

template <class F>
daq::ErrCode PyListImpl<F>::deleteAt(daq::SizeT index)
{
    pyObject.attr("pop")(index);
    return OPENDAQ_SUCCESS;
}

template <class F>
daq::ErrCode PyListImpl<F>::clear()
{
    auto size = pyObject.size();

    PyObject* po = pyObject.ptr();
    PySequence_DelSlice(po, 0, size - 1);
    return OPENDAQ_SUCCESS;
}

template <class F>
daq::ErrCode PyListImpl<F>::createStartIterator(daq::IIterator** /* iterator */)
{
    return OPENDAQ_ERR_NOTIMPLEMENTED;
}

template <class F>
daq::ErrCode PyListImpl<F>::createEndIterator(daq::IIterator** /* iterator */)
{
    return OPENDAQ_ERR_NOTIMPLEMENTED;
}

template <class F>
daq::ObjectPtr<daq::IBaseObject> wrapPyList(const pybind11::list& pyObj)
{
    IPyList* obj = new PyListImpl<F>(pyObj);
    return daq::ObjectPtr<daq::IBaseObject>(obj);
}
