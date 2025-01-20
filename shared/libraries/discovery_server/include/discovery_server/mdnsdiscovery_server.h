
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

#include <discovery_server/common.h>
#include <mutex>
#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <thread>
#include <atomic>
#include <mdns.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <iphlpapi.h>
#else
    #include <netdb.h>
    #include <ifaddrs.h>
    #include <net/if.h>
    #include <sys/time.h>
#endif

BEGIN_NAMESPACE_DISCOVERY_SERVICE

struct MdnsDiscoveredDevice
{
    MdnsDiscoveredDevice(const std::string& serviceName, uint32_t servicePort, const std::unordered_map<std::string, std::string>& properties);

private:
    friend class MDNSDiscoveryServer;

    void populateRecords(std::vector<mdns_record_t>& records) const;

    std::string serviceName;
    uint16_t servicePort;
    std::unordered_map<std::string, std::string> properties;
    mutable size_t recordSize;

    std::string serviceInstance;
    std::string serviceQualified;
};

class MDNSDiscoveryServer
{
public:
	explicit MDNSDiscoveryServer();
    ~MDNSDiscoveryServer();

    bool addDevice(const std::string& id, MdnsDiscoveredDevice& device);
    bool removeDevice(const std::string& id);
    
private:
    void start();
    void stop();
    void serviceLoop();

    void goodbyeMulticast(const MdnsDiscoveredDevice& device);

    std::string getHostname();
    
    void openClientSockets();
    void openServerSockets(std::vector<int>& sockets);

    int serviceCallback(int sock, const struct sockaddr* from, size_t addrlen, mdns_entry_type_t entry,
                 uint16_t query_id, uint16_t rtype, uint16_t rclass, uint32_t ttl, const void* data,
                 size_t size, size_t name_offset, size_t name_length, size_t record_offset,
                 size_t record_length, void* user_data);

    mdns_record_t createPtrRecord(const MdnsDiscoveredDevice& device) const;
    mdns_record_t createSrvRecord(const MdnsDiscoveredDevice& device) const;
    mdns_record_t createARecord(const MdnsDiscoveredDevice& device) const;
    mdns_record_t createAaaaRecord(const MdnsDiscoveredDevice& device) const;

    std::string hostName;
    sockaddr_in serviceAddressIpv4;
    sockaddr_in6 serviceAddressIpv6;
    
    std::mutex mx;
    std::atomic<bool> running {false};
    std::thread serviceThread;
    
    std::map<std::string, MdnsDiscoveredDevice> devices;

    std::vector<int> sockets;
};

END_NAMESPACE_DISCOVERY_SERVICE