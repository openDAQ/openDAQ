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
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>

BEGIN_NAMESPACE_OPENDAQ

class PtpSyncInterfaceBaseImpl : public SyncInterfaceBaseImpl<>
{
public:
    using Super = SyncInterfaceBaseImpl<>;

    explicit PtpSyncInterfaceBaseImpl();

protected:
    void createPortProporties(const StringPtr& portName);

    void setProfileOptions(const ListPtr<IString>& options);
    void setTransportProtocolOptions(const ListPtr<IString>& options);
    void setPortModeOptions(const ListPtr<IString>& options);
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

inline void PtpSyncInterfaceBaseImpl::createGeneralProperties()
{
    // Status
    status = this->objPtr.getPropertyValue("Status");
    portsStatus = PropertyObject();
    status.addProperty(ObjectPropertyBuilder("Ports", portsStatus).setReadOnly(true).build());

    // Parameters
    const PropertyObjectPtr parameters = PropertyObject();

    {
        // PTP Configuration
        const auto profileOptions = List<IString>("I558", "802_1AS", "None");
        const auto transportProtocolOptions = List<IString>("IEEE802_3", "UDP_IPV4", "UDP_IPV6");

        configuration = PropertyObject();
        configuration.addProperty(ListPropertyBuilder     ("ProfileOptions",            profileOptions).setReadOnly(true).setVisible(false).build());
        configuration.addProperty(StringPropertyBuilder   ("Profile",                   "None").setSelectionValues(EvalValue("$ProfileOptions")).build());
        configuration.addProperty(BoolProperty            ("TwoStepFlag",               true));
        configuration.addProperty(IntPropertyBuilder      ("DomainNumber",              0).setMinValue(0).build());
        configuration.addProperty(IntPropertyBuilder      ("UtcOffset",                 37).setMinValue(0).build());
        configuration.addProperty(IntPropertyBuilder      ("Priority1",                 128).setMinValue(0).setMaxValue(255).build());
        configuration.addProperty(IntPropertyBuilder      ("Priority2",                 128).setMinValue(0).setMaxValue(255).build());
        configuration.addProperty(ListPropertyBuilder     ("TransportProtocolOptions",  transportProtocolOptions).setReadOnly(true).setVisible(false).build());
        configuration.addProperty(StringPropertyBuilder   ("TransportProtocol",         "IEEE802_3").setSelectionValues(EvalValue("$TransportProtocolOptions")).build());
    
        parameters.addProperty(ObjectProperty("PtpConfiguration", configuration));
    }

    {
        // Ports Configuration
        portsConfiguration = PropertyObject();
        parameters.addProperty(ObjectProperty("Ports", portsConfiguration));
    }

    this->objPtr.addProperty(ObjectProperty("Parameters", parameters));
}

inline void PtpSyncInterfaceBaseImpl::createPortProporties(const StringPtr& portName)
{
    {
        // creating status property
        const PropertyObjectPtr portStatus = PropertyObject();
        portStatus.addProperty(StringPropertyBuilder("State", "Disabled").setReadOnly(true).build());

        portsStatus.addProperty(ObjectPropertyBuilder(portName, portStatus).setReadOnly(true).build());
    }

    {
        // creating configuration property
        const auto modeOptions = List<IString>("Output", "Auto", "Off");
        const auto delayMechanismOptions = List<IString>("E2E", "P2P");

        const PropertyObjectPtr portConfiguration = PropertyObject();
        portConfiguration.addProperty(ListPropertyBuilder   ("ModeOptions",            modeOptions).setReadOnly(true).setVisible(false).build());
        portConfiguration.addProperty(StringPropertyBuilder ("Mode",                   "Off").setSelectionValues(EvalValue("$ModeOptions")).build());
        portConfiguration.addProperty(ListPropertyBuilder   ("DelayMechanismOptions",  delayMechanismOptions).setReadOnly(true).setVisible(false).build());
        portConfiguration.addProperty(StringPropertyBuilder ("DelayMechanism",         "E2E").setSelectionValues(EvalValue("$DelayMechanismOptions")).build());
        portConfiguration.addProperty(IntProperty           ("LogSyncInterval",        0));

        portsConfiguration.addProperty(ObjectProperty(portName, portConfiguration));
    }
}

inline void PtpSyncInterfaceBaseImpl::setProfileOptions(const ListPtr<IString>& options)
{
    configuration.template asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue("ProfileOptions", options);
}

inline void PtpSyncInterfaceBaseImpl::setTransportProtocolOptions(const ListPtr<IString>& options)
{
    configuration.template asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue("TransportProtocolOptions", options);
}

inline void PtpSyncInterfaceBaseImpl::setPortModeOptions(const ListPtr<IString>& options)
{
    for (const auto& portConfig : portsConfiguration.getAllProperties())
    {
        const auto portConfigObj = portConfig.template asPtr<IPropertyObjectProtected>(true);
        portConfigObj.setProtectedPropertyValue("ModeOptions", options);
    }
}

inline void PtpSyncInterfaceBaseImpl::setPortDelayMechanismOptions(const ListPtr<IString>& options)
{
    for (const auto& portConfig : portsConfiguration.getAllProperties())
    {
        const auto portConfigObj = portConfig.template asPtr<IPropertyObjectProtected>(true);
        portConfigObj.setProtectedPropertyValue("DelayMechanismOptions", options);
    }
}

END_NAMESPACE_OPENDAQ
