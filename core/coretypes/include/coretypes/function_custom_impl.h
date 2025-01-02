/*
 * Copyright 2022-2025 openDAQ d.o.o.
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
#include <coretypes/function.h>
#include <coretypes/coretype.h>
#include <coretypes/intfs.h>
#include <utility>

BEGIN_NAMESPACE_OPENDAQ

template <typename TFunctor>
class CustomFunctionImpl : public ImplementationOf<IFunction, ICoreType>
{
public:
    CustomFunctionImpl(TFunctor value); // NOLINT(google-explicit-constructor)

    ErrCode INTERFACE_FUNC getCoreType(CoreType* coreType) override;
    ErrCode INTERFACE_FUNC call(IBaseObject* params, IBaseObject** result) override;

    ErrCode INTERFACE_FUNC toString(CharPtr* str) override;

private:
    TFunctor functor;
};

template <typename TFunctor>
CustomFunctionImpl<TFunctor>::CustomFunctionImpl(TFunctor value)
    : functor(std::move(value))
{
}

template <typename TFunctor>
ErrCode CustomFunctionImpl<TFunctor>::call(IBaseObject* params, IBaseObject** result)
{
    if (!result)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    if (!functor)
        return OPENDAQ_ERR_NOTASSIGNED;

    try
    {
        const auto err = functor(params, result);
        return err;
    }
    catch (const std::exception& e)
    {
        return errorFromException(e, nullptr,  OPENDAQ_ERR_CALLFAILED);
    }
    catch (...)
    {
        return OPENDAQ_ERR_CALLFAILED;
    }
}

template <typename TFunctor>
ErrCode CustomFunctionImpl<TFunctor>::getCoreType(CoreType* coreType)
{
    if (!coreType)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *coreType = ctFunc;
    return OPENDAQ_SUCCESS;
}

template <typename TFunctor>
ErrCode CustomFunctionImpl<TFunctor>::toString(CharPtr* str)
{
    if (str == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    return daqDuplicateCharPtr("Function", str);
}

END_NAMESPACE_OPENDAQ
