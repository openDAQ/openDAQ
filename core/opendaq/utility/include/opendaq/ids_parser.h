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

#include <coretypes/common.h>

BEGIN_NAMESPACE_OPENDAQ

class IdsParser
{
public:
    static bool splitRelativeId(const std::string& id, std::string& start, std::string& rest)
    {
        const auto equalsIdx = id.find_first_of('/');
        if (std::string::npos != equalsIdx)
        {
            start = id.substr(0, equalsIdx);
            rest = id.substr(equalsIdx + 1);
            return true;
        }

        return false;
    }

    static bool isNestedComponentId(const std::string& ancestorGlobalId, const std::string& descendantGlobalId)
    {
        return descendantGlobalId.find(ancestorGlobalId + "/") == 0;
    }

    static bool idEndsWith(const std::string& id, const std::string& idEnding)
    {
        if (idEnding.length() > id.length())
            return false;
        return std::equal(idEnding.rbegin(), idEnding.rend(), id.rbegin());
    }
};

END_NAMESPACE_OPENDAQ
