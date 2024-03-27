/*
 * Copyright 2022-2024 Blueberry d.o.o.
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
#include <coretypes/procedure_impl.h>
#include <coretypes/procedure_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

inline ProcedurePtr CustomProcedure(ProcCall proc)
{
    ProcedurePtr obj(Procedure_Create(proc));
    return obj;
}

template <typename TFunctor>
ProcedurePtr Procedure(TFunctor value)
{
    ProcedurePtr obj(ProcedureWrapper_Create<TFunctor>(std::move(value)));
    return obj;
}

template <typename TFunctor>
ProcedurePtr Procedure(TFunctor* value)
{
    ProcedurePtr obj(ProcedureWrapper_Create<TFunctor>(value));
    return obj;
}

template <typename... Params>
ErrCode wrapHandler(ProcedurePtr handler, Params... params)
{
    try
    {
        (handler)(params...);
        return OPENDAQ_SUCCESS;
    }
    catch (const DaqException& e)
    {
        return errorFromException(e);
    }
    catch (const std::exception& e)
    {
        return makeErrorInfo(OPENDAQ_ERR_GENERALERROR, e.what(), nullptr);
    }
    catch (...)
    {
        return OPENDAQ_ERR_GENERALERROR;
    }
}

END_NAMESPACE_OPENDAQ
