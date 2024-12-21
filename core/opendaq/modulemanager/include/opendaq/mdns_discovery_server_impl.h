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
#include <opendaq/discovery_server.h>
#include <discovery_server/mdnsdiscovery_server.h>
#include <opendaq/logger_ptr.h>
#include <coreobjects/property_object_ptr.h>
#include <opendaq/device_info_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class MdnsDiscoveryServerImpl : public ImplementationOf<IDiscoveryServer>
{
public:
    MdnsDiscoveryServerImpl(const LoggerPtr& logger,
                            const ListPtr<IString>& netInterfaceNames,
                            const ProcedurePtr& modifyIpConfigCallback,
                            const FunctionPtr& retrieveIpConfigCallback);

    ErrCode INTERFACE_FUNC registerService(IString* id, IPropertyObject* config, IDeviceInfo* deviceInfo) override;
    ErrCode INTERFACE_FUNC unregisterService(IString* id) override;

private:
    void registerIpModificationService(const DeviceInfoPtr& deviceInfo);
    static bool verifyIpModificationServiceParameters(const ListPtr<IString>& netInterfaceNames,
                                                      const ProcedurePtr& modifyIpConfigCallback,
                                                      const FunctionPtr& retrieveIpConfigCallback);
    static PropertyObjectPtr populateIpConfigProps(const discovery_server::TxtProperties& txtProps);
    static ListPtr<IString> populateAddresses(const std::string& addressesString);

    discovery_server::MDNSDiscoveryServer discoveryServer;
    LoggerComponentPtr loggerComponent;

    bool ipModificationEnabled{false};
    ListPtr<IString> netInterfaceNames;
    ProcedurePtr modifyIpConfigCallback;
    FunctionPtr retrieveIpConfigCallback;
};

END_NAMESPACE_OPENDAQ
