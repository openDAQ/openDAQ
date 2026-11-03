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

#include <coretypes/string_ptr.h>
#include <algorithm>
#include <string>

BEGIN_NAMESPACE_OPENDAQ

// Sets the port of a "prefix://host[:port][/path]" connection string; an IPv6 host has to be in brackets
inline StringPtr ReplaceConnectionStringPort(const StringPtr& connectionString, Int port)
{
    if (!connectionString.assigned())
        return connectionString;

    const std::string url = connectionString.toStdString();
    const auto separator = url.find("://");
    if (separator == std::string::npos)
        return connectionString;

    const auto hostStart = separator + 3;
    const auto hostEnd = std::min(url.find_first_of("/?#", hostStart), url.size());
    const auto authority = url.substr(hostStart, hostEnd - hostStart);

    std::string host;
    if (!authority.empty() && authority.front() == '[')
    {
        const auto bracketEnd = authority.find(']');
        if (bracketEnd == std::string::npos)
            return connectionString;
        host = authority.substr(0, bracketEnd + 1);
    }
    else
    {
        // an IPv6 address without brackets cannot carry a port
        if (std::count(authority.begin(), authority.end(), ':') > 1)
            return connectionString;
        host = authority.substr(0, authority.find(':'));
    }

    return String(url.substr(0, hostStart) + host + ":" + std::to_string(port) + url.substr(hostEnd));
}

END_NAMESPACE_OPENDAQ
