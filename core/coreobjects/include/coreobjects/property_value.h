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
#include <coreobjects/property_object_ptr.h>
#include <coretypes/coretype_traits.h>

BEGIN_NAMESPACE_OPENDAQ

template <typename T, typename Enable = void>
struct PropertyValueHelper;

template <typename T>
struct PropertyValueHelper<T, typename std::enable_if<std::is_base_of<IBaseObject, T>::value>::type>
{
    typedef T Interface;
    typedef typename InterfaceToSmartPtr<T>::SmartPtr InitType;
};

template <typename T>
struct PropertyValueHelper<T, typename std::enable_if<!std::is_base_of<IBaseObject, T>::value>::type>
{
    typedef typename CoreTypeHelper<T>::Interface Interface;
    typedef T InitType;
};

template <typename T>
class PropertyValue
{
public:
    using Interface = typename PropertyValueHelper<T>::Interface;
    using SmartPtr = typename InterfaceToSmartPtr<Interface>::SmartPtr;

    explicit PropertyValue(IPropertyObject* propObj, StringPtr name)
        : propName(name)
        , propObj(propObj)
    {
    }

    explicit PropertyValue(IPropertyObject* propObj, StringPtr name, const typename PropertyValueHelper<T>::InitType& value)
        : propName(name)
        , propObj(propObj)
    {
        this->operator=(value);
    }

    ~PropertyValue() = default;

    // prevent moving or copying
    PropertyValue(const PropertyValue& other) = delete;
    PropertyValue(PropertyValue&& other) noexcept = delete;
    PropertyValue& operator=(const PropertyValue& other) = delete;
    PropertyValue& operator=(PropertyValue&& other) noexcept = delete;

    template <typename U = SmartPtr>
    U get() const
    {
        return getProperty();
    }

    template <typename U = SmartPtr>
    U getWithDefault(SmartPtr defaultValue) const
    {
        ObjectPtr<IBaseObject> ptr;
        ErrCode errCode = propObj->getPropertyValue(propName, &ptr);

        if (errCode == OPENDAQ_ERR_NOTASSIGNED || errCode == OPENDAQ_ERR_NOTFOUND)
            return defaultValue;

        checkErrorInfo(errCode);
        return ptr;
    }

    template <typename U = SmartPtr>
    U getOrNull() const
    {
        return getWithDefault(nullptr);
    }

    template <typename U = T, typename = typename std::enable_if<std::is_same<Bool, U>::value>::type>
    operator bool() const
    {
        SmartPtr ptr = getProperty();

        return ptr;
    }

    SmartPtr set(SmartPtr const& value)
    {
        ErrCode errCode = propObj->setPropertyValue(propName, value);
        checkErrorInfo(errCode);

        return value;
    }

    SmartPtr setProtected(SmartPtr const& value)
    {
        PropertyObjectProtectedPtr protectedObj;
        propObj->queryInterface(IPropertyObjectProtected::Id, reinterpret_cast<void**>(&protectedObj));

        ErrCode errCode = protectedObj->setProtectedPropertyValue(propName, value);
        checkErrorInfo(errCode);

        return value;
    }

    void clear() const
    {
        ErrCode errCode = propObj->clearPropertyValue(propName);
        checkErrorInfo(errCode);
    }

    template <typename TInterface, typename TPtr = typename InterfaceToSmartPtr<TInterface>::SmartPtr>
    TPtr asPtr()
    {
        return getProperty(). template asPtr<TInterface, TPtr>(false);
    }

    template <typename TInterface, typename TPtr = typename InterfaceToSmartPtr<TInterface>::SmartPtr>
    TPtr asPtrOrNull()
    {
        return getProperty(). template asPtrOrNull<TInterface, TPtr>(false);
    }

    SmartPtr operator()(SmartPtr const& value)
    {
        ErrCode errCode = propObj->setPropertyValue(propName, value);
        checkErrorInfo(errCode);

        return value;
    }

    // access with '=' sign
    operator SmartPtr() const
    {
        return getProperty();
    }

    Interface* addRefAndReturn() const
    {
        SmartPtr ptr = getProperty();

        return ptr.addRefAndReturn();
    }

    Interface* detach() const
    {
        SmartPtr ptr = getProperty();

        return ptr.detach();
    }

    // Direct conversion operator for value types
    template <typename U = T>
    operator std::enable_if_t<is_ct_conv<U>::value, U>() const // NOLINT(google-explicit-constructor)
    {
        return getProperty();
    }

    template <typename U, typename = std::enable_if_t<std::is_enum_v<U>>>
    operator U() const // NOLINT(google-explicit-constructor)
    {
        ObjectPtr<IBaseObject> ptr = getProperty();
        Int enumVal = Int(ptr.asPtr<IInteger>(true));

        return static_cast<U>(enumVal);
    }

    SmartPtr operator->() const
    {
        return getProperty();
    }

    PropertyValue& operator=(SmartPtr const& value)
    {
        ErrCode errCode = propObj->setPropertyValue(propName, value);
        checkErrorInfo(errCode);

        return *this;
    }

    template <typename U, typename = std::enable_if_t<std::is_arithmetic_v<U> && std::is_arithmetic_v<T>>>
    bool operator >(const U& value) const
    {
        return static_cast<T>(getProperty()) > value;
    }

    template <typename U, typename = std::enable_if_t<std::is_arithmetic_v<U> && std::is_arithmetic_v<T>>>
    bool operator<(const U& value) const
    {
        return static_cast<T>(getProperty()) < value;
    }

    template <typename U = T, typename = std::enable_if_t<std::is_arithmetic_v<U>>>
    PropertyValue& operator--(int)
    {
        ObjectPtr<IBaseObject> ptr = getProperty();

        SmartPtr value = ptr - 1;
        ErrCode errCode = propObj->setPropertyValue(propName, value);

        checkErrorInfo(errCode);
        return *this;
    }

    template <typename U = T, typename = std::enable_if_t<std::is_arithmetic_v<U>>>
    PropertyValue& operator++(int)
    {
        ObjectPtr<IBaseObject> ptr = getProperty();
        SmartPtr value = ptr + 1;

        ErrCode errCode = propObj->setPropertyValue(propName, value);

        checkErrorInfo(errCode);
        return *this;
    }

    template <typename U = T, typename = std::enable_if_t<std::is_arithmetic_v<U>>>
    bool operator==(U compare)
    {
        ObjectPtr<IBaseObject> ptr = getProperty();
        SmartPtr value = ptr;

        return value == compare;
    }

    template <typename U = T, typename = std::enable_if_t<std::is_arithmetic_v<U>>>
    bool operator!=(U compare)
    {
        return !operator==(compare);
    }

    template <typename U, typename = std::enable_if_t<std::is_enum_v<U>>>
    PropertyValue& operator=(U const& value)
    {
        ErrCode errCode = propObj->setPropertyValue(propName, Integer(static_cast<EnumType>(value)));
        checkErrorInfo(errCode);

        return *this;
    }

private:
    StringPtr propName;
    IPropertyObject* propObj;

    [[nodiscard]]
    ObjectPtr<IBaseObject> getProperty() const
    {
        ObjectPtr<IBaseObject> ptr;
        ErrCode errCode = propObj->getPropertyValue(propName, &ptr);
        checkErrorInfo(errCode);

        return ptr;
    }
};

END_NAMESPACE_OPENDAQ
