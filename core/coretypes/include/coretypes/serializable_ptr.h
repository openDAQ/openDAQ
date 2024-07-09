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

#include <coretypes/serializable.h>
#include <coretypes/objectptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @addtogroup types_serializable
 * @{
 */

class SerializablePtr;

template <>
struct InterfaceToSmartPtr<ISerializable>
{
    typedef SerializablePtr SmartPtr;
};

class SerializablePtr : public ObjectPtr<ISerializable>
{
public:
    using ObjectPtr<ISerializable>::ObjectPtr;

    static SerializablePtr Borrow(ISerializable*& obj)
    {
        SerializablePtr objPtr;
        objPtr.object = obj;
        objPtr.borrowed = true;
        return objPtr;
    }

    SerializablePtr Borrow(const SerializablePtr& objPtr) const
    {
        SerializablePtr objPtrThis;
        objPtrThis.object = objPtr.object;
        objPtrThis.borrowed = true;
        return objPtrThis;
    }

    void serialize(const ObjectPtr<ISerializer>& serializator) const
    {
        if (!object)
            throw InvalidParameterException();

        ErrCode errCode = object->serialize(serializator);
        checkErrorInfo(errCode);
    }
};

/*!
 * @}
 */

END_NAMESPACE_OPENDAQ
