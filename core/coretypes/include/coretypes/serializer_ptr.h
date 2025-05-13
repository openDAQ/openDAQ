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
#include <coretypes/serializer.h>
#include <coretypes/serializable_ptr.h>
#include <coretypes/string_ptr.h>
#include <coretypes/baseobject_factory.h>

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
            DAQ_THROW_EXCEPTION(InvalidParameterException);

        ErrCode errCode = object->startObject();
        DAQ_CHECK_ERROR_INFO(errCode);
    }

    void startTaggedObject(const SerializablePtr& objPtr) const
    {
        if (!object)
            DAQ_THROW_EXCEPTION(InvalidParameterException);

        ErrCode errCode = object->startTaggedObject(objPtr);
        DAQ_CHECK_ERROR_INFO(errCode);
    }

    void endObject() const
    {
        if (!object)
            DAQ_THROW_EXCEPTION(InvalidParameterException);

        ErrCode errCode = object->endObject();
        DAQ_CHECK_ERROR_INFO(errCode);
    }

    void startList() const
    {
        if (!object)
            DAQ_THROW_EXCEPTION(InvalidParameterException);

        ErrCode errCode = object->startList();
        DAQ_CHECK_ERROR_INFO(errCode);
    }

    void endList() const
    {
        if (!object)
            DAQ_THROW_EXCEPTION(InvalidParameterException);

        ErrCode errCode = object->endList();
        DAQ_CHECK_ERROR_INFO(errCode);
    }

    StringPtr getOutput() const
    {
        if (!object)
            DAQ_THROW_EXCEPTION(InvalidParameterException);

        StringPtr serialized;
        ErrCode errCode = object->getOutput(&serialized);
        DAQ_CHECK_ERROR_INFO(errCode);

        return serialized;
    }

    void key(const StringPtr& name) const
    {
        if (!object)
            DAQ_THROW_EXCEPTION(InvalidParameterException);

        ErrCode errCode = object->keyStr(name);
        DAQ_CHECK_ERROR_INFO(errCode);
    }

    void key(ConstCharPtr name) const
    {
        if (!object)
            DAQ_THROW_EXCEPTION(InvalidParameterException);

        ErrCode errCode = object->key(name);
        DAQ_CHECK_ERROR_INFO(errCode);
    }

    void key(ConstCharPtr name, SizeT length) const
    {
        if (!object)
            DAQ_THROW_EXCEPTION(InvalidParameterException);

        ErrCode errCode = object->keyRaw(name, length);
        DAQ_CHECK_ERROR_INFO(errCode);
    }

    void writeInt(Int integer) const
    {
        if (!object)
            DAQ_THROW_EXCEPTION(InvalidParameterException);

        ErrCode errCode = object->writeInt(integer);
        DAQ_CHECK_ERROR_INFO(errCode);
    }

    void writeBool(Bool bolean) const
    {
        if (!object)
            DAQ_THROW_EXCEPTION(InvalidParameterException);

        ErrCode errCode = object->writeBool(bolean);
        DAQ_CHECK_ERROR_INFO(errCode);
    }

    void writeFloat(Float real) const
    {
        if (!object)
            DAQ_THROW_EXCEPTION(InvalidParameterException);

        ErrCode errCode = object->writeFloat(real);
        DAQ_CHECK_ERROR_INFO(errCode);
    }

    void writeNull() const
    {
        if (!object)
            DAQ_THROW_EXCEPTION(InvalidParameterException);

        ErrCode errCode = object->writeNull();
        DAQ_CHECK_ERROR_INFO(errCode);
    }

    void writeString(const StringPtr& string) const
    {
        if (!object)
            DAQ_THROW_EXCEPTION(InvalidParameterException);

        SizeT length;
        string->getLength(&length);

        ConstCharPtr value;
        DAQ_CHECK_ERROR_INFO(string->getCharPtr(&value));

        ErrCode errCode = object->writeString(value, length);
        DAQ_CHECK_ERROR_INFO(errCode);
    }

    void writeString(const std::string& str) const
    {
        if (!object)
            DAQ_THROW_EXCEPTION(InvalidParameterException);

        ErrCode errCode = object->writeString(str.data(), str.size());
        DAQ_CHECK_ERROR_INFO(errCode);
    }

    void writeString(ConstCharPtr string) const
    {
        if (!object)
            DAQ_THROW_EXCEPTION(InvalidParameterException);

        ErrCode errCode = object->writeString(string, strlen(string));
        DAQ_CHECK_ERROR_INFO(errCode);
    }

    void writeString(ConstCharPtr string, SizeT length) const
    {
        if (!object)
            DAQ_THROW_EXCEPTION(InvalidParameterException);

        ErrCode errCode = object->writeString(string, length);
        DAQ_CHECK_ERROR_INFO(errCode);
    }

    Bool isComplete() const
    {
        if (!object)
            DAQ_THROW_EXCEPTION(InvalidParameterException);

        Bool complete;
        ErrCode errCode = object->isComplete(&complete);
        DAQ_CHECK_ERROR_INFO(errCode);

        return complete;
    }

    void reset() const
    {
        if (!object)
            DAQ_THROW_EXCEPTION(InvalidParameterException);

        ErrCode errCode = object->reset();
        DAQ_CHECK_ERROR_INFO(errCode);
    }

    BaseObjectPtr getUser() const
    {
        if (!object)
            DAQ_THROW_EXCEPTION(InvalidParameterException);

        BaseObjectPtr userObject;
        ErrCode errCode = object->getUser(&userObject);
        DAQ_CHECK_ERROR_INFO(errCode);
        return userObject;
    }

    void setUser(const BaseObjectPtr& user) const
    {
        if (!object)
            DAQ_THROW_EXCEPTION(InvalidParameterException);

        ErrCode errCode = object->setUser(user);
        DAQ_CHECK_ERROR_INFO(errCode);
    }

    Int getVersion() const
    {
        if (!object)
            DAQ_THROW_EXCEPTION(InvalidParameterException);

        Int version;
        ErrCode errCode = object->getVersion(&version);
        DAQ_CHECK_ERROR_INFO(errCode);

        return version;
    }
};

/*!
 * @}
 */

END_NAMESPACE_OPENDAQ
