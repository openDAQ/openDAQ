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
#include <opendaq/device_capability.h>
#include <coretypes/validation.h>
#include <coretypes/intfs.h>
#include <coretypes/string_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class DeviceCapabilityImpl : public ImplementationOf<IDeviceCapability>
{
public:

    explicit DeviceCapabilityImpl(  ProtocolType protocolType, 
                                    ConnectionType connectionType, 
                                    const StringPtr& connectionStringPrefix, 
                                    const StringPtr& host, 
                                    Int port,
                                    const StringPtr& path);

    ErrCode INTERFACE_FUNC getProtocolType(ProtocolType* type) override;
    ErrCode INTERFACE_FUNC getConnectionType(ConnectionType* type) override;
    ErrCode INTERFACE_FUNC getConnectionStringPrefix(IString** connectionStringPrefix) override;
    ErrCode INTERFACE_FUNC getConnectionString(IString** connectionString) override;
    ErrCode INTERFACE_FUNC getHost(IString** host) override;
    ErrCode INTERFACE_FUNC getPort(Int* port) override;
    ErrCode INTERFACE_FUNC getPath(IString** path) override;

private:
    ProtocolType protocolType;
    ConnectionType connectionType;
    StringPtr connectionStringPrefix;
    StringPtr host;
    Int port;
    StringPtr path;
};

END_NAMESPACE_OPENDAQ
