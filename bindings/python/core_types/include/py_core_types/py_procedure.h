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

#include "py_base_object.h"

#include <coretypes/coretypes.h>

template <class F>
class PyProcedureImpl : public PyObjectImpl<pybind11::object, daq::IProcedure, daq::ICoreType>
{
public:
    explicit PyProcedureImpl(const pybind11::object& pyObject);
    explicit PyProcedureImpl(pybind11::object&& pyObject);

protected:
    daq::ErrCode INTERFACE_FUNC dispatch(daq::IBaseObject* params) override;
    // ICoreType
    daq::ErrCode INTERFACE_FUNC getCoreType(daq::CoreType* coreType) override;
};

template <class F>
daq::IProcedure* PyProcedure_Create(const pybind11::object& obj);
template <class F>
daq::IProcedure* PyProcedure_Create(pybind11::object&& obj);

template <class F>
PyProcedureImpl<F>::PyProcedureImpl(const pybind11::object& pyObject)
    : PyObjectImpl(pyObject)
{
}

template <class F>
PyProcedureImpl<F>::PyProcedureImpl(pybind11::object&& pyObject)
    : PyObjectImpl(std::move(pyObject))
{
}

template <class F>
daq::ErrCode PyProcedureImpl<F>::dispatch(daq::IBaseObject* params)
{
    if (params == nullptr)
        pyObject();
    else
    {
        const auto paramsPtr = daq::BaseObjectPtr::Borrow(params);
        auto list = paramsPtr.asPtrOrNull<daq::IList>(true);
        if (list.assigned())
        {
            pybind11::tuple args(list.getCount());

            for (size_t i = 0; i < list.getCount(); i++)
            {
                args[i] = F::ToPyObject(list[i]);
            }

            auto result = PyObject_CallObject(pyObject.ptr(), args.ptr());
            if (!result)
                throw pybind11::error_already_set();
        }
        else
        {
            auto po = F::ToPyObject(params);
            pyObject(po);
        }
    }
    return OPENDAQ_SUCCESS;
}

template <class F>
daq::ErrCode PyProcedureImpl<F>::getCoreType(daq::CoreType* coreType)
{
    if (coreType == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *coreType = daq::ctProc;
    return OPENDAQ_SUCCESS;
}

template <class F>
daq::IProcedure* PyProcedure_Create(const pybind11::object& obj)
{
    auto procObj = new PyProcedureImpl<F>(obj);
    procObj->addRef();
    return procObj;
}

template <class F>
daq::IProcedure* PyProcedure_Create(pybind11::object&& obj)
{
    auto procObj = new PyProcedureImpl<F>(std::move(obj));
    procObj->addRef();
    return procObj;
}
