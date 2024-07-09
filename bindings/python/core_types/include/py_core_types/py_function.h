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
#include "py_converter.h"

#include <coretypes/coretypes.h>
#include <coretypes/validation.h>

template <class F>
class PyFunctionImpl : public PyObjectImpl<pybind11::object, daq::IFunction, daq::ICoreType>
{
public:
    explicit PyFunctionImpl(const pybind11::object& pyObject);
    explicit PyFunctionImpl(pybind11::object&& pyObject);

protected:
    daq::ErrCode INTERFACE_FUNC call(daq::IBaseObject* params, daq::IBaseObject** result) override;
    // ICoreType
    daq::ErrCode INTERFACE_FUNC getCoreType(daq::CoreType* coreType) override;
};

template <class F>
daq::IFunction* PyFunction_Create(const pybind11::object& obj);

template <class F>
daq::IFunction* PyFunction_Create(pybind11::object&& obj);

template <class F>
PyFunctionImpl<F>::PyFunctionImpl(const pybind11::object& pyObject)
    : PyObjectImpl(pyObject)
{
}

template <class F>
PyFunctionImpl<F>::PyFunctionImpl(pybind11::object&& pyObject)
    : PyObjectImpl(std::move(pyObject))
{
}

template <class F>
daq::ErrCode PyFunctionImpl<F>::call(daq::IBaseObject* params, daq::IBaseObject** result)
{
    OPENDAQ_PARAM_NOT_NULL(result);
    
    if (params == nullptr)
    {
        auto r = pyObject();
        auto obj = pyObjectToBaseObject(r);
        *result = obj.detach();
    }
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

            auto r = pyObject(*args);
            auto obj = pyObjectToBaseObject(r);
            *result = obj.detach();
        }
        else
        {
            auto po = F::ToPyObject(params);
            auto r = pyObject(po);
            auto obj = pyObjectToBaseObject(r);
            *result = obj.detach();

        }
    }
    return OPENDAQ_SUCCESS;
}

template <class F>
daq::ErrCode PyFunctionImpl<F>::getCoreType(daq::CoreType* coreType)
{
    if (coreType == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *coreType = daq::ctFunc;
    return OPENDAQ_SUCCESS;
}

template <class F>
daq::IFunction* PyFunction_Create(const pybind11::object& obj)
{
    auto procObj = new PyFunctionImpl<F>(obj);
    procObj->addRef();
    return procObj;
}

template <class F>
daq::IFunction* PyFunction_Create(pybind11::object&& obj)
{
    auto procObj = new PyFunctionImpl<F>(std::move(obj));
    procObj->addRef();
    return procObj;
}
