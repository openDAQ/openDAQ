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
#include <opendaq/server_capability_config.h>
#include <coretypes/validation.h>
#include <coretypes/intfs.h>
#include <coretypes/string_ptr.h>
#include <coretypes/type_manager_ptr.h>
#include <coretypes/enumeration_type_ptr.h>
#include <coretypes/enumeration_ptr.h>
#include <coreobjects/property_object_impl.h>
#include <opendaq/context_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class ServerCapabilityConfigImpl : public GenericPropertyObjectImpl<IServerCapabilityConfig>
{
public:
    using Super = GenericPropertyObjectImpl<IServerCapabilityConfig>;

    explicit ServerCapabilityConfigImpl(const StringPtr& protocolName, ProtocolType protocolType);
    
    explicit ServerCapabilityConfigImpl(const StringPtr& protocolId);

    ErrCode INTERFACE_FUNC getPrimaryConnectionString(IString** connectionString) override;
    ErrCode INTERFACE_FUNC setPrimaryConnectionString(IString* connectionString) override;

    ErrCode INTERFACE_FUNC getConnectionStrings(IList** connectionStrings) override;
    ErrCode INTERFACE_FUNC addConnectionString(IString* connectionString) override;
        
    ErrCode INTERFACE_FUNC getProtocolName(IString** protocolName) override;
    ErrCode INTERFACE_FUNC setProtocolName(IString* protocolName) override;
    
    ErrCode INTERFACE_FUNC getProtocolType(ProtocolType* type) override;
    ErrCode INTERFACE_FUNC setProtocolType(ProtocolType type) override;
    
    ErrCode INTERFACE_FUNC getConnectionType(IString** type) override;
    ErrCode INTERFACE_FUNC setConnectionType(IString* type) override;
    
    ErrCode INTERFACE_FUNC getCoreEventsEnabled(Bool* enabled) override;
    ErrCode INTERFACE_FUNC setCoreEventsEnabled(Bool enabled) override;    
    
private:
    template <typename T>
    typename InterfaceToSmartPtr<T>::SmartPtr getTypedProperty(const StringPtr& name);

    static StringPtr ProtocolTypeToString(ProtocolType type);
    static ProtocolType StringToProtocolType(const StringPtr& type);

    TypeManagerPtr typeManager;
    EnumerationPtr protocolType;
};

END_NAMESPACE_OPENDAQ
