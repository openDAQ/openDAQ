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

#include <coretypes/coretypes.h>

#include <pybind11/pybind11.h>

DECLARE_OPENDAQ_INTERFACE(IPyObject, daq::IBaseObject)
{
    virtual daq::ErrCode INTERFACE_FUNC getPyObject(pybind11::object & pyObj) = 0;
};

template <class PyT, class... Interfaces>
class PyObjectImpl : public daq::ImplementationOf<IPyObject, Interfaces...>
{
public:
    explicit PyObjectImpl(const PyT& pyObject);
    explicit PyObjectImpl(PyT&& pyObject);

    daq::ErrCode INTERFACE_FUNC getPyObject(PyT& pyObj) override;
    daq::ErrCode INTERFACE_FUNC toString(daq::CharPtr* str) override;

protected:
    PyT pyObject;
};

template <class PyT, class... Interfaces>
PyObjectImpl<PyT, Interfaces...>::PyObjectImpl(const PyT& pyObject)
    : pyObject(pyObject)
{
}

template <class PyT, class... Interfaces>
PyObjectImpl<PyT, Interfaces...>::PyObjectImpl(PyT&& pyObject)
    : pyObject(std::move(pyObject))
{
}

template <class PyT, class... Interfaces>
daq::ErrCode PyObjectImpl<PyT, Interfaces...>::getPyObject(PyT& pyObj)
{
    pyObj = pyObject;
    return OPENDAQ_SUCCESS;
}

template <class PyT, class... Interfaces>
inline daq::ErrCode PyObjectImpl<PyT, Interfaces...>::toString(daq::CharPtr* str)
{
    if (str == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    std::string str1 = pybind11::repr(pyObject);
    const daq::ErrCode err = daqDuplicateCharPtr(str1.c_str(), str);
    return err;
}

template <class T>
daq::ObjectPtr<daq::IBaseObject> wrapPyObject(T&& pyObj)
{
    IPyObject* obj = new PyObjectImpl<pybind11::object>(std::forward<T>(pyObj));
    return {obj};
}
