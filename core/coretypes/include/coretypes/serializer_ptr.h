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
#include <coretypes/serializer.h>
#include <coretypes/serializable_ptr.h>
#include <coretypes/string_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @addtogroup types_serializer
 * @{
 */

class SerializerPtr : public ObjectPtr<ISerializer>
{
public:
    using ObjectPtr<ISerializer>::ObjectPtr;

    inline static SerializerPtr Borrow(ISerializer*& obj)
    {
        SerializerPtr objPtr;
        objPtr.object = obj;
        objPtr.borrowed = true;
        return objPtr;
    }

    inline SerializerPtr Borrow(const SerializerPtr& objPtr) const
    {
        SerializerPtr objPtrThis;
        objPtrThis.object = objPtr.object;
        objPtrThis.borrowed = true;
        return objPtrThis;
    }

    void startObject() const
    {
        if (!object)
            throw InvalidParameterException();

        ErrCode errCode = object->startObject();
        checkErrorInfo(errCode);
    }

    void startTaggedObject(const SerializablePtr& objPtr) const
    {
        if (!object)
            throw InvalidParameterException();

        ErrCode errCode = object->startTaggedObject(objPtr);
        checkErrorInfo(errCode);
    }

    void endObject() const
    {
        if (!object)
            throw InvalidParameterException();

        ErrCode errCode = object->endObject();
        checkErrorInfo(errCode);
    }

    void startList() const
    {
        if (!object)
            throw InvalidParameterException();

        ErrCode errCode = object->startList();
        checkErrorInfo(errCode);
    }

    void endList() const
    {
        if (!object)
            throw InvalidParameterException();

        ErrCode errCode = object->endList();
        checkErrorInfo(errCode);
    }

    StringPtr getOutput() const
    {
        if (!object)
            throw InvalidParameterException();

        StringPtr serialized;
        ErrCode errCode = object->getOutput(&serialized);
        checkErrorInfo(errCode);

        return serialized;
    }

    void key(const StringPtr& name) const
    {
        if (!object)
            throw InvalidParameterException();

        ErrCode errCode = object->keyStr(name);
        checkErrorInfo(errCode);
    }

    void key(ConstCharPtr name) const
    {
        if (!object)
            throw InvalidParameterException();

        ErrCode errCode = object->key(name);
        checkErrorInfo(errCode);
    }

    void key(ConstCharPtr name, SizeT length) const
    {
        if (!object)
            throw InvalidParameterException();

        ErrCode errCode = object->keyRaw(name, length);
        checkErrorInfo(errCode);
    }

    void writeInt(Int integer) const
    {
        if (!object)
            throw InvalidParameterException();

        ErrCode errCode = object->writeInt(integer);
        checkErrorInfo(errCode);
    }

    void writeBool(Bool bolean) const
    {
        if (!object)
            throw InvalidParameterException();

        ErrCode errCode = object->writeBool(bolean);
        checkErrorInfo(errCode);
    }

    void writeFloat(Float real) const
    {
        if (!object)
            throw InvalidParameterException();

        ErrCode errCode = object->writeFloat(real);
        checkErrorInfo(errCode);
    }

    void writeNull() const
    {
        if (!object)
            throw InvalidParameterException();

        ErrCode errCode = object->writeNull();
        checkErrorInfo(errCode);
    }

    void writeString(const StringPtr& string) const
    {
        if (!object)
            throw InvalidParameterException();

        SizeT length;
        string->getLength(&length);

        ConstCharPtr value;
        checkErrorInfo(string->getCharPtr(&value));

        ErrCode errCode = object->writeString(value, length);
        checkErrorInfo(errCode);
    }

    void writeString(const std::string& str) const
    {
        if (!object)
            throw InvalidParameterException();

        ErrCode errCode = object->writeString(str.data(), str.size());
        checkErrorInfo(errCode);
    }

    void writeString(ConstCharPtr string) const
    {
        if (!object)
            throw InvalidParameterException();

        ErrCode errCode = object->writeString(string, strlen(string));
        checkErrorInfo(errCode);
    }

    void writeString(ConstCharPtr string, SizeT length) const
    {
        if (!object)
            throw InvalidParameterException();

        ErrCode errCode = object->writeString(string, length);
        checkErrorInfo(errCode);
    }

    Bool isComplete() const
    {
        if (!object)
            throw InvalidParameterException();

        Bool complete;
        ErrCode errCode = object->isComplete(&complete);
        checkErrorInfo(errCode);

        return complete;
    }

    void reset() const
    {
        if (!object)
            throw InvalidParameterException();

        ErrCode errCode = object->reset();
        checkErrorInfo(errCode);
    }
};

/*!
 * @}
 */

END_NAMESPACE_OPENDAQ
