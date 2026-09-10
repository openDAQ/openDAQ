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

// Ports for the device module tests that ctest runs at the same time. Each such binary gets its own id
// through the OPENDAQ_TEST_ID environment variable and shifts every port it serves on by that id, so two
// binaries cannot ask for the same one. Without the variable the id is 0 and the ports keep the values a
// single binary would use, which is what a binary started by hand gets.
//
// Shifted ports let a binary run without the network resource locks, but only if it announces no device
// over mDNS: announcements reach every process on the machine, and the tests that look up devices read
// whatever else is on the network as their own.

#include <cstdint>
#include <cstdlib>
#include <string>

#include <opendaq/opendaq.h>

BEGIN_NAMESPACE_OPENDAQ

namespace test_helpers
{
    inline uint16_t testId()
    {
        static const uint16_t id = []() -> uint16_t
        {
            const char* const value = std::getenv("OPENDAQ_TEST_ID");
            return value != nullptr ? static_cast<uint16_t>(std::atoi(value)) : uint16_t{0};
        }();
        return id;
    }

    // Ports the server modules listen on unless a configuration names another one
    inline constexpr uint16_t NativePortBase = 7420;
    inline constexpr uint16_t OpcuaPortBase = 4840;
    inline constexpr uint16_t LtStreamingPortBase = 7414;
    inline constexpr uint16_t LtSecureStreamingPortBase = 7415;
    inline constexpr uint16_t LtControlPortBase = 7438;

    // The port this binary serves on in place of a module default. The shifted ports sit above the
    // range of registered services and below the ephemeral one, so that nothing already listening on the
    // machine answers in a server's place, and 100 apart, so that two ids cannot meet.
    inline uint16_t testPort(int basePort)
    {
        const auto id = testId();
        return static_cast<uint16_t>(id == 0 ? basePort : basePort + 16000 + (id - 1) * 100);
    }

    // Shifts every port a server configuration carries. The switches that enable those ports are named
    // after them, so only integers are shifted.
    [[maybe_unused]]
    inline void offsetPorts(const PropertyObjectPtr& config)
    {
        if (!config.assigned() || testId() == 0)
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
                config.setPropertyValue(name, testPort(static_cast<int>(port)));
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
        if (testId() == 0 || schemeEnd == std::string::npos)
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
