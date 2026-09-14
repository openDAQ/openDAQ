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
// through the OPENDAQ_TEST_ID environment variable and shifts every port it serves on by that id, and each
// GoogleTest shard of it shifts them further by its shard index, so no two of them ask for the same port.
// Without the variable the id is 0 and the ports keep the values a single binary would use, which is what
// a binary started by hand gets.
//
// Shifted ports let a binary run without the network resource locks. Announcements over mDNS reach every
// process on the machine, so a binary that announces devices also marks their serial numbers with its id
// and shard: a discovered device is addressed by its manufacturer and serial number, and the tests that
// look up devices filter what they find by them.

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

    // The GoogleTest shard this process runs, 0 when it runs the whole binary
    inline uint16_t testShard()
    {
        static const uint16_t shard = []() -> uint16_t
        {
            const char* const value = std::getenv("GTEST_SHARD_INDEX");
            return value != nullptr ? static_cast<uint16_t>(std::atoi(value)) : uint16_t{0};
        }();
        return shard;
    }

    // Ports the server modules listen on unless a configuration names another one
    inline constexpr uint16_t NativePortBase = 7420;
    inline constexpr uint16_t OpcuaPortBase = 4840;
    inline constexpr uint16_t LtStreamingPortBase = 7414;
    inline constexpr uint16_t LtSecureStreamingPortBase = 7415;
    inline constexpr uint16_t LtControlPortBase = 7438;

    // The port this binary serves on in place of a module default. The shifted ports sit above the
    // range of registered services and below the ephemeral one, so that nothing already listening on the
    // machine answers in a server's place. Ids sit 1000 apart and shards 50 apart, while the ports a test
    // derives from one base spread over less than 30, so no two of them can meet.
    inline uint16_t testPort(int basePort)
    {
        const auto id = testId();
        return static_cast<uint16_t>(id == 0 ? basePort : basePort + 16000 + (id - 1) * 1000 + testShard() * 50);
    }

    // The serial number this binary announces in place of a plain one
    inline std::string testSerial(const std::string& serialNumber)
    {
        if (testId() == 0)
            return serialNumber;
        return serialNumber + "_t" + std::to_string(testId()) + "s" + std::to_string(testShard());
    }

    // The smart connection string of a device this binary announces
    [[maybe_unused]]
    inline StringPtr smartConnectionString(const std::string& manufacturer, const std::string& serialNumber)
    {
        return String("daq://" + manufacturer + "_" + testSerial(serialNumber));
    }

    // A root device configuration carrying this binary's serial number
    [[maybe_unused]]
    inline PropertyObjectPtr rootDeviceConfig(const std::string& serialNumber)
    {
        auto config = PropertyObject();
        config.addProperty(StringProperty("SerialNumber", String(testSerial(serialNumber))));
        return config;
    }

    // Whether a property names a port. The switches that enable the ports are named after them, so only
    // integers count.
    inline bool isPortProperty(const PropertyPtr& property)
    {
        const std::string name = property.getName().toStdString();
        return name.size() >= 4 && name.compare(name.size() - 4, 4, "Port") == 0 && property.getValueType() == ctInt;
    }

    // Shifts every port a server configuration carries
    [[maybe_unused]]
    inline void offsetPorts(const PropertyObjectPtr& config)
    {
        if (!config.assigned() || testId() == 0)
            return;

        for (const auto& property : config.getAllProperties())
        {
            if (!isPortProperty(property))
                continue;

            const auto name = property.getName();
            const Int port = config.getPropertyValue(name);
            if (port > 0)
                config.setPropertyValue(name, testPort(static_cast<int>(port)));
        }
    }

    // Adds a server listening on this binary's ports. A configuration that names no port gets the module
    // default, shifted like the rest.
    [[maybe_unused]]
    inline ServerPtr addServer(const InstancePtr& instance, const StringPtr& typeId, const PropertyObjectPtr& config = nullptr)
    {
        PropertyObjectPtr serverConfig = config;
        const auto types = instance.getAvailableServerTypes();
        if (types.hasKey(typeId))
        {
            const auto defaults = types.get(typeId).createDefaultConfig();
            if (!serverConfig.assigned())
                serverConfig = defaults;
            else
                for (const auto& property : defaults.getAllProperties())
                    if (isPortProperty(property) && !serverConfig.hasProperty(property.getName()))
                        serverConfig.addProperty(IntProperty(property.getName(), defaults.getPropertyValue(property.getName())));
        }
        offsetPorts(serverConfig);
        return instance.addServer(typeId, serverConfig);
    }

    // Names this binary's port in a connection string, shifting the one it carries or inserting the module
    // default when it carries none, so that it reaches the servers added by addServer()
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
        else if (scheme == "daq.lt" || scheme == "daq.ws")  // daq.ws is the legacy alias of daq.lt
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

        if (hostEnd < url.size() && url[hostEnd] == ':')
        {
            auto portEnd = url.find_first_not_of("0123456789", hostEnd + 1);
            if (portEnd == std::string::npos)
                portEnd = url.size();
            const auto port = std::stoi(url.substr(hostEnd + 1, portEnd - hostEnd - 1));
            return String(url.substr(0, hostEnd) + ":" + std::to_string(testPort(port)) + url.substr(portEnd));
        }

        return String(url.substr(0, hostEnd) + ":" + std::to_string(testPort(base)) + url.substr(hostEnd));
    }
}

END_NAMESPACE_OPENDAQ
