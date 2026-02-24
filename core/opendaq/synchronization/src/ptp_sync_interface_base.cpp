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

#include <opendaq/ptp_sync_interface_base.h>
#include <coreobjects/property_object_class_factory.h>
#include <coreobjects/eval_value_factory.h>
#include <coreobjects/property_factory.h>

BEGIN_NAMESPACE_OPENDAQ

PtpSyncInterfaceBase::PtpSyncInterfaceBase(const StringPtr& name)
    : Super(name)
{
    createGeneralProperties();
}

ErrCode PtpSyncInterfaceBase::getSynced(Bool* synced)
{
    OPENDAQ_PARAM_NOT_NULL(synced);
    auto lock = getRecursiveConfigLock2();
    *synced = isSynced;
    return OPENDAQ_SUCCESS;
}

ErrCode PtpSyncInterfaceBase::getReferenceDomainId(IString** referenceDomainId)
{
    OPENDAQ_PARAM_NOT_NULL(referenceDomainId);
    auto lock = getRecursiveConfigLock2();
    *referenceDomainId = this->referenceDomainId.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

DictPtr<IInteger, IString> PtpSyncInterfaceBase::getAvailableProfiles() const
{
    return Dict<IInteger, IString>({{0, "None"}});
}

DictPtr<IInteger, IString> PtpSyncInterfaceBase::getAvailableTransportProtocol() const
{
    return Dict<IInteger, IString>(
    {
        {0, "IEEE802_3"},
        {1, "UDP_IPV4"},
        {2, "UDP_IPV6"}
    });
}

DictPtr<IInteger, IString> PtpSyncInterfaceBase::getAvailableDelayMechanism() const
{
    return Dict<IInteger, IString>(
    {
        {0, "E2E"},
        {1, "P2P"},
    });
}

DictPtr<IInteger, IString> PtpSyncInterfaceBase::getAvailablePortModes() const
{
    switch (mode)
    {
        case SyncMode::Input:
            return Dict<IInteger, IString>({{2, "Auto"}, {3, "Off"}});
        case SyncMode::Output:
            return Dict<IInteger, IString>({{1, "Output"}, {3, "Off"}});
        case SyncMode::Auto:
            return Dict<IInteger, IString>({{1, "Output"}, {2, "Auto"}, {3, "Off"}});
        case SyncMode::Off:
            return Dict<IInteger, IString>({{3, "Off"}});
    }
    return Dict<IInteger, IString>();
}

ListPtr<IString> PtpSyncInterfaceBase::getAvailablePortNames() const
{
    return List<IString>();
}

void PtpSyncInterfaceBase::createGeneralProperties()
{
    using namespace PtpSyncPropertyNames;
    using namespace PtpSyncDefaultValues;

    status = this->objPtr.getPropertyValue(Status);
    portsStatus = PropertyObject();
    status.addProperty(ObjectPropertyBuilder(StatusPorts, portsStatus).setReadOnly(true).build());

    const PropertyObjectPtr parameters = PropertyObject();

    configuration = PropertyObject();
    configuration.addProperty(DictPropertyBuilder(PtpConfigProfileOptions, getAvailableProfiles()).setReadOnly(true).build());
    configuration.addProperty(SelectionProperty(PtpConfigProfile, EvalValue("$ProfileOptions"), 0));
    configuration.addProperty(BoolProperty(PtpConfigTwoStepFlag, TwoStepFlag));
    configuration.addProperty(IntPropertyBuilder(PtpConfigDomainNumber, DomainNumber).setMinValue(0).build());
    configuration.addProperty(IntPropertyBuilder(PtpConfigUtcOffset, UtcOffset).setMinValue(0).build());
    configuration.addProperty(IntPropertyBuilder(PtpConfigPriority1, Priority1).setMinValue(0).setMaxValue(255).build());
    configuration.addProperty(IntPropertyBuilder(PtpConfigPriority2, Priority2).setMinValue(0).setMaxValue(255).build());
    configuration.addProperty(DictPropertyBuilder(PtpConfigTransportProtocolOptions, getAvailableTransportProtocol()).setReadOnly(true).build());
    configuration.addProperty(SelectionProperty(PtpConfigTransportProtocol, EvalValue("$TransportProtocolOptions"), 0));

    parameters.addProperty(ObjectProperty(PtpConfiguration, configuration));

    portsConfiguration = PropertyObject();
    parameters.addProperty(ObjectProperty(ParametersPorts, portsConfiguration));

    this->objPtr.addProperty(ObjectProperty(Parameters, parameters));
}

void PtpSyncInterfaceBase::createPortProporties(const StringPtr& portName)
{
    using namespace PtpSyncPropertyNames;
    using namespace PtpSyncDefaultValues;
 
    const PropertyObjectPtr status = PropertyObject();
    status.addProperty(StringPropertyBuilder(StatusPortState, "Disabled").setReadOnly(true).build());

    portsStatus.addProperty(ObjectPropertyBuilder(portName, status).setReadOnly(true).build());

    const PropertyObjectPtr configuration = PropertyObject();
    configuration.addProperty(DictPropertyBuilder(PortConfigModeOptions, getAvailablePortModes()).setReadOnly(true).build());
    configuration.addProperty(SelectionProperty      (PortConfigMode,            EvalValue("$ModeOptions"), static_cast<Int>(SyncMode::Off)));
    configuration.addProperty(SparseSelectionProperty(PortConfigDelayMechanism,  getAvailableDelayMechanism(), 0));
    configuration.addProperty(IntProperty            (PortConfigLogSyncInterval, LogSyncInterval));

    portsConfiguration.addProperty(ObjectProperty(portName, configuration));
}

void PtpSyncInterfaceBase::createPortProporties()
{
    using namespace PtpSyncPropertyNames;

    for (const auto& portName : getAvailablePortNames())
    {
        createPortProporties(portName);
    }
}

ErrCode PtpSyncInterfaceBase::setAsSource(Bool isSource)
{
    auto lock = getRecursiveConfigLock2();

    if (isSource)
    {
        setModeOptions(Dict<IInteger, IString>({{0, "Input"}, {2, "Auto"}}));
        setMode(SyncMode::Input);
    }
    else if (mode != SyncMode::Off)
    {
        setModeOptions(Dict<IInteger, IString>({{1, "Output"}, {3, "Off"}}));
        setMode(SyncMode::Output);
    }
    
    return OPENDAQ_SUCCESS;
}

void PtpSyncInterfaceBase::setMode(SyncMode mode)
{
    using namespace PtpSyncPropertyNames;
    this->mode = mode;
    this->objPtr.setPropertyValue(Mode, static_cast<Int>(mode));
}

void PtpSyncInterfaceBase::setReferenceDomainId(const StringPtr& domainId)
{
    using namespace PtpSyncPropertyNames;
    if (this->referenceDomainId != domainId)
    {
        this->referenceDomainId = domainId;
        status.asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue(StatusReferenceDomainId, domainId);
    }
}

void PtpSyncInterfaceBase::setSynced(Bool isSynced)
{
    using namespace PtpSyncPropertyNames;
    if (this->isSynced != isSynced)
    {
        this->isSynced = isSynced;
        status.asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue(StatusSynchronized, isSynced);
    }
}

void PtpSyncInterfaceBase::setProfileOptions(const DictPtr<IInteger, IString>& options)
{
    using namespace PtpSyncPropertyNames;
    configuration.asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue(PtpConfigProfileOptions, options);
}

void PtpSyncInterfaceBase::setTransportProtocolOptions(const DictPtr<IInteger, IString>& options)
{
    using namespace PtpSyncPropertyNames;
    configuration.asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue(PtpConfigTransportProtocolOptions, options);
}

void PtpSyncInterfaceBase::setPortsModeOptions(const DictPtr<IInteger, IString>& modeOptions)
{
    using namespace PtpSyncPropertyNames;
    for (const auto& portName : getAvailablePortNames())
    {
        if (portsConfiguration.hasProperty(portName))
        {
            PropertyObjectPtr portConfig = portsConfiguration.getPropertyValue(portName);
            portConfig.asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue(PortConfigModeOptions, modeOptions);
        }
    }
}

void PtpSyncInterfaceBase::setPortsMode(SyncMode mode)
{
    using namespace PtpSyncPropertyNames;
    for (const auto& portName : getAvailablePortNames())
    {
        if (portsConfiguration.hasProperty(portName))
        {
            PropertyObjectPtr portConfig = portsConfiguration.getPropertyValue(portName);
            portConfig.setPropertyValue(PortConfigMode, static_cast<Int>(mode));
        }
    }
}

END_NAMESPACE_OPENDAQ
