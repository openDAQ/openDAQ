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

BEGIN_NAMESPACE_OPENDAQ

namespace PtpPropertyNames
{
    // Mode property
    constexpr const char* Mode = "Mode";

    // Status properties
    constexpr const char* Status = "Status";
    constexpr const char* StatusPorts = "Ports";
    constexpr const char* StatusPortState = "State";
    constexpr const char* StatusReferenceDomainId = "ReferenceDomainId";
    constexpr const char* StatusSynchronized = "Synchronized";

    // Parameters properties
    constexpr const char* Parameters = "Parameters";
    constexpr const char* ParametersPorts = "Ports";

    // PTP Configuration properties
    constexpr const char* PtpConfiguration = "PtpConfiguration";
    constexpr const char* PtpConfigProfileOptions = "ProfileOptions";
    constexpr const char* PtpConfigProfile = "Profile";
    constexpr const char* PtpConfigTwoStepFlag = "TwoStepFlag";
    constexpr const char* PtpConfigDomainNumber = "DomainNumber";
    constexpr const char* PtpConfigUtcOffset = "UtcOffset";
    constexpr const char* PtpConfigPriority1 = "Priority1";
    constexpr const char* PtpConfigPriority2 = "Priority2";
    constexpr const char* PtpConfigTransportProtocolOptions = "TransportProtocolOptions";
    constexpr const char* PtpConfigTransportProtocol = "TransportProtocol";

    // Port Configuration properties
    constexpr const char* PortConfigModeOptions = "ModeOptions";
    constexpr const char* PortConfigMode = "Mode";
    constexpr const char* PortConfigDelayMechanismOptions = "DelayMechanismOptions";
    constexpr const char* PortConfigDelayMechanism = "DelayMechanism";
    constexpr const char* PortConfigLogSyncInterval = "LogSyncInterval";
}

class PtpSyncInterfaceBaseImpl : public SyncInterfaceBaseImpl<>
{
public:
    using Super = SyncInterfaceBaseImpl<>;

    explicit PtpSyncInterfaceBaseImpl();

    // ISyncInterfaceInternal
    ErrCode INTERFACE_FUNC setAsSource(Bool isSource) override;

protected:
    void createPortProporties(const StringPtr& portName);

    void setProfileOptions(const ListPtr<IString>& options);
    void setTransportProtocolOptions(const ListPtr<IString>& options);
    void setPortModeOptions(const ListPtr<IString>& options);
    void setPortsMode(const StringPtr& mode);
    void setPortDelayMechanismOptions(const ListPtr<IString>& options);

    PropertyObjectPtr status;
    PropertyObjectPtr portsStatus;
    PropertyObjectPtr configuration;
    PropertyObjectPtr portsConfiguration;

private:
    void createGeneralProperties();
};

inline PtpSyncInterfaceBaseImpl::PtpSyncInterfaceBaseImpl()
    : Super("PtpSyncInterface")
{
   createGeneralProperties();
}

inline ErrCode PtpSyncInterfaceBaseImpl::setAsSource(Bool isSource)
{
    auto lock = getRecursiveConfigLock2();

    if (isSource)
    {
        setModeOptions(List<IString>("Input", "Auto"));
        setMode("Input");
    }
    else if (this->objPtr.getPropertyValue(PtpPropertyNames::Mode) != "Off")
    {
        setModeOptions(List<IString>("Output", "Off"));
        setMode("Output");
    }

    return OPENDAQ_SUCCESS;
}

inline void PtpSyncInterfaceBaseImpl::createGeneralProperties()
{
    // Status
    status = this->objPtr.getPropertyValue(PtpPropertyNames::Status);
    portsStatus = PropertyObject();
    status.addProperty(ObjectPropertyBuilder(PtpPropertyNames::StatusPorts, portsStatus).setReadOnly(true).build());

    // Parameters
    const PropertyObjectPtr parameters = PropertyObject();
    this->objPtr.addProperty(ObjectProperty(PtpPropertyNames::Parameters, parameters));

    {
        // PTP Configuration
        configuration = PropertyObject();
        parameters.addProperty(ObjectProperty(PtpPropertyNames::PtpConfiguration, configuration));

        const auto profileOptions = List<IString>("I558", "802_1AS", "None");
        const auto transportProtocolOptions = List<IString>("IEEE802_3", "UDP_IPV4", "UDP_IPV6");

        configuration.addProperty(ListPropertyBuilder     (PtpPropertyNames::PtpConfigProfileOptions,     profileOptions).setReadOnly(true).setVisible(false).build());
        configuration.addProperty(StringPropertyBuilder   (PtpPropertyNames::PtpConfigProfile,            "None").setSelectionValues(EvalValue("$ProfileOptions")).build());
        configuration.addProperty(BoolProperty            (PtpPropertyNames::PtpConfigTwoStepFlag,        true));
        configuration.addProperty(IntPropertyBuilder      (PtpPropertyNames::PtpConfigDomainNumber,       0).setMinValue(0).build());
        configuration.addProperty(IntPropertyBuilder      (PtpPropertyNames::PtpConfigUtcOffset,          37).setMinValue(0).build());
        configuration.addProperty(IntPropertyBuilder      (PtpPropertyNames::PtpConfigPriority1,          128).setMinValue(0).setMaxValue(255).build());
        configuration.addProperty(IntPropertyBuilder      (PtpPropertyNames::PtpConfigPriority2,          128).setMinValue(0).setMaxValue(255).build());
        configuration.addProperty(ListPropertyBuilder     (PtpPropertyNames::PtpConfigTransportProtocolOptions, transportProtocolOptions).setReadOnly(true).setVisible(false).build());
        configuration.addProperty(StringPropertyBuilder   (PtpPropertyNames::PtpConfigTransportProtocol, "IEEE802_3").setSelectionValues(EvalValue("$TransportProtocolOptions")).build());

        configuration.setPropertyOrder(List<IString>(PtpPropertyNames::PtpConfigProfileOptions, PtpPropertyNames::PtpConfigTransportProtocolOptions));
    }

    {
        // Ports Configuration
        portsConfiguration = PropertyObject();
        parameters.addProperty(ObjectProperty(PtpPropertyNames::ParametersPorts, portsConfiguration));
    }
}

