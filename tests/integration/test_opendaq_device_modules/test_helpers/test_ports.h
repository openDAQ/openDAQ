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

// Ports for the tests that build into test_device_modules_streaming. They shift every port they serve on
// by PortOffset, which keeps them off the ports of the other device module tests and lets ctest run both
// binaries at the same time. Only the sources of that binary include this header.

#include <cstdint>
#include <string>

#include <opendaq/opendaq.h>

BEGIN_NAMESPACE_OPENDAQ

namespace test_helpers
{
    inline constexpr uint16_t PortOffset = 100;

    // Ports the server modules listen on unless a configuration names another one
    inline constexpr uint16_t NativePortBase = 7420;
    inline constexpr uint16_t OpcuaPortBase = 4840;
    inline constexpr uint16_t LtStreamingPortBase = 7414;
    inline constexpr uint16_t LtSecureStreamingPortBase = 7415;
    inline constexpr uint16_t LtControlPortBase = 7438;

    // The port this binary serves on in place of a module default
    inline constexpr uint16_t testPort(uint16_t basePort)
    {
        return static_cast<uint16_t>(basePort + PortOffset);
    }

    // Shifts every port a server configuration carries. The switches that enable those ports are named
    // after them, so only integers are shifted.
    [[maybe_unused]]
    inline void offsetPorts(const PropertyObjectPtr& config)
    {
        if (!config.assigned())
            return;

        for (const auto& property : config.getAllProperties())
        {
            const auto name = property.getName();
            const std::string nameStr = name.toStdString();
            if (nameStr.size() < 4 || nameStr.compare(nameStr.size() - 4, 4, "Port") != 0)
                continue;
            if (property.getValueType() != ctInt)
                continue;

            const Int port = config.getPropertyValue(name);
            if (port > 0)
                config.setPropertyValue(name, port + PortOffset);
        }
    }

    // Adds a server listening on this binary's ports
    [[maybe_unused]]
    inline ServerPtr addServer(const InstancePtr& instance, const StringPtr& typeId, const PropertyObjectPtr& config = nullptr)
    {
        const auto serverConfig =
            config.assigned() ? config : instance.getAvailableServerTypes().get(typeId).createDefaultConfig();
        offsetPorts(serverConfig);
        return instance.addServer(typeId, serverConfig);
    }

    // Names this binary's port in a connection string that carries none, so that it reaches the servers
    // added by addServer()
    [[maybe_unused]]
    inline StringPtr connectionStringWithPort(const std::string& url)
    {
        const auto schemeEnd = url.find("://");
        if (schemeEnd == std::string::npos)
            return String(url);

        uint16_t base;
        const auto scheme = url.substr(0, schemeEnd);
        if (scheme == "daq.nd" || scheme == "daq.ns")
            base = NativePortBase;
        else if (scheme == "daq.opcua")
            base = OpcuaPortBase;
        else if (scheme == "daq.lt")
            base = LtStreamingPortBase;
        else if (scheme == "daq.lts")
            base = LtSecureStreamingPortBase;
        else
            return String(url);  // the local prefixes reach no port

        const auto hostStart = schemeEnd + 3;
        auto hostEnd = url[hostStart] == '[' ? url.find(']', hostStart) : url.find_first_of(":/", hostStart);
        if (hostEnd == std::string::npos)
            hostEnd = url.size();
        else if (url[hostEnd] == ']')
            ++hostEnd;

        // a connection string that already names a port keeps it
        if (hostEnd < url.size() && url[hostEnd] == ':')
            return String(url);

        return String(url.substr(0, hostEnd) + ":" + std::to_string(testPort(base)) + url.substr(hostEnd));
    }
}

END_NAMESPACE_OPENDAQ
