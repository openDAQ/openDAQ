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
#include "opcua.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA

#pragma push_macro("major")
#pragma push_macro("minor")

#undef major
#undef minor

struct OpcUaVersion
{
    explicit OpcUaVersion(const char* version);
    explicit constexpr OpcUaVersion(int major = 0, int minor = 0, int patch = 0)
        : major(major)
        , minor(minor)
        , patch(patch)
    {
    }

    std::string toString() const;

    int major = 0;
    int minor = 0;
    int patch = 0;

    static bool Compatible(const OpcUaVersion& serverVersion, const OpcUaVersion& systemVersion);
    static bool HasFeature(const OpcUaVersion& serverVersion, const OpcUaVersion& featureVersion);
};

#pragma pop_macro("minor")
#pragma pop_macro("major")

END_NAMESPACE_OPENDAQ_OPCUA