inline void PtpSyncInterfaceBaseImpl::createPortProporties(const StringPtr& portName)
{
    {
        // creating status property
        const PropertyObjectPtr portStatus = PropertyObject();
        portStatus.addProperty(StringPropertyBuilder(PtpPropertyNames::StatusPortState, "Disabled").setReadOnly(true).build());

        portsStatus.addProperty(ObjectPropertyBuilder(portName, portStatus).setReadOnly(true).build());
    }

    {
        // creating configuration property
        const auto modeOptions = List<IString>("Output", "Auto", "Off");
        const auto delayMechanismOptions = List<IString>("E2E", "P2P");

        const PropertyObjectPtr portConfiguration = PropertyObject();
        portConfiguration.addProperty(ListPropertyBuilder   (PtpPropertyNames::PortConfigModeOptions,       modeOptions).setReadOnly(true).setVisible(false).build());
        portConfiguration.addProperty(StringPropertyBuilder (PtpPropertyNames::PortConfigMode,              "Off").setSelectionValues(EvalValue("$ModeOptions")).build());
        portConfiguration.addProperty(ListPropertyBuilder   (PtpPropertyNames::PortConfigDelayMechanismOptions, delayMechanismOptions).setReadOnly(true).setVisible(false).build());
        portConfiguration.addProperty(StringPropertyBuilder (PtpPropertyNames::PortConfigDelayMechanism,    "E2E").setSelectionValues(EvalValue("$DelayMechanismOptions")).build());
        portConfiguration.addProperty(IntProperty           (PtpPropertyNames::PortConfigLogSyncInterval,   0));

        portConfiguration.setPropertyOrder(List<IString>(PtpPropertyNames::PortConfigModeOptions, PtpPropertyNames::PortConfigDelayMechanismOptions));

        portsConfiguration.addProperty(ObjectProperty(portName, portConfiguration));
    }
}

inline void PtpSyncInterfaceBaseImpl::setProfileOptions(const ListPtr<IString>& options)
{
    configuration.template asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue(PtpPropertyNames::PtpConfigProfileOptions, options);
}

inline void PtpSyncInterfaceBaseImpl::setTransportProtocolOptions(const ListPtr<IString>& options)
{
    configuration.template asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue(PtpPropertyNames::PtpConfigTransportProtocolOptions, options);
}

inline void PtpSyncInterfaceBaseImpl::setPortModeOptions(const ListPtr<IString>& options)
{
    for (const auto& portProperty : portsConfiguration.getAllProperties())
    {
        const PropertyObjectProtectedPtr portConfig = portsConfiguration.getPropertyValue(portProperty.getName());
        portConfig.setProtectedPropertyValue(PtpPropertyNames::PortConfigModeOptions, options);
    }
}

inline void PtpSyncInterfaceBaseImpl::setPortsMode(const StringPtr& mode)
{
    for (const auto& portProperty : portsConfiguration.getAllProperties())
    {
        const PropertyObjectPtr portConfig = portsConfiguration.getPropertyValue(portProperty.getName());
        portConfig.setPropertyValue(PtpPropertyNames::PortConfigMode, mode);
    }
}

inline void PtpSyncInterfaceBaseImpl::setPortDelayMechanismOptions(const ListPtr<IString>& options)
{
    for (const auto& portProperty : portsConfiguration.getAllProperties())
    {
        const PropertyObjectProtectedPtr portConfig = portsConfiguration.getPropertyValue(portProperty.getName());
        portConfig.setProtectedPropertyValue(PtpPropertyNames::PortConfigDelayMechanismOptions, options);
    }
}

END_NAMESPACE_OPENDAQ
