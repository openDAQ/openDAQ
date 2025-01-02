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
#include <coretypes/type_ptr.h>
#include <coretypes/enumeration_type.h>
#include <coretypes/listobject_factory.h>
#include <coretypes/dictobject_factory.h>

BEGIN_NAMESPACE_OPENDAQ

class EnumerationTypePtr;

template <>
struct InterfaceToSmartPtr<IEnumerationType>
{
    using SmartPtr = EnumerationTypePtr;
};

/*!
 * @ingroup types_enumerations
 * @defgroup types_enumerations_enumeration_type EnumerationType
 * @{
 */

/*!
 * @brief Enumeration types define the enumerator names and values of Enumerations with a name matching
 * that of the Enumeration type.
 * 
 * An Enumeration type provides a String-type name, a list of enumerator names (list of Strings) and
 * a list of enumerator values (list of Integer objects). To use Enumeration types for creating Enumeration
 * objects, they must be added to the Type Manager. Alternatively, if an Enumeration is created without
 * a matching Enumeration type in the manager, a default Enumeration type is generated based on the
 * enumerator names and values of the created Enumeration object. This generated Enumeration type is then
 * added to the Type Manager.
 * 
 * The enumerator values are represented as a list of Integer objects. These values can be explicitly specified
 * during Enumeration type creation, or only the first enumerator value can be specified, with the rest
 * automatically assigned in ascending order, starting from that value (or from the default '0' value if not
 * specified).
 */

class EnumerationTypePtr : public GenericTypePtr<IEnumerationType>
{
public:
    using GenericTypePtr<IEnumerationType>::GenericTypePtr;

    EnumerationTypePtr()
        : GenericTypePtr<IEnumerationType>()

    {
    }

    EnumerationTypePtr(ObjectPtr<IEnumerationType>&& ptr)
        : GenericTypePtr<IEnumerationType>(std::move(ptr))

    {
    }

    EnumerationTypePtr(const ObjectPtr<IEnumerationType>& ptr)
        : GenericTypePtr<IEnumerationType>(ptr)

    {
    }

    EnumerationTypePtr(const EnumerationTypePtr& other)
        : GenericTypePtr<IEnumerationType>(other)

    {
    }

    EnumerationTypePtr(EnumerationTypePtr&& other) noexcept
        : GenericTypePtr<IEnumerationType>(std::move(other))

    {
    }
    
    EnumerationTypePtr& operator=(const EnumerationTypePtr& other)
    {
        if (this == &other)
            return *this;

        GenericTypePtr<IEnumerationType>::operator =(other);

        return *this;
    }

    EnumerationTypePtr& operator=(EnumerationTypePtr&& other) noexcept
    {
        if (this == std::addressof(other))
        {
            return *this;
        }

        GenericTypePtr<IEnumerationType>::operator =(std::move(other));

        return *this;
    }

    /*!
     * @brief Gets the list of enumerator names.
     * @returns The list of enumerator names (String objects)
     */
    ListPtr<IString> getEnumeratorNames() const
    {
        if (this->object == nullptr)
            throw InvalidParameterException();

        ListPtr<IString> names;
        auto errCode = this->object->getEnumeratorNames(&names);
        checkErrorInfo(errCode);

        return names;
    }

    /*!
     * @brief Gets the enumerator names and values as a Dictionary.
     * @returns The Dictionary object with enumerator names as keys, and enumerator values     * as its values.
     */
    DictPtr<IString, IInteger> getAsDictionary() const
    {
        if (this->object == nullptr)
            throw InvalidParameterException();

        DictPtr<IString, IInteger> dictionary;
        auto errCode = this->object->getAsDictionary(&dictionary);
        checkErrorInfo(errCode);

        return dictionary;
    }

    /*!
     * @brief Gets the value of enumerator with the specified name.
     * @param name The name of the enumerator (String object).
     * @returns The integer value of the enumerator with the specified name.
     */
    Int getEnumeratorIntValue(const StringPtr& name) const
    {
        if (this->object == nullptr)
            throw InvalidParameterException();

        Int value;
        auto errCode = this->object->getEnumeratorIntValue(name, &value);
        checkErrorInfo(errCode);

        return value;
    }

    /*!
     * @brief Gets the number of enumerators within the Enumeration Type.
     * @returns The number of enumerators within the Enumeration Type.
     */
    SizeT getCount() const
    {
        if (this->object == nullptr)
            throw InvalidParameterException();

        SizeT count;
        auto errCode = this->object->getCount(&count);
        checkErrorInfo(errCode);

        return count;
    }
};

/*!@}*/

END_NAMESPACE_OPENDAQ
