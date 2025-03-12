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
#include <opendaq/address_info.h>
#include <opendaq/connected_client_info.h>
#include <coreobjects/property_object_impl.h>
#include <opendaq/context_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class ConnectedClientInfoImpl : public GenericPropertyObjectImpl<IConnectedClientInfo>
{
public:
    using Super = GenericPropertyObjectImpl<IConnectedClientInfo>;

    explicit ConnectedClientInfoImpl();
    explicit ConnectedClientInfoImpl(const StringPtr& url,
                                     ProtocolType protocolType,
                                     const StringPtr& protocolName,
                                     const StringPtr& clientType,
                                     const StringPtr& hostName);

    ErrCode INTERFACE_FUNC getUrl(IString** url) override;
    ErrCode INTERFACE_FUNC getProtocolType(ProtocolType* type) override;
    ErrCode INTERFACE_FUNC getProtocolName(IString** protocolName) override;
    ErrCode INTERFACE_FUNC getClientTypeName(IString** type) override;
    ErrCode INTERFACE_FUNC getHostName(IString** hostName) override;

    ErrCode INTERFACE_FUNC getInterfaceIds(SizeT* idCount, IntfID** ids) override;

    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;
    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

    ErrCode INTERFACE_FUNC clone(IPropertyObject** cloned) override;

private:
    template <typename T>
    typename InterfaceToSmartPtr<T>::SmartPtr getTypedProperty(const StringPtr& name);

    static StringPtr ProtocolTypeToString(ProtocolType type);
    static ProtocolType StringToProtocolType(const StringPtr& type);
};

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(ConnectedClientInfoImpl)

END_NAMESPACE_OPENDAQ
