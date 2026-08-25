#include <opendaq/ptp_sync_interface_impl.h>
#include <opendaq/component_status_container_private_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

namespace PtpPropertyNames
{
    // Status properties
    constexpr const char* StatusPorts = "Ports";
    constexpr const char* StatusPortState = "State";
    constexpr const char* StatusReferenceDomainId = "ReferenceDomainId";
    constexpr const char* StatusSynchronized = "Synchronized";

    // PTP Configuration properties
    constexpr const char* PtpConfiguration = "PtpConfiguration";
    constexpr const char* PtpConfigMode = "Mode";
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

PtpSyncInterfaceBaseImpl::PtpSyncInterfaceBaseImpl(const TypeManagerPtr& manager, 
                                                   const StringPtr& name,
                                                   const std::vector<SyncMode>& availableModes)
    : Super(manager, name, availableModes)
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

        const auto profileOptions = List<IString>("None");
        const auto transportProtocolOptions = List<IString>("IEEE802_3", "UDP_IPV4", "UDP_IPV6");

        ptpConfiguration.addProperty(ListPropertyBuilder     (PtpPropertyNames::PtpConfigProfileOptions, profileOptions).setReadOnly(true).setVisible(false).build());
        ptpConfiguration.addProperty(ListPropertyBuilder     (PtpPropertyNames::PtpConfigTransportProtocolOptions, transportProtocolOptions).setReadOnly(true).setVisible(false).build());

        ptpConfiguration.addProperty(StringPropertyBuilder   (PtpPropertyNames::PtpConfigProfile,           "None").setSelectionValues(EvalValue("$ProfileOptions")).build());
        ptpConfiguration.addProperty(BoolProperty            (PtpPropertyNames::PtpConfigTwoStepFlag,       true));
        ptpConfiguration.addProperty(IntPropertyBuilder      (PtpPropertyNames::PtpConfigDomainNumber,      0).setMinValue(0).build());
        ptpConfiguration.addProperty(IntPropertyBuilder      (PtpPropertyNames::PtpConfigUtcOffset,         37).setMinValue(0).build());
        ptpConfiguration.addProperty(IntPropertyBuilder      (PtpPropertyNames::PtpConfigPriority1,         128).setMinValue(0).setMaxValue(255).build());
        ptpConfiguration.addProperty(IntPropertyBuilder      (PtpPropertyNames::PtpConfigPriority2,         128).setMinValue(0).setMaxValue(255).build());
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
        const EnumerationTypePtr syncRoleStatusType = manager.getType("SynchronizationRoleStatusType");
        portStatus.addProperty(SelectionPropertyBuilder(PtpPropertyNames::StatusPortState, syncRoleStatusType.getEnumeratorNames(), static_cast<Int>(SyncRoleStatus::Off)).setReadOnly(true).build());

        portsStatus.addProperty(ObjectPropertyBuilder(portName, portStatus).setReadOnly(true).build());

