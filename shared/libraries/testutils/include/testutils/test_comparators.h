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
#include <coretypes/common.h>
#include <coreobjects/property_object_ptr.h>
#include <opendaq/function_block_type_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class TestComparators
{
public:
    static bool PropertyObjectEquals(const PropertyObjectPtr& a, const PropertyObjectPtr& b)
    {
        auto propertiesA = a.getAllProperties();
        auto propertiesB = b.getAllProperties();

        if (propertiesA.getCount() != propertiesB.getCount())
            return false;

        for (size_t i = 0; i < propertiesA.getCount(); i++)
        {
            const auto keyA = propertiesA.getItemAt(i).getName();
            const auto keyB = propertiesB.getItemAt(i).getName();

            if (!BaseObjectPtr::Equals(keyA, keyB))
                return false;

            const auto valueA = a.getPropertyValue(keyA);
            const auto valueB = b.getPropertyValue(keyA);

            if (!BaseObjectPtr::Equals(valueA, valueB))
                return false;
        }

        return true;
    }

    static bool FunctionBlockTypeEquals(const FunctionBlockTypePtr& a, const FunctionBlockTypePtr& b)
    {
        if (a.getId() != b.getId())
            return false;
        if (a.getName() != b.getName())
            return false;
        if (a.getDescription() != b.getDescription())
            return false;

        const auto configA = a.createDefaultConfig();
        const auto configB = b.createDefaultConfig();

        if (!configA.assigned() && !configB.assigned())
            return true;
        else if (configA.assigned() && configB.assigned())
            return PropertyObjectEquals(configA, configB);
        else
            return false;
    }
};

END_NAMESPACE_OPENDAQ
