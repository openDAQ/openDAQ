
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
#include <functional>
#include <mdns.h>
#include <unordered_set>

#ifdef _WIN32
    #include <winsock2.h>
    #include <iphlpapi.h>
#else
    #include <netdb.h>
    #include <ifaddrs.h>
    #include <net/if.h>
    #include <sys/time.h>
#endif

#include <discovery_common/daq_discovery_common.h>

BEGIN_NAMESPACE_DISCOVERY_SERVICE

struct MdnsDiscoveredService
{
    MdnsDiscoveredService(const std::string& serviceName, uint32_t servicePort, const std::unordered_map<std::string, std::string>& properties);

private:
    friend class MDNSDiscoveryServer;

    std::string serviceName;
    uint16_t servicePort;
    discovery_common::TxtProperties properties;

    std::string serviceInstance;
    std::string serviceQualified;
};

using ModifyIpConfigCallback = std::function<discovery_common::TxtProperties(const std::string& ifaceName, const discovery_common::TxtProperties& properties)>;
using RetrieveIpConfigCallback = std::function<discovery_common::TxtProperties(const std::string& ifaceName)>;

class MDNSDiscoveryServer
{
public:
	explicit MDNSDiscoveryServer();
    ~MDNSDiscoveryServer();

    bool registerService(const std::string& id, MdnsDiscoveredService& service);
    bool unregisterService(const std::string& id);

    bool registerIpModificationService(MdnsDiscoveredService& service,
                                       const ModifyIpConfigCallback& modifyIpConfigCb,
                                       const RetrieveIpConfigCallback& retrieveIpConfigCb);
    bool unregisterIpModificationService();
    bool isServiceRegistered(const std::string& id);
    
private:
    void start();
    void stop();
    void serviceLoop();

    void goodbyeMulticast(const MdnsDiscoveredService& service);

    std::string getHostname();
    
    void openClientSockets();
    void openServerSockets(std::vector<int>& sockets);

    int serviceCallback(int sock, const struct sockaddr* from, size_t addrlen, mdns_entry_type_t entry,
                 uint16_t query_id, uint16_t rtype, uint16_t rclass, uint32_t ttl, const void* buffer,
                 size_t size, size_t name_offset, size_t name_length, size_t rdata_offset,
                 size_t rdata_length, void* user_data, uint8_t opcode);

    int nonDiscoveryCallback(int sock, const sockaddr* from, size_t addrlen, mdns_entry_type_t entry,
        uint16_t query_id, uint16_t rtype, uint16_t rclass, const void* buffer,
        size_t size, size_t name_offset, size_t name_length, size_t rdata_offset,
        size_t rdata_length, void* user_data, uint8_t opcode);

    int discoveryCallback(int sock, const sockaddr* from, size_t addrlen, mdns_entry_type_t entry,
        uint16_t query_id, uint16_t rtype, uint16_t rclass, const void* buffer,
        size_t size, size_t name_offset, size_t name_length, size_t rdata_offset,
        size_t rdata_length, void* user_data);

    mdns_record_t createPtrRecord(const MdnsDiscoveredService& service) const;
    mdns_record_t createSrvRecord(const MdnsDiscoveredService& service) const;
    mdns_record_t createARecord(const MdnsDiscoveredService& service) const;
    mdns_record_t createAaaaRecord(const MdnsDiscoveredService& service) const;
    void populateTxtRecords(const std::string& recordName, const discovery_common::TxtProperties& props, std::vector<mdns_record_t>& records) const;

    void sendIpConfigResponse(int sock,
                              const sockaddr* to,
                              size_t addrlen,
                              uint16_t query_id,
                              discovery_common::TxtProperties& resProps,
                              uint8_t opcode,
                              bool unicast,
                              const std::string& uuid);

    std::string hostName;
    sockaddr_in serviceAddressIpv4;
    sockaddr_in6 serviceAddressIpv6;
    
    std::mutex mx;
    std::atomic<bool> running {false};
    std::thread serviceThread;
    
    std::map<std::string, MdnsDiscoveredService> services;

    std::vector<int> sockets;

    std::string manufacturer;
    std::string serialNumber;
    ModifyIpConfigCallback modifyIpConfigCallback{nullptr};
    RetrieveIpConfigCallback retrieveIpConfigCallback{nullptr};
    std::unordered_set<std::string> processedIpConfigReqIds;
};

END_NAMESPACE_DISCOVERY_SERVICE
