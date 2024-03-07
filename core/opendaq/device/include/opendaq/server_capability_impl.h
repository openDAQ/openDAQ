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
#include <opendaq/server_capability.h>
#include <coretypes/validation.h>
#include <coretypes/intfs.h>
#include <coretypes/string_ptr.h>
#include <coretypes/type_manager_ptr.h>
#include <coretypes/enumeration_type_ptr.h>
#include <coretypes/enumeration_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class ServerCapabilityImpl : public ImplementationOf<IServerCapability>
{
public:

    explicit ServerCapabilityImpl(const StringPtr& connectionString,
                                  const StringPtr& protocolName, 
                                  const StringPtr& protocolType, 
                                  const StringPtr& connectionType);
    
    ErrCode INTERFACE_FUNC getConnectionString(IString** connectionString) override;
    ErrCode INTERFACE_FUNC getProtocolName(IString** protocolName) override;
    ErrCode INTERFACE_FUNC getProtocolType(IEnumeration** type) override;
    ErrCode INTERFACE_FUNC getConnectionType(IString** type) override;

    ErrCode INTERFACE_FUNC setSupportedProtocolType(IString* type) override;
    ErrCode INTERFACE_FUNC setProtocolType(IString* type) override;
    
private:
    static TypeManagerPtr getTypeManager();
    static EnumerationTypePtr GetProtocolTypeEnumeration();

    StringPtr connectionString;
    StringPtr protocolName;
    EnumerationPtr protocolType;
    StringPtr connectionType;
};

END_NAMESPACE_OPENDAQ
