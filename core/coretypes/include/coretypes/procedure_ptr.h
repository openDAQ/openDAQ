/*
 * Copyright 2022-2024 openDAQ d.o.o.
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
#include <coretypes/objectptr.h>
#include <utility>

BEGIN_NAMESPACE_OPENDAQ

class ProcedurePtr;

template <>
struct InterfaceToSmartPtr<daq::IProcedure>
{
    using SmartPtr = daq::ProcedurePtr;
};

ProcedurePtr CustomProcedure(ProcCall proc);

template <typename TFunctor>
ProcedurePtr Procedure(TFunctor value);

template <typename TFunctor>
ProcedurePtr Procedure(TFunctor* value);

/*!
 * @addtogroup types_procedure
 * @{
 */


/*!
 * @brief Holds a callback function without return value.
 *
 * Represents a callable object without return value. The openDAQ SDK uses this object when
 * it needs to make a call back to the client.
 *
 * Available factories:
 * @code
 * // Creates a new Function object.
 * template <typename TFunctor>
 * ProcedurePtr Procedure(TFunctor value)
 * @endcode
 *
 * Example:
 * @code
 * auto procObj = Procedure([](Int a) { std::cout << a; } );
 * procObj(2);
 * @endcode
 */
class ProcedurePtr : public ObjectPtr<IProcedure>
{
public:
    using ObjectPtr<IProcedure>::ObjectPtr;
    using Delegate = ErrCode (*)(IBaseObject*);

    ProcedurePtr() = default;

    ProcedurePtr(std::nullptr_t null) // NOLINT(google-explicit-constructor)
        : ObjectPtr<daq::IProcedure>(null)
    {
    }

    ProcedurePtr(const IProcedure*& raw) // NOLINT(google-explicit-constructor)
        : ObjectPtr<IProcedure>(raw)
    {
    }

    ProcedurePtr(IProcedure*&& raw) // NOLINT(google-explicit-constructor)
        : ObjectPtr<IProcedure>(std::forward<decltype(raw)>(raw))
    {
    }

    template <typename TFunctor>
    ProcedurePtr(TFunctor dispatch) // NOLINT(google-explicit-constructor)
        : ObjectPtr<IProcedure>(Procedure(std::forward<decltype(dispatch)>(dispatch)))
    {
    }

    ProcedurePtr(Delegate dispatch) // NOLINT(google-explicit-constructor)
        : ObjectPtr<IProcedure>(CustomProcedure(std::forward<decltype(dispatch)>(dispatch)))
    {
    }

    ProcedurePtr(const ObjectPtr<IProcedure>& ptr) // NOLINT(google-explicit-constructor)
        : ObjectPtr<IProcedure>(ptr)
    {
    }

    ProcedurePtr(ObjectPtr<IProcedure>&& ptr) // NOLINT(google-explicit-constructor)
        : ObjectPtr<IProcedure>(std::move(ptr))
    {
    }

    template <typename... Params>
    void operator()(Params... params) const
    {
        this->execute(std::forward<decltype(params)>(params)...);
    }

    void operator()() const
    {
        dispatch();
    }
};

/*!@}*/

END_NAMESPACE_OPENDAQ
