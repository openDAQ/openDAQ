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
#include <coretypes/enumeration.h>
#include <coretypes/objectptr.h>
#include <coretypes/dictobject_factory.h>
#include <coretypes/baseobject_factory.h>
#include <coretypes/enumeration_type_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class EnumerationPtr;

template <>
struct InterfaceToSmartPtr<IEnumeration>
{
    using SmartPtr = EnumerationPtr;
};

/*!
 * @ingroup types_enumerations
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
 *
 * Since the Enumerations objects are immutable the value of an existing Enumeration object cannot be modified.
 * However, the Enumeration object encapsulated by a smart pointer of the corresponding type can be replaced
 * with a newly created one. This replacement is accomplished using the assignment operator with the right
 * operand being a constant string literal containing the enumerator value name valid for the Enumeration type
 * of the original Enumeration object.
 */

class EnumerationPtr : public ObjectPtr<IEnumeration>
{
public:
    using ObjectPtr<IEnumeration>::ObjectPtr;

    EnumerationPtr()
        : ObjectPtr<IEnumeration>()

    {
    }

    EnumerationPtr(ObjectPtr<IEnumeration>&& ptr)
        : ObjectPtr<IEnumeration>(std::move(ptr))

    {
    }

    EnumerationPtr(const ObjectPtr<IEnumeration>& ptr)
        : ObjectPtr<IEnumeration>(ptr)

    {
    }

    EnumerationPtr(const EnumerationPtr& other)
        : ObjectPtr<IEnumeration>(other)

    {
    }

    EnumerationPtr(EnumerationPtr&& other) noexcept
        : ObjectPtr<IEnumeration>(std::move(other))

    {
    }
    
    EnumerationPtr& operator=(const EnumerationPtr& other)
    {
        if (this == &other)
            return *this;

        ObjectPtr<IEnumeration>::operator =(other);

        return *this;
    }

    EnumerationPtr& operator=(EnumerationPtr&& other) noexcept
    {
        if (this == std::addressof(other))
        {
            return *this;
        }

        ObjectPtr<IEnumeration>::operator =(std::move(other));

        return *this;
    }

    EnumerationPtr& operator=(const wchar_t* value)
    {
        StringPtr stringObj = value;

        return fromStringObj(stringObj);
    }

    EnumerationPtr& operator=(const char* value)
    {
        StringPtr stringObj = value;

        return fromStringObj(stringObj);
    }

    /*!
     * @brief Gets the Enumeration's type.
     * @returns The Enumeration type
     */
    EnumerationTypePtr getEnumerationType() const
    {
        if (this->object == nullptr)
            throw InvalidParameterException();

        EnumerationTypePtr type;
        auto errCode = this->object->getEnumerationType(&type);
        checkErrorInfo(errCode);

        return type;
    }

    /*!
     * @brief Gets the Enumeration value as String containing the name of the enumerator constant.
     * @returns Emumeration value.
     */
    StringPtr getValue() const
    {
        if (this->object == nullptr)
            throw InvalidParameterException();

        StringPtr value;
        auto errCode = this->object->getValue(&value);
        checkErrorInfo(errCode);

        return value;
    }

    /*!
     * @brief Gets the Enumeration value as Integer enumerator constant.
     * @returns Emumeration Integer value.
     */
    Int getIntValue() const
    {
        if (this->object == nullptr)
            throw InvalidParameterException();

        Int value;
        auto errCode = this->object->getIntValue(&value);
        checkErrorInfo(errCode);

        return value;
    }

private:

    EnumerationPtr& fromStringObj(const StringPtr& value)
    {
        if (this->object == nullptr)
            throw InvalidParameterException();

        StringPtr val;
        auto errCode = this->object->getValue(&val);
        checkErrorInfo(errCode);

        if (val == value)
        {
            return *this;
        }

        EnumerationTypePtr type;
        errCode = this->object->getEnumerationType(&type);
        checkErrorInfo(errCode);

        EnumerationPtr other = EnumerationWithType_Create(type, value);

        ObjectPtr<IEnumeration>::operator =(other);

        return *this;
    }
};

/*!@}*/

END_NAMESPACE_OPENDAQ
