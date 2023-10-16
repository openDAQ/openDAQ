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
#include <coretypes/objectptr.h>
#include <coretypes/stringobject.h>
#include <string_view>

BEGIN_NAMESPACE_OPENDAQ

class StringPtr;

template <>
struct InterfaceToSmartPtr<IString>
{
    typedef StringPtr SmartPtr;
};

/*!
 * @addtogroup types_string
 * @{
 */

/*!
 * Represents string variable. Use this object to wrap string
 * variable when you need to add the variable to lists, dictionaries and other containers which
 * accept `IBaseObject` and derived interfaces.
 *
 * Available factories:
 * @code
 * StringPtr String(ConstCharPtr str)
 * StringPtr String(ConstCharPtr str, SizeT length)
 * StringPtr String(const std::string& str)
 * StringPtr operator"" _daq(const char* str)
 * @endcode
 */

class StringPtr : public ObjectPtr<IString>
{
public:
    using ObjectPtr<IString>::ObjectPtr;

    StringPtr()
    {
    }

    StringPtr(std::string& string)
        : ObjectPtr<IString>(string)
    {
    }

    StringPtr(std::string&& string)
        : ObjectPtr<IString>(string)
    {
    }

    StringPtr(ObjectPtr<IString>&& ptr)
        : ObjectPtr<IString>(std::move(ptr))
    {
    }

    StringPtr(const ObjectPtr<IString>& ptr)
        : ObjectPtr<IString>(ptr)
    {
    }

    StringPtr(const StringPtr& other)
        : ObjectPtr<IString>(other)
    {
    }

    StringPtr(StringPtr&& other) noexcept
        : ObjectPtr<IString>(std::move(other))
    {
    }

    StringPtr& operator=(const StringPtr& other)
    {
        if (this == &other)
            return *this;

        ObjectPtr<IString>::operator=(other);
        return *this;
    }

    StringPtr& operator=(StringPtr&& other) noexcept
    {
        if (this == std::addressof(other))
        {
            return *this;
        }

        ObjectPtr<IString>::operator=(std::move(other));
        return *this;
    }

    /*!
     * @brief Gets a string value stored in the object as std::string.
     * @return The string value as std::string.
     */
    [[nodiscard]] std::string toStdString() const
    {
        if (!object)
        {
            throw InvalidParameterException();
        }

        return getValue<std::string>("");
    }

    /*!
     * @brief Gets a string value stored in the object as std::string_view.
     * @return The string value as std::string_view.
     */
    [[nodiscard]] std::string_view toView() const
    {
        if (!object)
        {
            throw InvalidParameterException();
        }

        ConstCharPtr value;
        auto errCode = this->object->getCharPtr(&value);
        checkErrorInfo(errCode);

        SizeT size;
        errCode = this->object->getLength(&size);
        checkErrorInfo(errCode);

        return std::string_view(value, size);
    }

    /*!
     * @brief Gets a string value stored in the object as char*.
     * @return The string value as char*.
     */
    [[nodiscard]] ConstCharPtr getCharPtr() const
    {
        if (!object)
        {
            throw InvalidParameterException();
        }

        ConstCharPtr value;
        auto errCode = this->object->getCharPtr(&value);
        checkErrorInfo(errCode);

        return value;
    }

    /*!
     * @brief Gets length of string.
     * @return The size of the string.
     *
     * Call this method to get the length of the string. Null char terminator is not included in
     * the size of the string.
     */
    [[nodiscard]] SizeT getLength() const
    {
        if (!object)
        {
            throw InvalidParameterException();
        }

        SizeT size;
        auto errCode = this->object->getLength(&size);
        checkErrorInfo(errCode);

        return size;
    }

    char operator[](std::size_t index) const
    {
        return getCharPtr()[index];
    }

    StringPtr operator+(const char rhs[]) const
    {
        return StringPtr(toStdString() + std::string(rhs));
    }

    friend std::ostream& operator<<(std::ostream& stream, const StringPtr& string)
    {
        stream << string.getCharPtr();

        return stream;
    }
};

/*!@}*/

END_NAMESPACE_OPENDAQ
