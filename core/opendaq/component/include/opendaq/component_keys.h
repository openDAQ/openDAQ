/*
 * Copyright 2022-2024 openDAQ d. o. o.
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
#include <coreobjects/object_keys.h>
#include <opendaq/component_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

struct ComponentHash
{
    size_t operator()(const ComponentPtr& component) const
    {
        return std::hash<StringPtr>().operator()(component.getGlobalId());
    }
};

struct ComponentEqualTo
{
    bool operator()(const ComponentPtr& a, const ComponentPtr& b) const
    {
        return std::equal_to<StringPtr>().operator()(a.getGlobalId(), b.getGlobalId());
    };
};

END_NAMESPACE_OPENDAQ

namespace std
{
    template <>
    struct hash<daq::ComponentPtr> : daq::ComponentHash
    {
    };

    template <>
    struct equal_to<daq::ComponentPtr> : daq::ComponentEqualTo
    {
    };
}
