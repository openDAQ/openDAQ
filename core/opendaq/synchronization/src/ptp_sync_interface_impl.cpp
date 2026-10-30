#include <opendaq/ptp_sync_interface_impl.h>
#include <opendaq/component_status_container_private_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

namespace PtpPropertyNames
{
    // PTP Configuration properties
    constexpr const char* PtpConfigProfileSelection = "ProfileSelection";
    constexpr const char* PtpConfigTransportProtocolSelection = "TransportProtocolSelection";
    constexpr const char* PtpConfigProfile = "Profile";
    constexpr const char* PtpConfigTwoStepFlag = "TwoStepFlag";
    constexpr const char* PtpConfigDomainNumber = "DomainNumber";
    constexpr const char* PtpConfigUtcOffset = "UtcOffset";
    constexpr const char* PtpConfigPriority1 = "Priority1";
    constexpr const char* PtpConfigPriority2 = "Priority2";
    constexpr const char* PtpConfigTransportProtocol = "TransportProtocol";

    // Port Configuration properties
    constexpr const char* PortConfiguration = "PortConfiguration";
    constexpr const char* PortConfigModeSelection = "ModeSelection";
    constexpr const char* PortConfigMode = "Mode";
    constexpr const char* PortConfigDelayMechanismSelection = "DelayMechanismSelection";
    constexpr const char* PortConfigDelayMechanism = "DelayMechanism";
    constexpr const char* PortConfigLogSyncInterval = "LogSyncInterval";
}

PtpSyncInterfaceBaseImpl::PtpSyncInterfaceBaseImpl(const TypeManagerPtr& manager,
                                                   const StringPtr& name,
                                                   const std::vector<SyncMode>& availableModes)
    : Super(manager, name, availableModes, createConfig())
    , portsConfiguration(config.getPropertyValue(PtpPropertyNames::PortConfiguration))
{
}

ErrCode PtpSyncInterfaceBaseImpl::getSyncType(IString** syncType)
{
    OPENDAQ_PARAM_NOT_NULL(syncType);
    *syncType = String("ptp").detach();
    return OPENDAQ_SUCCESS;
}

PropertyObjectPtr PtpSyncInterfaceBaseImpl::createConfig()
{
    const auto config = PropertyObject();
    config.addProperty(ListPropertyBuilder  (PtpPropertyNames::PtpConfigProfileSelection, List<IString>("None")).setReadOnly(true).setVisible(false).build());
    config.addProperty(ListPropertyBuilder  (PtpPropertyNames::PtpConfigTransportProtocolSelection, List<IString>("IEEE802_3", "UDP_IPV4", "UDP_IPV6")).setReadOnly(true).setVisible(false).build());

    config.addProperty(StringPropertyBuilder(PtpPropertyNames::PtpConfigProfile,           "None").setSelectionValues(EvalValue("$ProfileSelection")).build());
    config.addProperty(BoolProperty         (PtpPropertyNames::PtpConfigTwoStepFlag,       true));
    config.addProperty(IntPropertyBuilder   (PtpPropertyNames::PtpConfigDomainNumber,      0).setMinValue(0).setMaxValue(255).build());
    config.addProperty(IntPropertyBuilder   (PtpPropertyNames::PtpConfigUtcOffset,         37).setMinValue(0).setMaxValue(32767).build());
    config.addProperty(IntPropertyBuilder   (PtpPropertyNames::PtpConfigPriority1,         128).setMinValue(0).setMaxValue(255).build());
    config.addProperty(IntPropertyBuilder   (PtpPropertyNames::PtpConfigPriority2,         128).setMinValue(0).setMaxValue(255).build());
    config.addProperty(StringPropertyBuilder(PtpPropertyNames::PtpConfigTransportProtocol, "IEEE802_3").setSelectionValues(EvalValue("$TransportProtocolSelection")).build());
    const auto ports = PropertyObject();
    ports.getPermissionManager().setPermissions(PermissionsBuilder().inherit(true).build());
    config.addProperty(ObjectProperty       (PtpPropertyNames::PortConfiguration,          ports));
    config.setPropertyOrder({PtpPropertyNames::PtpConfigProfileSelection, PtpPropertyNames::PtpConfigTransportProtocolSelection});
    return config;
}

