#include <opendaq/ptp_sync_interface_impl.h>

BEGIN_NAMESPACE_OPENDAQ

namespace PtpPropertyNames
{
    // Mode property
    constexpr const char* Mode = "Mode";

    // Status properties
    constexpr const char* StatusPorts = "Ports";
    constexpr const char* StatusPortState = "State";
    constexpr const char* StatusReferenceDomainId = "ReferenceDomainId";
    constexpr const char* StatusSynchronized = "Synchronized";

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
    constexpr const char* PortConfiguration = "PortConfiguration";
    constexpr const char* PortConfigModeOptions = "ModeOptions";
    constexpr const char* PortConfigMode = "Mode";
    constexpr const char* PortConfigDelayMechanismOptions = "DelayMechanismOptions";
    constexpr const char* PortConfigDelayMechanism = "DelayMechanism";
    constexpr const char* PortConfigLogSyncInterval = "LogSyncInterval";
}

PtpSyncInterfaceBaseImpl::PtpSyncInterfaceBaseImpl()
    : Super("PtpSyncInterface", {SyncMode::Off, SyncMode::Input, SyncMode::Output, SyncMode::Auto})
{
    createGeneralProperties();
}

void PtpSyncInterfaceBaseImpl::createGeneralProperties()
{
    // Status
    portsStatus = PropertyObject();
    status.addProperty(ObjectPropertyBuilder(PtpPropertyNames::StatusPorts, portsStatus).setReadOnly(true).build());

    {
        // PTP Configuration
        ptpConfiguration = PropertyObject();
        configuration.addProperty(ObjectProperty(PtpPropertyNames::PtpConfiguration, ptpConfiguration));

        const auto profileOptions = List<IString>("I558", "802_1AS", "None");
        const auto transportProtocolOptions = List<IString>("IEEE802_3", "UDP_IPV4", "UDP_IPV6");

        ptpConfiguration.addProperty(ListPropertyBuilder     (PtpPropertyNames::PtpConfigProfileOptions,     profileOptions).setReadOnly(true).setVisible(false).build());
        ptpConfiguration.addProperty(StringPropertyBuilder   (PtpPropertyNames::PtpConfigProfile,            "None").setSelectionValues(EvalValue("$ProfileOptions")).build());
        ptpConfiguration.addProperty(BoolProperty            (PtpPropertyNames::PtpConfigTwoStepFlag,        true));
        ptpConfiguration.addProperty(IntPropertyBuilder      (PtpPropertyNames::PtpConfigDomainNumber,       0).setMinValue(0).build());
        ptpConfiguration.addProperty(IntPropertyBuilder      (PtpPropertyNames::PtpConfigUtcOffset,          37).setMinValue(0).build());
        ptpConfiguration.addProperty(IntPropertyBuilder      (PtpPropertyNames::PtpConfigPriority1,          128).setMinValue(0).setMaxValue(255).build());
        ptpConfiguration.addProperty(IntPropertyBuilder      (PtpPropertyNames::PtpConfigPriority2,          128).setMinValue(0).setMaxValue(255).build());
        ptpConfiguration.addProperty(ListPropertyBuilder     (PtpPropertyNames::PtpConfigTransportProtocolOptions, transportProtocolOptions).setReadOnly(true).setVisible(false).build());
        ptpConfiguration.addProperty(StringPropertyBuilder   (PtpPropertyNames::PtpConfigTransportProtocol, "IEEE802_3").setSelectionValues(EvalValue("$TransportProtocolOptions")).build());

        ptpConfiguration.setPropertyOrder(List<IString>(PtpPropertyNames::PtpConfigProfileOptions, PtpPropertyNames::PtpConfigTransportProtocolOptions));
    }

    {
        // Ports Configuration
        portsConfiguration = PropertyObject();
        configuration.addProperty(ObjectProperty(PtpPropertyNames::PortConfiguration, portsConfiguration));
    }
}

void PtpSyncInterfaceBaseImpl::createPortProporties(const StringPtr& portName)
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

void PtpSyncInterfaceBaseImpl::setProfileOptions(const ListPtr<IString>& options)
{
    ptpConfiguration.template asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue(PtpPropertyNames::PtpConfigProfileOptions, options);
}

void PtpSyncInterfaceBaseImpl::setTransportProtocolOptions(const ListPtr<IString>& options)
{
    ptpConfiguration.template asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue(PtpPropertyNames::PtpConfigTransportProtocolOptions, options);
}

void PtpSyncInterfaceBaseImpl::setPortModeOptions(const ListPtr<IString>& options)
{
    for (const auto& portProperty : portsConfiguration.getAllProperties())
    {
        const PropertyObjectProtectedPtr portConfig = portsConfiguration.getPropertyValue(portProperty.getName());
        portConfig.setProtectedPropertyValue(PtpPropertyNames::PortConfigModeOptions, options);
    }
}

void PtpSyncInterfaceBaseImpl::setPortsMode(const StringPtr& mode)
{
    for (const auto& portProperty : portsConfiguration.getAllProperties())
    {
        const PropertyObjectPtr portConfig = portsConfiguration.getPropertyValue(portProperty.getName());
        portConfig.setPropertyValue(PtpPropertyNames::PortConfigMode, mode);
    }
}

void PtpSyncInterfaceBaseImpl::setPortDelayMechanismOptions(const ListPtr<IString>& options)
{
    for (const auto& portProperty : portsConfiguration.getAllProperties())
    {
        const PropertyObjectProtectedPtr portConfig = portsConfiguration.getPropertyValue(portProperty.getName());
        portConfig.setProtectedPropertyValue(PtpPropertyNames::PortConfigDelayMechanismOptions, options);
    }
}

END_NAMESPACE_OPENDAQ
