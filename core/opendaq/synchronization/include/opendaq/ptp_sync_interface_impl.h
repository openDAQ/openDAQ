/*
 * Copyright 2022-2026 openDAQ d.o.o.
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

#include <opendaq/sync_interface_base_impl.h>

BEGIN_NAMESPACE_OPENDAQ

enum class PortSyncMode : EnumType
{
    Off = 0,
    Output,
    Auto
};

#ifdef _MSC_VER
    #pragma warning(push)
    // PtpSyncInterfaceBaseImpl's data members don't have dll-interface, but they are never
    // accessed outside of this DLL directly - only through the exported ABI (ISyncInterface, ...) methods.
    #pragma warning(disable : 4251)
    #pragma warning(disable : 4275)
#endif

class PUBLIC_EXPORT PtpSyncInterfaceBaseImpl : public SyncInterfaceBaseImpl
{
public:
    using Super = SyncInterfaceBaseImpl;

    ErrCode INTERFACE_FUNC getClockType(IString** clockType) override;

protected:
    PtpSyncInterfaceBaseImpl(const TypeManagerPtr& manager,
                             const StringPtr& name = "PtpSyncInterface",
                             const std::vector<SyncMode>& availableModes = {SyncMode::Off, SyncMode::Input, SyncMode::Output, SyncMode::Auto});

    void createPortProporties(const StringPtr& portName);

    void setProfileOptions(const ListPtr<IString>& options);
    void setTransportProtocolOptions(const ListPtr<IString>& options);
    void setPortDelayMechanismOptions(const ListPtr<IString>& options);

    void setPortSyncStatus(const StringPtr& portName, SyncRoleStatus status, const StringPtr& message);

    void onConfigurationChanged(const StringPtr& name, const BaseObjectPtr& value) override;

    PropertyObjectPtr portsStatus;
    PropertyObjectPtr ptpConfiguration;
    PropertyObjectPtr portsConfiguration;

private:
    void createGeneralProperties();
    void setPortModeOptions(const DictPtr<IInteger, IString>& options);
    void setPortsMode(PortSyncMode mode);
    void onModeChanged(SyncMode mode);
};

#ifdef _MSC_VER
    #pragma warning(pop)
#endif

END_NAMESPACE_OPENDAQ
