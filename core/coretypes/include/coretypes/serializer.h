/*
 * Copyright 2022-2024 Blueberry d.o.o.
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
#include <coretypes/baseobject.h>
#include <coretypes/stringobject.h>

BEGIN_NAMESPACE_OPENDAQ

struct ISerializable;

/*!
 * @ingroup types_serialization
 * @defgroup types_serializer Serializer
 * @{
 */

DECLARE_OPENDAQ_INTERFACE(ISerializer, IBaseObject)
{
    /**
    * Starts an object with the class / type identifier
    * @return A non-zero error code if an error occurred
    */
    virtual ErrCode INTERFACE_FUNC startTaggedObject(ISerializable* obj) = 0;

    /**
     * Starts a plain object (without an identifier)
     * @return A non-zero error code if an error occurred
     */
    virtual ErrCode INTERFACE_FUNC startObject() = 0;
    virtual ErrCode INTERFACE_FUNC endObject() = 0;

    virtual ErrCode INTERFACE_FUNC startList() = 0;
    virtual ErrCode INTERFACE_FUNC endList() = 0;

    virtual ErrCode INTERFACE_FUNC getOutput(IString** serialized) = 0;

    virtual ErrCode INTERFACE_FUNC key(ConstCharPtr string) = 0;
    virtual ErrCode INTERFACE_FUNC keyStr(IString* name) = 0;
    virtual ErrCode INTERFACE_FUNC keyRaw(ConstCharPtr string, SizeT length) = 0;

    virtual ErrCode INTERFACE_FUNC writeInt(Int integer) = 0;
    virtual ErrCode INTERFACE_FUNC writeBool(Bool boolean) = 0;
    virtual ErrCode INTERFACE_FUNC writeFloat(Float real) = 0;
    virtual ErrCode INTERFACE_FUNC writeString(ConstCharPtr string, SizeT length) = 0;
    virtual ErrCode INTERFACE_FUNC writeNull() = 0;

    virtual ErrCode INTERFACE_FUNC reset() = 0;
    virtual ErrCode INTERFACE_FUNC isComplete(Bool* complete) = 0;
};

/*!
 * @}
 */

#define SERIALIZE_PROP_PTR(name)                                                                             \
    if (this->name.assigned())                                                                               \
    {                                                                                                        \
        ISerializable* serializable;                                                                         \
        ErrCode errCode = name->borrowInterface(ISerializable::Id, reinterpret_cast<void**>(&serializable)); \
                                                                                                             \
        if (errCode == OPENDAQ_ERR_NOINTERFACE)                                                              \
        {                                                                                                    \
            return OPENDAQ_ERR_NOT_SERIALIZABLE;                                                             \
        }                                                                                                    \
                                                                                                             \
        if (OPENDAQ_FAILED(errCode))                                                                         \
        {                                                                                                    \
            return errCode;                                                                                  \
        }                                                                                                    \
                                                                                                             \
        serializer->key(#name);                                                                              \
        errCode = serializable->serialize(serializer);                                                       \
                                                                                                             \
        if (OPENDAQ_FAILED(errCode))                                                                         \
        {                                                                                                    \
            return errCode;                                                                                  \
        }                                                                                                    \
    }

END_NAMESPACE_OPENDAQ
