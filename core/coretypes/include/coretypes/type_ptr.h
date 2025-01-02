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
#include <coretypes/type.h>
#include <coretypes/objectptr.h>
#include <coretypes/string_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

template <typename InterfaceType = daq::IType>
class GenericTypePtr;

using TypePtr = GenericTypePtr<>;

template <>
struct InterfaceToSmartPtr<IType>
{
    using SmartPtr = daq::GenericTypePtr<daq::IType>;
};

/*!
 * @ingroup types_types
 * @defgroup types_types_simple_type Type
 * @{
 */

/*!
 * @brief The base object type that is inherited by all Types (eg. Struct type, Simple type, Property object class)
 * in openDAQ.
 *
 * Types are used for the construction of objects that are require validation/have pre-defined fields such as
 * Structs and Property objects. Types should be inserted into the Type manager to be used by different parts
 * of the SDK.
 */
template <typename InterfaceType>
class GenericTypePtr : public ObjectPtr<InterfaceType>
{
public:
    using ObjectPtr<InterfaceType>::ObjectPtr;
    
    GenericTypePtr()
        : daq::ObjectPtr<InterfaceType>()
    {
    }

    GenericTypePtr(ObjectPtr<InterfaceType>&& ptr)
        : ObjectPtr<InterfaceType>(std::move(ptr))
    {
    }

    GenericTypePtr(const ObjectPtr<InterfaceType>& ptr)
        : ObjectPtr<InterfaceType>(ptr)
    {
    }


    GenericTypePtr(const TypePtr& ptr)
        : daq::ObjectPtr<InterfaceType>(ptr)

    {
    }

    GenericTypePtr(TypePtr&& ptr) noexcept
        : daq::ObjectPtr<InterfaceType>(std::move(ptr))

    {
    }
        
    GenericTypePtr& operator=(const TypePtr& other)
    {
        if (this == &other)
            return *this;

        daq::ObjectPtr<InterfaceType>::operator =(other);
        return *this;
    }

    GenericTypePtr& operator=(TypePtr&& other) noexcept
    {
        if (this == std::addressof(other))
        {
            return *this;
        }

        daq::ObjectPtr<InterfaceType>::operator =(std::move(other));
        return *this;
    }

    /*!
     * @brief Gets the name of the Type
     * @returns The name of the Type.
     */
    StringPtr getName() const
    {
        if (this->object == nullptr)
            throw InvalidParameterException();

        StringPtr typeName;
        const auto errCode = this->object->getName(&typeName);
        checkErrorInfo(errCode);

        return typeName;
    }
};

/*!@}*/

END_NAMESPACE_OPENDAQ
