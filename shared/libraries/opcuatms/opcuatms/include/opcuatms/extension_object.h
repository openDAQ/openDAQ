/*
 * Copyright 2022-2024 openDAQ d.o.o.
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
#include "opcuatms/opcuatms.h"
#include "opcuashared/opcuavariant.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

class ExtensionObject : public OpcUaObject<UA_ExtensionObject>
{
public:
    using Super = OpcUaObject<UA_ExtensionObject>;
    using Super::Super;

    ExtensionObject();
    ExtensionObject(const OpcUaObject<UA_ExtensionObject>& extensionObject);
    ExtensionObject(const OpcUaVariant& variant);

    void setFromVariant(const OpcUaVariant& variant);
    OpcUaVariant getAsVariant();
    bool isDecoded() const;

    template <typename T>
    inline bool isType() const
    {
        return this->value.content.decoded.type == GetUaDataType<T>();
    }

    template <typename T>
    T* asType()
    {
        return (T*) this->value.content.decoded.data;
    }
};

END_NAMESPACE_OPENDAQ_OPCUA_TMS