void PtpSyncInterfaceBaseImpl::createPortProperties(const StringPtr& portName)
{
    {
        // creating status container entry
        const EnumerationTypePtr syncRoleStatusType = manager.getType("SyncRoleStatusType");
        const auto syncRoleStatus = EnumerationWithIntValueAndType(syncRoleStatusType, static_cast<Int>(SyncRoleStatus::Off));
        const auto statusContainerPrivate = this->statusContainer.asPtr<IComponentStatusContainerPrivate>(true);
        statusContainerPrivate.addStatus(portName, syncRoleStatus);
    }

    {
        // creating configuration property
        const PropertyObjectPtr portConfiguration = PropertyObject();
        portConfiguration.getPermissionManager().setPermissions(PermissionsBuilder().inherit(true).build());
        portConfiguration.addProperty(ListPropertyBuilder    (PtpPropertyNames::PortConfigModeSelection, List<IString>("Off", "Output", "Auto")).setReadOnly(true).setVisible(false).build());
        portConfiguration.addProperty(ListPropertyBuilder    (PtpPropertyNames::PortConfigDelayMechanismSelection, List<IString>("E2E", "P2P")).setReadOnly(true).setVisible(false).build());
        portConfiguration.addProperty(StringPropertyBuilder  (PtpPropertyNames::PortConfigMode, "Off").setSelectionValues(EvalValue("$ModeSelection")).build());
        portConfiguration.addProperty(StringPropertyBuilder  (PtpPropertyNames::PortConfigDelayMechanism, "E2E").setSelectionValues(EvalValue("$DelayMechanismSelection")).build());
        portConfiguration.addProperty(IntPropertyBuilder     (PtpPropertyNames::PortConfigLogSyncInterval, 0).setMinValue(-128).setMaxValue(127).build());

        portConfiguration.setPropertyOrder({PtpPropertyNames::PortConfigModeSelection, PtpPropertyNames::PortConfigDelayMechanismSelection});

        portsConfiguration.addProperty(ObjectProperty(portName, portConfiguration));
    }
}

void PtpSyncInterfaceBaseImpl::setProfileSelection(const ListPtr<IString>& selection)
{
    config.template asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue(PtpPropertyNames::PtpConfigProfileSelection, selection);
}

void PtpSyncInterfaceBaseImpl::setTransportProtocolSelection(const ListPtr<IString>& selection)
{
    config.template asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue(PtpPropertyNames::PtpConfigTransportProtocolSelection, selection);
}

void PtpSyncInterfaceBaseImpl::setPortDelayMechanismSelection(const ListPtr<IString>& selection)
{
    for (const auto& portProperty : portsConfiguration.getAllProperties())
    {
        const PropertyObjectProtectedPtr portConfig = portsConfiguration.getPropertyValue(portProperty.getName());
        portConfig.setProtectedPropertyValue(PtpPropertyNames::PortConfigDelayMechanismSelection, selection);
    }
}

void PtpSyncInterfaceBaseImpl::setPortSyncStatus(const StringPtr& portName, SyncRoleStatus status, const StringPtr& message)
{
    const auto syncRoleStatus =
        EnumerationWithIntValue("SyncRoleStatusType", static_cast<Int>(status), manager);

    const auto statusContainerPrivate = this->statusContainer.asPtr<IComponentStatusContainerPrivate>(true);
    statusContainerPrivate.setStatusWithMessage(portName, syncRoleStatus, message);
}

void PtpSyncInterfaceBaseImpl::onModeChanged(SyncMode mode)
{
    StringPtr portsMode = "Off";
    auto portModeSelection = List<IString>("Off");

    switch (mode)
    {
        case SyncMode::Off:
            // save state
            return;
        case SyncMode::Input:
            portModeSelection.pushBack("Auto");
            portsMode = "Auto";
            break;
        case SyncMode::Output:
            portModeSelection.pushBack("Output");
            portsMode = "Output";
            break;
         case SyncMode::Auto:
            portModeSelection.pushBack("Output");
            portModeSelection.pushBack("Auto");
            portsMode = "Auto";
            break;
    };

    setPortModeSelection(portModeSelection);
    setPortsMode(portsMode);
}

void PtpSyncInterfaceBaseImpl::setPortModeSelection(const ListPtr<IString>& selection)
{
    for (const auto& portProperty : portsConfiguration.getAllProperties())
    {
        const PropertyObjectProtectedPtr portConfig = portsConfiguration.getPropertyValue(portProperty.getName());
        portConfig.setProtectedPropertyValue(PtpPropertyNames::PortConfigModeSelection, selection);
    }
}

void PtpSyncInterfaceBaseImpl::setPortsMode(const StringPtr& mode)
{
    for (const auto& portProperty : portsConfiguration.getAllProperties())
    {
        const PropertyObjectPtr portConfig = portsConfiguration.getPropertyValue(portProperty.getName());
        const StringPtr currentMode = portConfig.getPropertyValue(PtpPropertyNames::PortConfigMode);
        if (currentMode != "Off")
            portConfig.setPropertyValue(PtpPropertyNames::PortConfigMode, mode);
    }
}

END_NAMESPACE_OPENDAQ
