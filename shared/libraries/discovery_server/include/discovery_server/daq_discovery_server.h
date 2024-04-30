/*
 * Copyright 2022-2024 Blueberry d.o.o.
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
#include <coretypes/listobject_factory.h>
#include <discovery_server/mdnsdiscovery_server.h>
#include <coreobjects/property_object_ptr.h>

BEGIN_NAMESPACE_DISCOVERY_SERVICE


class DiscoveryServer final
{
public:
    explicit DiscoveryServer() = default;

    void registerDevice(const StringPtr& serverId, const PropertyObjectPtr& config);
    void removeDevice(const StringPtr& serverId);
    
private:
    MDNSDiscoveryServer server;
};

END_NAMESPACE_DISCOVERY_SERVICE
