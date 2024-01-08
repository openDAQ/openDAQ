/*
 * Copyright 2022-2023 Blueberry d.o.o.
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
#include <coretypes/enumeration.h>
#include <coretypes/objectptr.h>
#include <coretypes/dictobject_factory.h>
#include <coretypes/baseobject_factory.h>
#include <coretypes/enumeration_type_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class EnumerationPtr;

template <>
struct InterfaceToSmartPtr<daq::IEnumeration>
{
    using SmartPtr = daq::EnumerationPtr;
};

/*!
 * @ingroup types_enumeration
 * @defgroup types_enumerations_enumeration Enumeration
 * @{
 */

/*!
 * @brief Enumerations are objects that encapsulate a value within a predefined set of named integral constants.
 * These constants are predefined in an Enumeration type with the same name as the Enumeration.
 * 
 * The Enumeration types are stored within a Type manager. In any given instance of openDAQ, a single Type manager should
 * exist that is part of its Context.
 * 
 * When creating an Enumeration object, the Type manager is used to validate the given enumerator value name against the
 * Enumeration type stored within the manager. If no type with the given Enumeration name is currently stored,
 * construction of the Enumeration object will fail. Similarly, if the provided enumerator value name is not part of
 * the Enumeration type, the construction of the Enumeration object will also fail.
 */

class EnumerationPtr : public daq::ObjectPtr<IEnumeration>
{
public:
    using daq::ObjectPtr<IEnumeration>::ObjectPtr;

    EnumerationPtr()
        : daq::ObjectPtr<IEnumeration>()

    {
    }

    EnumerationPtr(daq::ObjectPtr<IEnumeration>&& ptr)
        : daq::ObjectPtr<IEnumeration>(std::move(ptr))

    {
    }

    EnumerationPtr(const daq::ObjectPtr<IEnumeration>& ptr)
        : daq::ObjectPtr<IEnumeration>(ptr)

    {
    }

    EnumerationPtr(const EnumerationPtr& other)
        : daq::ObjectPtr<IEnumeration>(other)

    {
    }

    EnumerationPtr(EnumerationPtr&& other) noexcept
        : daq::ObjectPtr<IEnumeration>(std::move(other))

    {
    }
    
    EnumerationPtr& operator=(const EnumerationPtr& other)
    {
        if (this == &other)
            return *this;

        daq::ObjectPtr<IEnumeration>::operator =(other);

        return *this;
    }

    EnumerationPtr& operator=(EnumerationPtr&& other) noexcept
    {
        if (this == std::addressof(other))
        {
            return *this;
        }

        daq::ObjectPtr<IEnumeration>::operator =(std::move(other));

        return *this;
    }

    /*!
     * @brief Gets the Enumeration's type.
     * @returns The Enumeration type
     */
    daq::EnumerationTypePtr getEnumerationType() const
    {
        if (this->object == nullptr)
            throw daq::InvalidParameterException();

        daq::EnumerationTypePtr type;
        auto errCode = this->object->getEnumerationType(&type);
        daq::checkErrorInfo(errCode);

        return type;
    }

    /*!
     * @brief Gets the Enumeration value as String containing the name of the enumerator constant.
     * @returns Emumeration value.
     */
    daq::StringPtr getValue() const
    {
        if (this->object == nullptr)
            throw daq::InvalidParameterException();

        daq::StringPtr value;
        auto errCode = this->object->getValue(&value);
        daq::checkErrorInfo(errCode);

        return value;
    }
};

/*!@}*/

END_NAMESPACE_OPENDAQ
