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

#include <opendaq/sync_interface_base_impl.h>
#include <coreobjects/property_object_class_factory.h>
#include <coreobjects/eval_value_factory.h>
#include <coreobjects/property_factory.h>

BEGIN_NAMESPACE_OPENDAQ

namespace PtpSyncPropertyNames
{
    constexpr const char* Mode = "Mode";
    constexpr const char* Status = "Status";
    constexpr const char* StatusPorts = "Ports";
    constexpr const char* StatusPortState = "State";
    constexpr const char* StatusReferenceDomainId = "ReferenceDomainId";
    constexpr const char* StatusSynchronized = "Synchronized";
    constexpr const char* Parameters = "Parameters";
    constexpr const char* ParametersPorts = "Ports";
    constexpr const char* PtpConfiguration = "PtpConfiguration";
    constexpr const char* PtpConfigProfile = "Profile";
    constexpr const char* PtpConfigProfileOptions = "ProfileOptions";
    constexpr const char* PtpConfigTwoStepFlag = "TwoStepFlag";
    constexpr const char* PtpConfigDomainNumber = "DomainNumber";
    constexpr const char* PtpConfigUtcOffset = "UtcOffset";
    constexpr const char* PtpConfigPriority1 = "Priority1";
    constexpr const char* PtpConfigPriority2 = "Priority2";
    constexpr const char* PtpConfigTransportProtocol = "TransportProtocol";
    constexpr const char* PtpConfigTransportProtocolOptions = "TransportProtocolOptions";
    constexpr const char* PortConfigModeOptions = "ModeOptions";
    constexpr const char* PortConfigMode = "Mode";
    constexpr const char* PortConfigDelayMechanism = "DelayMechanism";
    constexpr const char* PortConfigLogSyncInterval = "LogSyncInterval";
    constexpr const char* Ports = "Ports";
}

namespace PtpSyncDefaultValues
{
    constexpr bool TwoStepFlag = true;
    constexpr int DomainNumber = 0;
    constexpr int UtcOffset = 37;
    constexpr int Priority1 = 128;
    constexpr int Priority2 = 128;
    constexpr int LogSyncInterval = 0;
}

/**
 * Base class for PTP-based sync interfaces. Provides the same property structure as PtpSyncInterface
 * (Status, Ports, Configuration, PortsConfiguration) and virtual hooks for configuration changes,
 * but no PtpManager or platform-specific PTP logic. Derived classes (e.g. PtpSyncInterface) add
 * the actual PtpManager and implement the virtuals.
 */
class PtpSyncInterfaceBase : public SyncInterfaceBaseImpl<>
{
public:
    using Super = SyncInterfaceBaseImpl<>;

    explicit PtpSyncInterfaceBase(const StringPtr& name);

    // ISyncInterface
    ErrCode INTERFACE_FUNC getSynced(Bool* synced) override;
    ErrCode INTERFACE_FUNC getReferenceDomainId(IString** referenceDomainId) override;

    // ISyncInterfaceInternal
    ErrCode INTERFACE_FUNC setAsSource(Bool isSource) override;

protected:

    void createGeneralProperties();
    void createPortProporties(const StringPtr& portName);
    void createPortProporties();

    virtual ListPtr<IString> getAvailablePortNames() const;
    virtual DictPtr<IInteger, IString> getAvailableProfiles() const;
    virtual DictPtr<IInteger, IString> getAvailableTransportProtocol() const;
    virtual DictPtr<IInteger, IString> getAvailableDelayMechanism() const;
    virtual DictPtr<IInteger, IString> getAvailablePortModes() const;

    void setMode(SyncMode mode);
    void setReferenceDomainId(const StringPtr& domainId);
    void setSynced(Bool isSynced);
    void setProfileOptions(const DictPtr<IInteger, IString>& options);
    void setTransportProtocolOptions(const DictPtr<IInteger, IString>& options);
    void setPortsModeOptions(const DictPtr<IInteger, IString>& modeOptions);
    void setPortsMode(SyncMode mode);

    PropertyObjectPtr status;
    PropertyObjectPtr portsStatus;
    PropertyObjectPtr configuration;
    PropertyObjectPtr portsConfiguration;

    SyncMode mode{SyncMode::Off};
    StringPtr referenceDomainId{""};
    Bool isSynced{False};
};

END_NAMESPACE_OPENDAQ
