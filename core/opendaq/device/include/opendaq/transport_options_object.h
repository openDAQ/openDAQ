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
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_object_ptr.h>
#include <coretypes/exceptions.h>
#include <opendaq/transport_option_ptr.h>
#include <algorithm>
#include <string>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief Creates the Property object that keeps transport options, as a server capability and the add configs do.
 * @param options The options, keyed by their ids.
 * @throws InvalidValueException if two ids differ only in "/", ".", "[" or "]".
 *
 * Each option is a child object, named after the id of its option without the characters a property name cannot hold.
 */
inline PropertyObjectPtr TransportOptionsObject(const DictPtr<IString, ITransportOption>& options)
{
    auto container = PropertyObject();
    for (const auto& [id, option] : options)
    {
        std::string key = id;
        key.erase(std::remove_if(key.begin(), key.end(), [](char c) { return c == '/' || c == '.' || c == '[' || c == ']'; }),
                  key.end());
        if (key.empty() || container.hasProperty(key))
            DAQ_THROW_EXCEPTION(InvalidValueException, R"(Transport option "{}" cannot be told apart from another one)", option.getId());

        container.addProperty(ObjectProperty(key, option));
    }

    return container;
}

/*!
 * @brief Gets the transport options that a Property object created by TransportOptionsObject keeps.
 * @param container The Property object, also one that was deserialized or received from a peer.
 * @returns The options keyed by their ids, in the order of the children.
 *
 * A child that is not a transport option is left out. A peer of another version may send one.
 */
inline DictPtr<IString, ITransportOption> TransportOptionsFromObject(const PropertyObjectPtr& container)
{
    auto options = Dict<IString, ITransportOption>();
    for (const auto& prop : container.getAllProperties())
    {
        if (prop.getValueType() != ctObject)
            continue;

        const auto option = container.getPropertyValue(prop.getName()).asPtrOrNull<ITransportOption, TransportOptionPtr>();
        if (option.assigned())
            options.set(option.getId(), option);
    }

    return options;
}

END_NAMESPACE_OPENDAQ
