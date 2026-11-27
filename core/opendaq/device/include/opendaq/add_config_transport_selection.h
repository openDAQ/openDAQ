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
#include <coreobjects/property_factory.h>
#include <coreobjects/property_object_ptr.h>
#include <coreobjects/property_object_protected_ptr.h>
#include <coretypes/exceptions.h>
#include <opendaq/container_utils.h>
#include <opendaq/transport_option_factory.h>
#include <opendaq/transport_option_ptr.h>
#include <opendaq/transport_options_object.h>

BEGIN_NAMESPACE_OPENDAQ

// The transport selection that AddDeviceConfigImpl and AddStreamingConfigImpl share
namespace add_config
{

inline constexpr const char* AvailableTransportOptionsProperty = "AvailableTransportOptions";

inline void FreezeAvailableTransportOptions(const PropertyObjectPtr& config)
{
    config.getPropertyValue(AvailableTransportOptionsProperty).freeze();
}

inline void AddTransportSelectionProperties(const PropertyObjectPtr& config, const DictPtr<IString, ITransportOption>& options)
{
    const TransportOptionPtr firstOption = FirstValue(options);
    const StringPtr firstId = firstOption.assigned() ? firstOption.getId() : String("");
    config.addProperty(StringPropertyBuilder(TransportOptionConfigProperty, firstId).setSelectionValues(options.getKeyList()).build());
    config.addProperty(IntPropertyBuilder(PortConfigProperty, -1).setMinValue(-1).build());
    config.addProperty(ObjectProperty(AvailableTransportOptionsProperty, PropertyObject()));
    config.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue(AvailableTransportOptionsProperty, TransportOptionsObject(options));
    FreezeAvailableTransportOptions(config);
}

inline ListPtr<IString> TransportOptionIds(const PropertyObjectPtr& config)
{
    return config.getProperty(TransportOptionConfigProperty).getSelectionValues().asPtr<IList, ListPtr<IString>>();
}

inline bool HasTransportOptions(const PropertyObjectPtr& config)
{
    return TransportOptionIds(config).getCount() > 0;
}

// Builds only the selected option. Unassigned for a config without options.
inline TransportOptionPtr ReadSelectedTransportOption(const PropertyObjectPtr& config)
{
    if (!HasTransportOptions(config))
        return nullptr;

    const StringPtr selected = config.getPropertyValue(TransportOptionConfigProperty);
    const PropertyObjectPtr options = config.getPropertyValue(AvailableTransportOptionsProperty);
    for (const auto& prop : options.getAllProperties())
    {
        const PropertyObjectPtr option = options.getPropertyValue(prop.getName());
        if (option.getPropertyValue("Id") == selected)
            return TransportOptionFromObject(option);
    }

    DAQ_THROW_EXCEPTION(NotFoundException, R"(The selected transport option "{}" is not available)", selected);
}

inline ErrCode GetAvailableTransportOptions(const PropertyObjectPtr& config, IDict** transportOptions)
{
    OPENDAQ_PARAM_NOT_NULL(transportOptions);

    // A new dictionary on each call, frozen so that a change to it fails instead of being lost
    return daqTry([&]
    {
        auto options = TransportOptionsFromObject(config.getPropertyValue(AvailableTransportOptionsProperty));
        options.freeze();
        *transportOptions = options.detach();
    });
}

inline ErrCode SetTransportOption(const PropertyObjectPtr& config, IString* id)
{
    OPENDAQ_PARAM_NOT_NULL(id);

    return daqTry([&]
    {
        const StringPtr idStr = id;
        for (const auto& optionId : TransportOptionIds(config))
        {
            // A port set stays: it overrides the port of whichever option is selected
            if (optionId == idStr)
                return config->setPropertyValue(String(TransportOptionConfigProperty), idStr);
        }

        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTFOUND, fmt::format(R"(There is no transport option "{}" to select)", idStr));
    });
}

inline ErrCode GetSelectedTransportOption(const PropertyObjectPtr& config, ITransportOption** transportOption)
{
    OPENDAQ_PARAM_NOT_NULL(transportOption);

    return daqTry([&] { *transportOption = ReadSelectedTransportOption(config).detach(); });
}

inline ErrCode SetPort(const PropertyObjectPtr& config, Int port)
{
    // -1 drops the port set, so the port of the selected option applies again
    if (port == -1)
        return config->clearPropertyValue(String(PortConfigProperty));

    if (port < 0)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "The port must not be negative, other than -1");

    return config->setPropertyValue(String(PortConfigProperty), Integer(port));
}

inline ErrCode GetPort(const PropertyObjectPtr& config, Int* port)
{
    OPENDAQ_PARAM_NOT_NULL(port);

    return daqTry([&]
    {
        // A port set is used whichever option is selected; -1 means none is set
        const Int portSet = config.getPropertyValue(PortConfigProperty);
        const TransportOptionPtr selected = portSet == -1 ? ReadSelectedTransportOption(config) : nullptr;
        *port = selected.assigned() ? selected.getPort() : portSet;
    });
}

}

END_NAMESPACE_OPENDAQ
