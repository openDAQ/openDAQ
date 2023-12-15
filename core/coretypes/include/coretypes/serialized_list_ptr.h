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
#include <coretypes/serialized_list.h>
#include <coretypes/baseobject_factory.h>
#include <coretypes/stringobject_factory.h>
#include <coretypes/listobject_factory.h>

BEGIN_NAMESPACE_OPENDAQ

class SerializedObjectPtr;

/*!
 * @addtogroup types_serialized_list
 */

class SerializedListPtr : public ObjectPtr<ISerializedList>
{
public:
    using ObjectPtr<ISerializedList>::ObjectPtr;

    SerializedListPtr()
    {
    }

    SerializedListPtr Borrow(ISerializedList*& obj)
    {
        SerializedListPtr objPtr;
        objPtr.object = obj;
        objPtr.borrowed = true;
        return objPtr;
    }

    SerializedListPtr Borrow(const SerializedListPtr& objPtr) const
    {
        SerializedListPtr objPtrThis;
        objPtrThis.object = objPtr.object;
        objPtrThis.borrowed = true;
        return objPtrThis;
    }

    SerializedListPtr readSerializedList() const
    {
        if (!object)
            throw InvalidParameterException();

        ISerializedList* value;
        checkErrorInfo(object->readSerializedList(&value));

        return SerializedListPtr(std::move(value));
    }

    template <typename T>
    ListPtr<T> readList(const BaseObjectPtr& context = nullptr, const FunctionPtr& factoryCallback = nullptr) const
    {
        if (!object)
            throw InvalidParameterException();

        IList* value;
        checkErrorInfo(object->readList(context, factoryCallback, &value));

        return ListPtr<T>(std::move(value));
    }

    BaseObjectPtr readObject(const BaseObjectPtr& context = nullptr, const FunctionPtr& factoryCallback = nullptr) const
    {
        if (!object)
            throw InvalidParameterException();

        IBaseObject* value;
        checkErrorInfo(object->readObject(context, factoryCallback, &value));

        return BaseObjectPtr(std::move(value));
    }

    StringPtr readString() const
    {
        if (!object)
            throw InvalidParameterException();

        IString* value;
        checkErrorInfo(object->readString(&value));

        return StringPtr(std::move(value));
    }

    Bool readBool() const
    {
        if (!object)
            throw InvalidParameterException();

        Bool value;
        checkErrorInfo(object->readBool(&value));

        return value;
    }

    Int readInt() const
    {
        if (!object)
            throw InvalidParameterException();

        Int value;
        checkErrorInfo(object->readInt(&value));

        return value;
    }

    Float readFloat() const
    {
        if (!object)
            throw InvalidParameterException();

        Float value;
        checkErrorInfo(object->readFloat(&value));

        return value;
    }

    SizeT getCount() const
    {
        if (!object)
            throw InvalidParameterException();

        SizeT value;
        checkErrorInfo(object->getCount(&value));

        return value;
    }

    ObjectPtr<ISerializedObject> readSerializedObject() const
    {
        if (!object)
            throw InvalidParameterException();

        ObjectPtr<ISerializedObject> value;
        checkErrorInfo(object->readSerializedObject(&value));

        return value;
    }

    CoreType getCurrentItemType() const
    {
        if (!object)
            throw InvalidParameterException();

        CoreType ct;
        checkErrorInfo(object->getCurrentItemType(&ct));
        return ct;
    }
};

/*!@}*/

END_NAMESPACE_OPENDAQ
