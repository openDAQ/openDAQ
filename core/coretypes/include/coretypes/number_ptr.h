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
#include <coretypes/number.h>
#include <coretypes/objectptr.h>

BEGIN_NAMESPACE_OPENDAQ

class NumberPtr;

template <>
struct InterfaceToSmartPtr<INumber>
{
    typedef NumberPtr SmartPtr;
};

/*!
 * @addtogroup types_numerics
 * @{
 */

/*!
 * @brief Represents either a float or an int number.
 *
 * Number is used if data type of the number is not strictly defined, i.e.
 * it can accept a float or an int.
 */
class NumberPtr : public ObjectPtr<INumber>
{
public:
    using ObjectPtr<INumber>::ObjectPtr;

    NumberPtr()
    {
    }

    NumberPtr(ObjectPtr<INumber>&& ptr)
        : ObjectPtr<INumber>(std::move(ptr))
    {
    }

    NumberPtr(const ObjectPtr<INumber>& ptr)
        : ObjectPtr<INumber>(ptr)
    {
    }

    /*!
     * @brief Gets a value stored in the object as a floating point value.
     * @return Stored value as a floating point.
     */
    Float getFloatValue() const
    {
        if (this->object == nullptr)
            throw InvalidParameterException();

        Float value;
        auto errCode = this->object->getFloatValue(&value);
        checkErrorInfo(errCode);

        return value;
    }

    /*!
     * @brief Gets a value stored in the object as an integer value.
     * @return Stored value as an integer.
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
};

/*!@}*/

END_NAMESPACE_OPENDAQ
