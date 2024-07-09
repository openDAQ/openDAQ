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
#include <coretypes/procedure.h>
#include <coretypes/coretype.h>
#include <coretypes/intfs.h>
#include <type_traits>
#include <utility>

BEGIN_NAMESPACE_OPENDAQ

template <typename F>
class CustomProcedureImpl : public ImplementationOf<IProcedure, ICoreType>
{
public:
    explicit CustomProcedureImpl(F proc);
    ErrCode INTERFACE_FUNC getCoreType(CoreType* coreType) override;
    ErrCode INTERFACE_FUNC toString(CharPtr* str) override;
    ErrCode INTERFACE_FUNC dispatch(IBaseObject* params) override;

private:
    F proc;
};

template <typename F>
CustomProcedureImpl<F>::CustomProcedureImpl(F proc)
    : proc(std::move(proc))
{
}

template <typename F>
ErrCode CustomProcedureImpl<F>::getCoreType(CoreType* coreType)
{
    if (!coreType)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *coreType = ctProc;
    return OPENDAQ_SUCCESS;
}

template <typename F>
ErrCode CustomProcedureImpl<F>::toString(CharPtr* str)
{
    if (str == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    return daqDuplicateCharPtr("Procedure", str);
}

template <typename F>
ErrCode CustomProcedureImpl<F>::dispatch(IBaseObject* params)
{
    if constexpr (!std::is_same_v<F, std::nullptr_t>)
    {
        if (!this->proc)
        {
            return OPENDAQ_ERR_NOTASSIGNED;
        }

        try
        {
            return this->proc(params);
        }
        catch (const DaqException& e)
        {
            return errorFromException(e);
        }
        catch (...)
        {
            return OPENDAQ_ERR_CALLFAILED;
        }
    }
    else
    {
        return OPENDAQ_ERR_NOTASSIGNED;
    }
}

END_NAMESPACE_OPENDAQ