        const auto syncRoleStatus = EnumerationWithIntValueAndType(syncRoleStatusType, static_cast<Int>(SyncRoleStatus::Off));
        const auto statusContainerPrivate = this->statusContainer.asPtr<IComponentStatusContainerPrivate>(true);
        statusContainerPrivate.addStatus(portName, syncRoleStatus);
    }

    {
        // creating configuration property
        const auto modeOptions = Dict<IInteger, IString>({
            {static_cast<Int>(PortSyncMode::Off),    "Off"},
            {static_cast<Int>(PortSyncMode::Output), "Output"},
            {static_cast<Int>(PortSyncMode::Auto),   "Auto"}
        });
        const auto delayMechanismOptions = List<IString>("E2E", "P2P");

        const PropertyObjectPtr portConfiguration = PropertyObject();
        portConfiguration.addProperty(DictPropertyBuilder    (PtpPropertyNames::PortConfigModeOptions, modeOptions).setReadOnly(true).setVisible(false).build());
        portConfiguration.addProperty(SparseSelectionProperty(PtpPropertyNames::PortConfigMode, EvalValue("$ModeOptions"), static_cast<Int>(PortSyncMode::Off)));
        portConfiguration.addProperty(ListPropertyBuilder    (PtpPropertyNames::PortConfigDelayMechanismOptions, delayMechanismOptions).setReadOnly(true).setVisible(false).build());
        portConfiguration.addProperty(StringPropertyBuilder  (PtpPropertyNames::PortConfigDelayMechanism, "E2E").setSelectionValues(EvalValue("$DelayMechanismOptions")).build());
        portConfiguration.addProperty(IntProperty            (PtpPropertyNames::PortConfigLogSyncInterval, 0));

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

void PtpSyncInterfaceBaseImpl::setPortDelayMechanismOptions(const ListPtr<IString>& options)
{
    for (const auto& portProperty : portsConfiguration.getAllProperties())
    {
        const PropertyObjectProtectedPtr portConfig = portsConfiguration.getPropertyValue(portProperty.getName());
        portConfig.setProtectedPropertyValue(PtpPropertyNames::PortConfigDelayMechanismOptions, options);
    }
}

void PtpSyncInterfaceBaseImpl::setPortSyncStatus(const StringPtr& portName, SyncRoleStatus status, const StringPtr& message)
{
    PropertyObjectPtr portStatus = portsStatus.getPropertyValue(portName);
    portStatus.asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue(PtpPropertyNames::StatusPortState, static_cast<Int>(status));

    const auto syncRoleStatus =
        EnumerationWithIntValue("SynchronizationRoleStatusType", static_cast<Int>(status), manager);

    const auto statusContainerPrivate = this->statusContainer.asPtr<IComponentStatusContainerPrivate>(true);
    statusContainerPrivate.setStatusWithMessage(portName, syncRoleStatus, message);
}

void PtpSyncInterfaceBaseImpl::onConfigurationChanged(const StringPtr& name, const BaseObjectPtr& value)
{
    if (name == PtpPropertyNames::PtpConfigMode)
    {
        Int intMode = 0;
        checkErrorInfo(value.asPtr<IInteger>(true)->getValue(&intMode));
        onModeChanged(static_cast<SyncMode>(intMode));
    }
}

void PtpSyncInterfaceBaseImpl::onModeChanged(SyncMode mode)
{
    auto portsMode = PortSyncMode::Off;
    auto portModeOptions = Dict<IInteger, IString>({{static_cast<Int>(PortSyncMode::Off), "Off"}});

    switch (mode)
    {
        case SyncMode::Off:
            // save state
            return;
        case SyncMode::Input:
            portModeOptions.set(static_cast<Int>(PortSyncMode::Auto), "Auto");
            portsMode = PortSyncMode::Auto;
            break;
        case SyncMode::Output:
            portModeOptions.set(static_cast<Int>(PortSyncMode::Output), "Output");
            portsMode = PortSyncMode::Output;
            break;
         case SyncMode::Auto:
            portModeOptions.set(static_cast<Int>(PortSyncMode::Output), "Output");
            portModeOptions.set(static_cast<Int>(PortSyncMode::Auto), "Auto");
            portsMode = PortSyncMode::Auto;
            break;
    };

    setPortModeOptions(portModeOptions);
    setPortsMode(portsMode);
}

void PtpSyncInterfaceBaseImpl::setPortModeOptions(const DictPtr<IInteger, IString>& options)
{
    for (const auto& portProperty : portsConfiguration.getAllProperties())
    {
        const PropertyObjectProtectedPtr portConfig = portsConfiguration.getPropertyValue(portProperty.getName());
        portConfig.setProtectedPropertyValue(PtpPropertyNames::PortConfigModeOptions, options);
    }
}

void PtpSyncInterfaceBaseImpl::setPortsMode(PortSyncMode mode)
{
    const auto intMode = Integer(static_cast<Int>(mode));
    for (const auto& portProperty : portsConfiguration.getAllProperties())
    {
        const PropertyObjectPtr portConfig = portsConfiguration.getPropertyValue(portProperty.getName());
        Int currentMode = portConfig.getPropertyValue(PtpPropertyNames::PortConfigMode);
        if (static_cast<PortSyncMode>(currentMode) != PortSyncMode::Off)
            portConfig.setPropertyValue(PtpPropertyNames::PortConfigMode, intMode);
    }
}

END_NAMESPACE_OPENDAQ
