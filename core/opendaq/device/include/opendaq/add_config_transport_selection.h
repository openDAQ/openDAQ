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
#include <coretypes/exceptions.h>
#include <opendaq/container_utils.h>
#include <opendaq/transport_option_factory.h>
#include <opendaq/transport_option_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

// The transport selection that AddDeviceConfigImpl and AddStreamingConfigImpl share
namespace add_config
{

// Adds the selected transport option, for a type with options, and the port set, -1 while none is set
inline void AddTransportSelectionProperties(const PropertyObjectPtr& config, const DictPtr<IString, ITransportOption>& options)
{
    if (options.getCount() > 0)
    {
        const TransportOptionPtr firstOption = FirstValue(options);
        config.addProperty(
            StringPropertyBuilder(TransportOptionConfigProperty, firstOption.getId()).setSelectionValues(options.getKeyList()).build());
    }

    config.addProperty(IntPropertyBuilder(PortConfigProperty, -1).setMinValue(-1).build());
}

inline ErrCode SetTransportOption(const PropertyObjectPtr& config, const DictPtr<IString, ITransportOption>& options, IString* id)
{
    OPENDAQ_PARAM_NOT_NULL(id);

    return daqTry([&]
    {
        const StringPtr idStr = id;
        if (!options.hasKey(idStr))
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTFOUND, fmt::format(R"(There is no transport option "{}" to select)", idStr));

        // A port set stays: it overrides the port of whichever option is selected
        return config->setPropertyValue(String(TransportOptionConfigProperty), idStr);
    });
}

inline ErrCode GetSelectedTransportOption(const PropertyObjectPtr& config,
                                          const DictPtr<IString, ITransportOption>& options,
                                          ITransportOption** transportOption)
{
    OPENDAQ_PARAM_NOT_NULL(transportOption);

    if (options.getCount() == 0)
    {
        *transportOption = nullptr;
        return OPENDAQ_SUCCESS;
    }

    // The property only takes the ids of the options
    return daqTry([&]
    {
        const StringPtr selected = config.getPropertyValue(TransportOptionConfigProperty);
        *transportOption = options.get(selected).detach();
    });
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

inline ErrCode GetPort(const PropertyObjectPtr& config, const DictPtr<IString, ITransportOption>& options, Int* port)
{
    OPENDAQ_PARAM_NOT_NULL(port);

    return daqTry([&]
    {
        // A port set is used whichever option is selected; -1 means none is set
        const Int portSet = config.getPropertyValue(PortConfigProperty);
        if (portSet != -1 || options.getCount() == 0)
        {
            *port = portSet;
            return;
        }

        const StringPtr selected = config.getPropertyValue(TransportOptionConfigProperty);
        *port = options.get(selected).getPort();
    });
}

}

END_NAMESPACE_OPENDAQ
