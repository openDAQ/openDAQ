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
#include <opendaq/network_interface.h>
#include <opendaq/module_manager_utils_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class NetworkInterfaceImpl : public ImplementationOf<INetworkInterface>
{
public:
    explicit NetworkInterfaceImpl(const StringPtr& name,
                                  const StringPtr& ownerDeviceManufacturerName,
                                  const StringPtr& ownerDeviceSerialNumber,
                                  const BaseObjectPtr& moduleManager);

    ErrCode INTERFACE_FUNC requestCurrentConfiguration(IPropertyObject** config) override;
    ErrCode INTERFACE_FUNC submitConfiguration(IPropertyObject* config) override;
    ErrCode INTERFACE_FUNC createDefaultConfiguration(IPropertyObject** defaultConfig) override;

private:
    static PropertyObjectPtr createDefaultConfiguration();
    void validate();

    StringPtr interfaceName;
    StringPtr ownerDeviceManufacturerName;
    StringPtr ownerDeviceSerialNumber;
    ModuleManagerUtilsPtr moduleManager;
};

END_NAMESPACE_OPENDAQ
