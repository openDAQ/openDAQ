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
#include <coretypes/serialized_object.h>
#include <coretypes/stringobject_factory.h>
#include <coretypes/baseobject_factory.h>
#include <coretypes/listobject_factory.h>
#include <coretypes/function_ptr.h>

BEGIN_NAMESPACE_OPENDAQ


class SerializedObjectPtr;

template <>
struct InterfaceToSmartPtr<ISerializedObject>
{
    typedef SerializedObjectPtr SmartPtr;
};

/*!
 * @addtogroup types_serialized_object
 * @{
 */

class SerializedObjectPtr : public ObjectPtr<ISerializedObject>
{
public:
    using ObjectPtr<ISerializedObject>::ObjectPtr;

    SerializedObjectPtr()
    {
    }

    static SerializedObjectPtr Borrow(ISerializedObject*& obj)
    {
        SerializedObjectPtr objPtr;
        objPtr.object = obj;
        objPtr.borrowed = true;
        return objPtr;
    }

    static SerializedObjectPtr Borrow(const SerializedObjectPtr& objPtr)
    {
        SerializedObjectPtr objPtrThis;
        objPtrThis.object = objPtr.object;
        objPtrThis.borrowed = true;
        return objPtrThis;
    }

    SerializedObjectPtr readSerializedObject(const StringPtr& key) const
    {
        if (!object)
            throw InvalidParameterException();

        SerializedObjectPtr value;
        checkErrorInfo(object->readSerializedObject(key, &value));

        return value;
    }

    BaseObjectPtr readObject(const StringPtr& key, const BaseObjectPtr& context = nullptr, const FunctionPtr& factoryCallback = nullptr) const
    {
        if (!object)
            throw InvalidParameterException();

        IBaseObject* value;
        checkErrorInfo(object->readObject(key, context, factoryCallback, &value));

        return BaseObjectPtr(std::move(value));
    }

    StringPtr readString(const StringPtr& key) const
    {
        if (!object)
            throw InvalidParameterException();

        StringPtr value;
        checkErrorInfo(object->readString(key, &value));

        return value;
    }

    Bool readBool(const StringPtr& key) const
    {
        if (!object)
            throw InvalidParameterException();

        Bool value;
        checkErrorInfo(object->readBool(key, &value));

        return value;
    }

    Int readInt(const StringPtr& key) const
    {
        if (!object)
            throw InvalidParameterException();

        Int value;
        ErrCode errCode = object->readInt(key, &value);
        checkErrorInfo(errCode);

        return value;
    }

    Float readFloat(const StringPtr& key) const
    {
        if (!object)
            throw InvalidParameterException();

        Float value;
        checkErrorInfo(object->readFloat(key, &value));

        return value;
    }

    ListPtr<IString> getKeys() const
    {
        if (!object)
            throw InvalidParameterException();

        IList* keys;
        checkErrorInfo(object->getKeys(&keys));

        return ListPtr<IString>(std::move(keys));
    }

    Bool hasKey(const StringPtr& hasMember) const
    {
        if (!object)
            throw InvalidParameterException();

        Bool keys;
        checkErrorInfo(object->hasKey(hasMember, &keys));

        return keys;
    }

    template <typename T>
    ListPtr<T> readList(const StringPtr& key, const BaseObjectPtr& context = nullptr, const FunctionPtr& factoryCallback = nullptr) const
    {
        if (!object)
            throw InvalidParameterException();

        IList* value;
        checkErrorInfo(object->readList(key, context, factoryCallback, &value));

        return ListPtr<T>(std::move(value));
    }

    ObjectPtr<IBaseObject> readSerializedList(const StringPtr& key) const
    {
        if (!object)
            throw InvalidParameterException();

        ObjectPtr<ISerializedList> value;
        checkErrorInfo(object->readSerializedList(key, &value));

        return value;
    }

    CoreType getType(const StringPtr& key) const
    {
        if (!object)
            throw InvalidParameterException();

        CoreType ct;
        checkErrorInfo(object->getType(key, &ct));
        return ct;
    }

    void checkObjectType(const std::string& objectType) const
    {
        if (objectType.empty())
            return;

        const auto type = readString("__type");
        if (type.toStdString() != objectType)
            throw InvalidTypeException(fmt::format("Object not of ""{}"" type", objectType));
    }

};

/*!
 * @}
 */

END_NAMESPACE_OPENDAQ
