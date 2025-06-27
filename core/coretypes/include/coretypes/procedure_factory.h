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
    if (!handler.assigned())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ARGUMENT_NULL);

    const ErrCode errCode = daqTry([&]()
    {
        (handler)(params...);
    });
    OPENDAQ_RETURN_IF_FAILED(errCode, "Failed to wrap handler call for procedure");
    return errCode;
}

END_NAMESPACE_OPENDAQ
