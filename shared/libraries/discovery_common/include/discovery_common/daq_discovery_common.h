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
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>
#include <mdns.h>

#define BEGIN_NAMESPACE_DISCOVERY_COMMON namespace daq { namespace discovery_common {
#define END_NAMESPACE_DISCOVERY_COMMON } }

BEGIN_NAMESPACE_DISCOVERY_COMMON

using TxtProperties = std::unordered_map<std::string, std::string>;

class IpModificationUtils
{
public:
    static constexpr const char* DAQ_IP_MODIFICATION_SERVICE_NAME = "_opendaq-ip-modification._udp.local.";
    static constexpr const char* DAQ_IP_MODIFICATION_SERVICE_ID = "OpenDAQIPC";
    static constexpr const char* DAQ_IP_MODIFICATION_SERVICE_VERSION = "0";

    static constexpr const char* ERROR_CODE_KEY = "ErrorCode";
    static constexpr const char* ERROR_MESSAGE_KEY = "ErrorMessage";
    // max lenght of "key=value" in txt record is 255, so max length of "value" == 255 - size('=') - size("key")
    static constexpr const size_t ERROR_MESSAGE_LENGTH = 255 - 1 - std::char_traits<char>::length(ERROR_MESSAGE_KEY);

    static constexpr const uint8_t IP_MODIFICATION_OPCODE = 0xF;
    static constexpr const uint8_t IP_GET_CONFIG_OPCODE = 0x8;

    static void encodeIpConfiguration(const PropertyObjectPtr& config, TxtProperties& props);
    static PropertyObjectPtr populateIpConfigProperties(const TxtProperties& txtProps);
};

class DiscoveryUtils
{
public:
    static TxtProperties readTxtRecord(size_t size, const void* buffer, size_t rdata_offset, size_t rdata_length);
    static std::string extractRecordName(const void* buffer, size_t nameOffset, size_t bufferSize);
    static std::string toTxtValue(const char* source, size_t length);

private:
    static size_t getTxtRecordsCount(const void* buffer, size_t size, size_t offset, size_t length);
};

END_NAMESPACE_DISCOVERY_COMMON
