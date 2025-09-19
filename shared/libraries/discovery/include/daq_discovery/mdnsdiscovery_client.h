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
#include <mdns.h>
#include <string>
#include <map>
#include <coretypes/errors.h>

#if defined(_WIN32) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS 1
#endif

#include <cerrno>
#include <cstdio>
#include <csignal>
#include <mutex>
#include <chrono>
#include <thread>
#include <vector>
#include <unordered_set>

#include <coretypes/string_ptr.h>
#include <coretypes/listobject_factory.h>
#include "daq_discovery/common.h"

#ifdef _WIN32
#include <iphlpapi.h>
#include <WinSock2.h>
#else
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <discovery_common/daq_discovery_common.h>

BEGIN_NAMESPACE_DISCOVERY

using namespace std::chrono_literals;

struct MdnsDiscoveredDevice
{
    std::string canonicalName;
    std::string serviceName;
    std::string serviceInstance;
    uint32_t servicePriority;
    uint32_t serviceWeight;
    uint32_t servicePort;
    std::unordered_set<std::string> ipv4Addresses;
    std::unordered_set<std::string> ipv6Addresses;
    discovery_common::TxtProperties properties;

    // Kept for API compatibility; modules should use all ip addresses instead
    std::string ipv4Address;
    std::string ipv6Address;

    std::string getPropertyOrDefault(const std::string& name, const std::string& def = "") const
    {
        if (auto iter = properties.find(name); iter != properties.cend())
        {
            return iter->second;
        }
        return def;
    }
};

// Implementation code adapted from https://github.com/mjansson/mdns
class MDNSDiscoveryClient
{
public:
    explicit MDNSDiscoveryClient(const ListPtr<IString>& serviceNames);
    ~MDNSDiscoveryClient();

    std::vector<MdnsDiscoveredDevice> getAvailableDevices();
    void setDiscoveryDuration(std::chrono::milliseconds discoveryDuration);

    ErrCode requestIpConfigModification(const std::string& serviceName, const discovery_common::TxtProperties& reqProps);
    ErrCode requestCurrentIpConfiguration(const std::string& serviceName,
                                          const discovery_common::TxtProperties& reqProps,
                                          discovery_common::TxtProperties& resProps);

protected:
    typedef struct
    {
        std::string serviceInstance;
        std::string serviceName;
    } PTRRecord;

    typedef struct
    {
        std::string serviceInstance;
        std::string serviceQualified;
        uint16_t priority;
        uint16_t weight;
        uint16_t port;
    } SRVRecord;
    
    typedef struct
    {
        std::string serviceQualified;
        std::string address;
    } ARecord;

    typedef struct
    {
        std::string serviceQualified;
        std::string address;
    } AAAARecord;

    typedef struct
    {
        std::string serviceInstance;
        std::vector<std::pair<std::string, std::string>> txt;
    } TXTRecord;

    // Key is serviceInstance
    std::unordered_map<std::string, PTRRecord> ptrRecords;
    // Key is serviceInstance
    std::unordered_map<std::string, SRVRecord> srvRecords;
    // Key is IPv4 address
    std::unordered_map<std::string, ARecord> aRecords;
    // Key is IPv6 address
    std::unordered_map<std::string, AAAARecord> aaaaRecords;
    // Key is serviceInstance
    std::unordered_map<std::string, TXTRecord> txtRecords;

    std::mutex recordsLock;
    std::atomic_bool started;

private:
    using QueryCallback = std::function<int(int sock,
                                            const sockaddr* from,
                                            size_t addrlen,
                                            mdns_entry_type_t entry,
                                            uint16_t query_id,
                                            uint16_t rtype,
                                            uint16_t rclass,
                                            uint32_t ttl,
                                            const void* buffer,
                                            size_t size,
                                            size_t name_offset,
                                            size_t name_length,
                                            size_t rdata_offset,
                                            size_t rdata_length,
                                            void* user_data,
                                            uint8_t opcode)>;

    static void transformToLower(std::string& str);
    void encodeNonDiscoveryRequest(const std::string& recordName,
                                   const discovery_common::TxtProperties& props,
                                   std::vector<mdns_record_t>& records);
    void setupDiscoveryQuery();
    void openClientSockets(std::vector<int>& sockets);
    std::vector<MdnsDiscoveredDevice> createDevices();
    void sendDiscoveryQuery();

    void sendNonDiscoveryQuery(const std::vector<mdns_record_t>& requestRecords,
                               uint8_t opCode,
                               uint16_t queryId,
                               QueryCallback callback);

    int discoveryQueryCallback(int sock,
                               const sockaddr* from,
                               size_t addrlen,
                               mdns_entry_type_t entry,
                               uint16_t query_id,
                               uint16_t rtype,
                               uint16_t rclass,
                               uint32_t ttl,
                               const void* buffer,
                               size_t size,
                               size_t rname_offset,
                               size_t rname_length,
                               size_t rdata_offset,
                               size_t rdata_length,
                               void* user_data,
                               uint8_t opcode);

    int ipConfigModificationQueryCallback(int sock,
                                          const sockaddr* from,
                                          size_t addrlen,
                                          mdns_entry_type_t entry,
                                          uint16_t responseQueryId,
                                          uint16_t rtype,
                                          uint16_t rclass,
                                          uint32_t ttl,
                                          const void* buffer,
                                          size_t size,
                                          size_t rname_offset,
                                          size_t rname_length,
                                          size_t rdata_offset,
                                          size_t rdata_length,
                                          void* user_data,
                                          uint8_t opcode,
                                          uint16_t requestQueryId,
                                          ErrCode& rpcErrorCode,
                                          std::string& rpcErrorMessage);

    int currentIpConfigQueryCallback(int sock,
                                     const sockaddr* from,
                                     size_t addrlen,
                                     mdns_entry_type_t entry,
                                     uint16_t responseQueryId,
                                     uint16_t rtype,
                                     uint16_t rclass,
                                     uint32_t ttl,
                                     const void* buffer,
                                     size_t size,
                                     size_t rname_offset,
                                     size_t rname_length,
                                     size_t rdata_offset,
                                     size_t rdata_length,
                                     void* user_data,
                                     uint8_t opcode,
                                     uint16_t requestQueryId,
                                     ErrCode& rpcErrorCode,
                                     std::string& rpcErrorMessage,
                                     discovery_common::TxtProperties& resProps);

    std::string ipv4AddressToString(const sockaddr_in* addr, size_t addrlen);
    std::string ipv6AddressToString(const sockaddr_in6* addr, size_t addrlen, const sockaddr_in6* from);
    bool isValidMdnsDevice(const MdnsDiscoveredDevice& device);
    std::string getIpv6NetworkInterface(const struct sockaddr_in6* from, size_t addrlen);

    std::vector<mdns_query_t> discoveryQueries;
    std::vector<std::string> serviceNames;
    std::thread discoveryThread;
    std::chrono::milliseconds discoveryDuration = 0ms;

    // prevents multiple requests to be processed simultaneously
    std::mutex requestSync;
    // guarantee the unique id for non-discovery requests as pair of client uuid and numeric query id
    std::string uuid;
    std::atomic_uint16_t nonDiscoveryQueryId;
    // prevents handling answer for same request multiple times
    std::unordered_set<uint16_t> answeredNonMdnsQueryIds;
    std::chrono::milliseconds nonDiscoveryReqDuration = 500ms;
};

inline MDNSDiscoveryClient::MDNSDiscoveryClient(const ListPtr<IString>& serviceNames)
    : started(false)
{
    this->serviceNames.reserve(serviceNames.getCount());
    for (const auto& service : serviceNames)
    {
        auto serviceStr = service.toStdString();
        transformToLower(serviceStr);
        this->serviceNames.push_back(serviceStr);
    }

    setupDiscoveryQuery();

    boost::uuids::random_generator gen;
    const auto uuidBoost = gen();
    uuid = boost::uuids::to_string(uuidBoost);

#ifdef _WIN32
    WORD versionWanted = MAKEWORD(1, 1);
    WSADATA wsaData;
    if (WSAStartup(versionWanted, &wsaData))
        throw std::runtime_error("MDNSDiscoveryClient::Failed to initialize WinSock");
#endif
}

inline MDNSDiscoveryClient::~MDNSDiscoveryClient()
{
#ifdef _WIN32
    WSACleanup();
#endif
}

inline std::vector<MdnsDiscoveredDevice> MDNSDiscoveryClient::getAvailableDevices()
{
    std::vector<MdnsDiscoveredDevice> devices;

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start < discoveryDuration)
    {
        try
        {
            this->sendDiscoveryQuery();
        }
        catch (const std::exception& e)
        {
            printf("MDNSDiscoveryClient: sendMdnsQuery failed with the error %s\n", e.what());
        }
        catch (...)
        {
            fprintf(stderr, "MDNSDiscoveryClient: sendMdnsQuery failed with an unknown error\n");
            break;
        }
    }

    return createDevices();
}

inline void MDNSDiscoveryClient::setDiscoveryDuration(std::chrono::milliseconds discoveryDuration)
{
    this->discoveryDuration = discoveryDuration;
}

inline ErrCode MDNSDiscoveryClient::requestIpConfigModification(const std::string& serviceName, const discovery_common::TxtProperties& reqProps)
{
    std::scoped_lock lock(requestSync);

    std::vector<mdns_record_t> records;
    encodeNonDiscoveryRequest(serviceName, reqProps, records);

    ErrCode rpcErrorCode = OPENDAQ_ERR_GENERALERROR;
    std::string rpcErrorMessage = "No response from device";
    const auto requestQueryId = ++nonDiscoveryQueryId;

    auto callback = [&](int sock,
                        const sockaddr* from,
                        size_t addrlen,
                        mdns_entry_type_t entry,
                        uint16_t query_id,
                        uint16_t rtype,
                        uint16_t rclass,
                        uint32_t ttl,
                        const void* buffer,
                        size_t size,
                        size_t name_offset,
                        size_t name_length,
                        size_t rdata_offset,
                        size_t rdata_length,
                        void* user_data,
                        uint8_t opcode) -> int
    {
        return ipConfigModificationQueryCallback(sock,
                                                 from,
                                                 addrlen,
                                                 entry,
                                                 query_id,
                                                 rtype,
                                                 rclass,
                                                 ttl,
                                                 buffer,
                                                 size,
                                                 name_offset,
                                                 name_length,
                                                 rdata_offset,
                                                 rdata_length,
                                                 user_data,
                                                 opcode,
                                                 requestQueryId,
                                                 rpcErrorCode,
                                                 rpcErrorMessage);
    };

    sendNonDiscoveryQuery(records, discovery_common::IpModificationUtils::IP_MODIFICATION_OPCODE, requestQueryId, callback);

    if (OPENDAQ_FAILED(rpcErrorCode))
        return DAQ_MAKE_ERROR_INFO(rpcErrorCode, rpcErrorMessage);

    return OPENDAQ_SUCCESS;
}

inline ErrCode MDNSDiscoveryClient::requestCurrentIpConfiguration(const std::string& serviceName,
                                                                  const discovery_common::TxtProperties& reqProps,
                                                                  discovery_common::TxtProperties& resProps)
{
    std::scoped_lock lock(requestSync);

    std::vector<mdns_record_t> records;
    encodeNonDiscoveryRequest(serviceName, reqProps, records);

    ErrCode rpcErrorCode = OPENDAQ_ERR_GENERALERROR;
    std::string rpcErrorMessage = "No response from device";
    const auto requestQueryId = ++nonDiscoveryQueryId;

    auto callback = [&](int sock,
                        const sockaddr* from,
                        size_t addrlen,
                        mdns_entry_type_t entry,
                        uint16_t query_id,
                        uint16_t rtype,
                        uint16_t rclass,
                        uint32_t ttl,
                        const void* buffer,
                        size_t size,
                        size_t name_offset,
                        size_t name_length,
                        size_t rdata_offset,
                        size_t rdata_length,
                        void* user_data,
                        uint8_t opcode) -> int
    {
        return currentIpConfigQueryCallback(sock,
                                            from,
                                            addrlen,
                                            entry,
                                            query_id,
                                            rtype,
                                            rclass,
                                            ttl,
                                            buffer,
                                            size,
                                            name_offset,
                                            name_length,
                                            rdata_offset,
                                            rdata_length,
                                            user_data,
                                            opcode,
                                            requestQueryId,
                                            rpcErrorCode,
                                            rpcErrorMessage,
                                            resProps);
    };

    sendNonDiscoveryQuery(records, discovery_common::IpModificationUtils::IP_GET_CONFIG_OPCODE, requestQueryId, callback);

    if (OPENDAQ_FAILED(rpcErrorCode))
        return DAQ_MAKE_ERROR_INFO(rpcErrorCode, rpcErrorMessage);

    return OPENDAQ_SUCCESS;
}

inline void MDNSDiscoveryClient::transformToLower(std::string& str)
{
    std::transform(str.begin(), str.end(), str.begin(),[](unsigned char c){ return static_cast<char>(std::tolower(c)); });
}

inline void MDNSDiscoveryClient::encodeNonDiscoveryRequest(const std::string& recordName,
                                                           const discovery_common::TxtProperties& props,
                                                           std::vector<mdns_record_t>& records)
{
    for (const auto & [key, value] : props)
    {
        mdns_record_t record;
        record.name = {recordName.c_str(), recordName.size()},
        record.type = MDNS_RECORDTYPE_TXT,
        record.data.txt.key = {key.c_str(), key.size()},
        record.data.txt.value = {value.c_str(), value.size()},
        record.rclass = MDNS_CLASS_IN | MDNS_UNICAST_RESPONSE,
        record.ttl = 0;
        records.push_back(record);
    }

    // attach uuid
    {
        mdns_record_t record;
        record.name = {recordName.c_str(), recordName.size()},
        record.type = MDNS_RECORDTYPE_TXT,
        record.data.txt.key = {"uuid", 4},
        record.data.txt.value = {uuid.c_str(), uuid.size()},
        record.rclass = MDNS_CLASS_IN | MDNS_UNICAST_RESPONSE,
        record.ttl = 0;
        records.push_back(record);
    }
}

inline void MDNSDiscoveryClient::setupDiscoveryQuery()
{
    std::vector<mdns_record_type> types {MDNS_RECORDTYPE_PTR};
    discoveryQueries.resize(serviceNames.size() * types.size());
    for (size_t nameIdx = 0; nameIdx < serviceNames.size(); nameIdx++)
    {
        const auto& name = serviceNames[nameIdx];
        size_t offset = nameIdx * types.size();
        for (size_t i = 0; i < types.size(); i++)
        {
            discoveryQueries[offset + i].name = name.c_str();
            discoveryQueries[offset + i].name_length = name.size();
            discoveryQueries[offset + i].type = types[i];
        }
    }
}

inline void MDNSDiscoveryClient::openClientSockets(std::vector<int>& sockets)
{
#ifdef _WIN32
    IP_ADAPTER_ADDRESSES* adapterAddress = nullptr;
    ULONG addressSize = 8000;
    unsigned int ret;
    unsigned int numRetries = 4;
    do
    {
        adapterAddress = static_cast<IP_ADAPTER_ADDRESSES*>(malloc(addressSize));
        ret = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_ANYCAST, nullptr, adapterAddress, &addressSize);
        if (ret == ERROR_BUFFER_OVERFLOW)
        {
            free(adapterAddress);
            adapterAddress = nullptr;
            addressSize *= 2;
        }
        else
        {
            break;
        }
    } while (numRetries-- > 0);

    if (!adapterAddress || (ret != NO_ERROR))
    {
        free(adapterAddress);
        throw std::runtime_error("Failed to get adapter addresses");
    }

    for (PIP_ADAPTER_ADDRESSES adapter = adapterAddress; adapter; adapter = adapter->Next)
    {
        if (adapter->TunnelType == TUNNEL_TYPE_TEREDO)
            continue;
        if (adapter->OperStatus != IfOperStatusUp)
            continue;

        for (IP_ADAPTER_UNICAST_ADDRESS* unicast = adapter->FirstUnicastAddress; unicast; unicast = unicast->Next)
        {
            if (unicast->Address.lpSockaddr->sa_family == AF_INET)
            {
                auto saddr = reinterpret_cast<sockaddr_in*>(unicast->Address.lpSockaddr);

                if ((saddr->sin_addr.S_un.S_un_b.s_b1 != 127) || (saddr->sin_addr.S_un.S_un_b.s_b2 != 0) ||
                    (saddr->sin_addr.S_un.S_un_b.s_b3 != 0) || (saddr->sin_addr.S_un.S_un_b.s_b4 != 1))
                {
                    saddr->sin_port = htons(static_cast<unsigned short>(0));
                    int sock = mdns_socket_open_ipv4(saddr);
                    if (sock >= 0)
                        sockets.push_back(sock);
                }
            }
            else if (unicast->Address.lpSockaddr->sa_family == AF_INET6)
            {
                auto saddr = reinterpret_cast<sockaddr_in6*>(unicast->Address.lpSockaddr);

                const unsigned char localhost[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
                const unsigned char localhostMapped[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff, 0x7f, 0, 0, 1};
                if ((unicast->DadState == NldsPreferred) && memcmp(saddr->sin6_addr.s6_addr, localhost, 16) &&
                    memcmp(saddr->sin6_addr.s6_addr, localhostMapped, 16))
                {
                    saddr->sin6_port = htons(static_cast<unsigned short>(0));
                    int sock = mdns_socket_open_ipv6(saddr);
                    if (sock >= 0)
                        sockets.push_back(sock);
                }
            }
        }
    }

    free(adapterAddress);
#else
    struct ifaddrs* ifaddr = 0;
    struct ifaddrs* ifa = 0;

    if (getifaddrs(&ifaddr) < 0)
        throw std::runtime_error("Failed to get network interfaces");

    for (ifa = ifaddr; ifa; ifa = ifa->ifa_next)
    {
        if (!ifa->ifa_addr)
            continue;
        if (!(ifa->ifa_flags & IFF_UP) || !(ifa->ifa_flags & IFF_MULTICAST))
            continue;
        if ((ifa->ifa_flags & IFF_LOOPBACK) || (ifa->ifa_flags & IFF_POINTOPOINT))
            continue;

        if (ifa->ifa_addr->sa_family == AF_INET)
        {
            struct sockaddr_in* saddr = (struct sockaddr_in*) ifa->ifa_addr;
            if (saddr->sin_addr.s_addr != htonl(INADDR_LOOPBACK))
            {
                saddr->sin_port = htons(static_cast<unsigned short>(0));
                int sock = mdns_socket_open_ipv4(saddr);
                if (sock >= 0)
                    sockets.push_back(sock);
            }
        }
        else if (ifa->ifa_addr->sa_family == AF_INET6)
        {
            struct sockaddr_in6* saddr = (struct sockaddr_in6*) ifa->ifa_addr;
            static const unsigned char localhost[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
            static const unsigned char localhost_mapped[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff, 0x7f, 0, 0, 1};
            if (memcmp(saddr->sin6_addr.s6_addr, localhost, 16) && memcmp(saddr->sin6_addr.s6_addr, localhost_mapped, 16))
            {
                saddr->sin6_port = htons(static_cast<unsigned short>(0));
                int sock = mdns_socket_open_ipv6(saddr);
                if (sock >= 0)
                    sockets.push_back(sock);
            }
        }
    }

    freeifaddrs(ifaddr);
#endif
}

inline std::vector<MdnsDiscoveredDevice> MDNSDiscoveryClient::createDevices()
{
    std::unordered_map<std::string, std::string> serviceInstances;
    std::vector<MdnsDiscoveredDevice> devices;

    for (const auto& [serviceInstance, ptr] : ptrRecords)
    {
        if (std::find(serviceNames.begin(), serviceNames.end(), ptr.serviceName) == serviceNames.end())
            continue;

        serviceInstances.emplace(serviceInstance, ptr.serviceName);
    }

    for (const auto& [srvServiceInstance, srv] : srvRecords)
    {
        auto it = serviceInstances.find(srvServiceInstance);
        if (it == serviceInstances.end())
            continue;

        auto device = MdnsDiscoveredDevice{};
        device.serviceName = it->second;
        device.serviceInstance = srvServiceInstance;
        device.canonicalName = srv.serviceQualified;
        device.servicePriority = srv.priority;
        device.serviceWeight = srv.weight;
        device.servicePort = srv.port;

        for (const auto& [ipv4, a] : aRecords)
        {
            if (a.serviceQualified == srv.serviceQualified)
                device.ipv4Addresses.insert(ipv4);
        }

        for (const auto& [ipv6, aaaa] : aaaaRecords)
        {
            if (aaaa.serviceQualified == srv.serviceQualified)
                device.ipv6Addresses.insert(ipv6);
        }

        for (const auto& [txtServiceInstance, txt] : txtRecords)
        {
            if (txtServiceInstance == srvServiceInstance && device.properties.empty())
            {
                for (const auto& prop : txt.txt)
                    device.properties.insert(prop);
            }
        }

        // Kept to maintain API compatibility
        if (!device.ipv4Addresses.empty())
            device.ipv4Address = *device.ipv4Addresses.begin();

        if (!device.ipv6Addresses.empty())
            device.ipv6Address = *device.ipv6Addresses.begin();

        if (!device.ipv4Addresses.empty() || !device.ipv6Addresses.empty())
            devices.emplace_back(device);
    }

    return devices;
}

inline std::string MDNSDiscoveryClient::ipv4AddressToString(const sockaddr_in* addr, size_t addrlen)
{
    char host[NI_MAXHOST] = {0};
    char service[NI_MAXSERV] = {0};
    int ret = getnameinfo((const struct sockaddr*)addr, (socklen_t)addrlen, host, NI_MAXHOST,
                        service, NI_MAXSERV, NI_NUMERICSERV | NI_NUMERICHOST);
    if (ret != 0)
        return "";
    return std::string(host);
}

inline std::string MDNSDiscoveryClient::ipv6AddressToString(const sockaddr_in6* addr, size_t addrlen, const sockaddr_in6* from)
{
    char host[NI_MAXHOST] = {0};
    char service[NI_MAXSERV] = {0};
    int ret = getnameinfo((const struct sockaddr*)addr, (socklen_t)addrlen, host, NI_MAXHOST,
                          service, NI_MAXSERV, NI_NUMERICSERV | NI_NUMERICHOST);
    if (ret != 0)
        return "";

    std::string hostStr(host);

    if (IN6_IS_ADDR_LINKLOCAL(&addr->sin6_addr))
    {
        std::string ifname = getIpv6NetworkInterface(reinterpret_cast<const sockaddr_in6*>(from), addrlen);
        if (ifname.empty())
            return "";
        hostStr += ifname;
    }

    return  "[" + hostStr + "]";
}

inline std::string MDNSDiscoveryClient::getIpv6NetworkInterface(const struct sockaddr_in6* addr, size_t addrlen)
{
    if (addr->sin6_scope_id)
    {
#ifdef _WIN32
        // On Windows, sin6_scope_id should be used directly as an integer.
        return "%" + std::to_string(addr->sin6_scope_id);
#else
        // On Linux/macOS, convert interface index to name.
        char ifname[IF_NAMESIZE] = {0};
        if (if_indextoname(addr->sin6_scope_id, ifname))
            return "%" + std::string(ifname);
#endif
    }
    return "";
}

inline bool MDNSDiscoveryClient::isValidMdnsDevice(const MdnsDiscoveredDevice& device)
{
    return !device.ipv4Addresses.empty() || !device.ipv6Addresses.empty();
}

inline int MDNSDiscoveryClient::discoveryQueryCallback(int sock,
                                                       const sockaddr* from,
                                                       size_t addrlen,
                                                       mdns_entry_type_t entry,
                                                       uint16_t query_id,
                                                       uint16_t rtype,
                                                       uint16_t rclass,
                                                       uint32_t ttl,
                                                       const void* buffer,
                                                       size_t size,
                                                       size_t rname_offset,
                                                       size_t rname_length,
                                                       size_t rdata_offset,
                                                       size_t rdata_length,
                                                       void* user_data,
                                                       uint8_t opcode)
{
    // ignore non-discovery responses
    if (opcode)
        return 0;

    if (entry == MDNS_ENTRYTYPE_QUESTION)
        return 0;

    std::string recordName = discovery_common::DiscoveryUtils::extractRecordName(buffer, rname_offset, size);
    transformToLower(recordName);

    std::lock_guard lg(recordsLock);

    if (rtype == MDNS_RECORDTYPE_PTR)
    {
        char tempBuffer[1024];
        mdns_string_t ptr = mdns_record_parse_ptr(buffer, size, rdata_offset, rdata_length, tempBuffer, sizeof(tempBuffer));
        std::string serviceInstance = std::string(ptr.str, ptr.length);
        transformToLower(serviceInstance);
        if (ptrRecords.count(serviceInstance))
            return 0;

        auto& record = ptrRecords[serviceInstance];
        record.serviceName = recordName;
        record.serviceInstance = serviceInstance;
    }
    else if (rtype == MDNS_RECORDTYPE_SRV)
    {
        if (srvRecords.count(recordName))
            return 0;

        char tempBuffer[1024];
        mdns_record_srv_t srv = mdns_record_parse_srv(buffer, size, rdata_offset, rdata_length, tempBuffer, sizeof(tempBuffer));
        std::string serviceQualified = std::string(srv.name.str, srv.name.length);
        transformToLower(serviceQualified);

        auto& record = srvRecords[recordName];
        record.serviceInstance = recordName;
        record.serviceQualified = serviceQualified;
        record.priority = srv.priority;
        record.weight = srv.weight;
        record.port = srv.port;
    }
    else if (rtype == MDNS_RECORDTYPE_A)
    {
        sockaddr_in addr;
        mdns_record_parse_a(buffer, size, rdata_offset, rdata_length, &addr);
        std::string address = ipv4AddressToString(&addr, sizeof(addr));
        if (aRecords.count(address))
            return 0;

        auto& record = aRecords[address];
        record.serviceQualified = recordName;
        record.address = address;
    }
    else if (rtype == MDNS_RECORDTYPE_AAAA)
    {
        sockaddr_in6 addr;
        mdns_record_parse_aaaa(buffer, size, rdata_offset, rdata_length, &addr);
        std::string address = ipv6AddressToString(&addr, sizeof(addr), reinterpret_cast<const sockaddr_in6*>(from));
        if (address == "" || aaaaRecords.count(address))
            return 0;

        auto& record = aaaaRecords[address];
        record.serviceQualified = recordName;
        record.address = address;
    }
    else if (rtype == MDNS_RECORDTYPE_TXT)
    {
        if (txtRecords.count(recordName))
            return 0;

        auto& record = txtRecords[recordName];
        record.serviceInstance = recordName;
        auto reqProps = discovery_common::DiscoveryUtils::readTxtRecord(size, buffer, rdata_offset, rdata_length);
        for (const auto& prop : reqProps)
            record.txt.emplace_back(prop);
    }

    return 0;
}

inline int MDNSDiscoveryClient::ipConfigModificationQueryCallback(int sock,
                                                                  const sockaddr* from,
                                                                  size_t addrlen,
                                                                  mdns_entry_type_t entry,
                                                                  uint16_t responseQueryId,
                                                                  uint16_t rtype,
                                                                  uint16_t rclass,
                                                                  uint32_t ttl,
                                                                  const void* buffer,
                                                                  size_t size,
                                                                  size_t rname_offset,
                                                                  size_t rname_length,
                                                                  size_t rdata_offset,
                                                                  size_t rdata_length,
                                                                  void* user_data,
                                                                  uint8_t opcode,
                                                                  uint16_t requestQueryId,
                                                                  ErrCode& rpcErrorCode,
                                                                  std::string& rpcErrorMessage)
{
    using namespace discovery_common;

    if (opcode != IpModificationUtils::IP_MODIFICATION_OPCODE ||
        rtype != MDNS_RECORDTYPE_TXT ||
        entry != MDNS_ENTRYTYPE_ANSWER ||
        responseQueryId != requestQueryId)
        return 0;

    // ignore duplicates of already handled answer
    if (answeredNonMdnsQueryIds.find(responseQueryId) != answeredNonMdnsQueryIds.end())
        return 0;

    if (DiscoveryUtils::extractRecordName(buffer, rname_offset, size) != IpModificationUtils::DAQ_IP_MODIFICATION_SERVICE_NAME)
        return 0;

    auto resProps = DiscoveryUtils::readTxtRecord(size, buffer, rdata_offset, rdata_length);

    // ignore if client uuid doesn't match
    if (const auto it = resProps.find("uuid"); it == resProps.end() || it->second != uuid)
        return 0;

    answeredNonMdnsQueryIds.insert(responseQueryId);

    const auto errCodeIt = resProps.find(IpModificationUtils::ERROR_CODE_KEY);
    if (errCodeIt == resProps.end())
        return 0;
    
    const auto errMsgIt = resProps.find(IpModificationUtils::ERROR_MESSAGE_KEY);
    if (errMsgIt == resProps.end())
        return 0;

    ErrCode rpcErrorCodeTmp;
    try
    {
        rpcErrorCodeTmp = static_cast<ErrCode>(std::stoul(errCodeIt->second));
    }
    catch (...)
    {
        return 0;
    }

    // set output parameters only if required txt records are correct
    rpcErrorCode = rpcErrorCodeTmp;
    rpcErrorMessage = errMsgIt->second;

    return 0;
}

inline unsigned long getAvailableData(int sock)
{
    unsigned long availableData = 0;

#ifdef _WIN32
    if (ioctlsocket(sock, FIONREAD, &availableData) == SOCKET_ERROR) 
        return 0;
#else
    if (ioctl(sock, FIONREAD, &availableData) == -1)
        return 0;
#endif

    return availableData;
}

inline int MDNSDiscoveryClient::currentIpConfigQueryCallback(int sock,
                                                             const sockaddr* from,
                                                             size_t addrlen,
                                                             mdns_entry_type_t entry,
                                                             uint16_t responseQueryId,
                                                             uint16_t rtype,
                                                             uint16_t rclass,
                                                             uint32_t ttl,
                                                             const void* buffer,
                                                             size_t size,
                                                             size_t rname_offset,
                                                             size_t rname_length,
                                                             size_t rdata_offset,
                                                             size_t rdata_length,
                                                             void* user_data,
                                                             uint8_t opcode,
                                                             uint16_t requestQueryId,
                                                             ErrCode& rpcErrorCode,
                                                             std::string& rpcErrorMessage,
                                                             discovery_common::TxtProperties& resProps)
{
    using namespace discovery_common;

    if (opcode != IpModificationUtils::IP_GET_CONFIG_OPCODE ||
        rtype != MDNS_RECORDTYPE_TXT ||
        entry != MDNS_ENTRYTYPE_ANSWER ||
        responseQueryId != requestQueryId)
        return 0;

    // ignore duplicates of already handled answer
    if (answeredNonMdnsQueryIds.find(responseQueryId) != answeredNonMdnsQueryIds.end())
        return 0;

    if (DiscoveryUtils::extractRecordName(buffer, rname_offset, size) != IpModificationUtils::DAQ_IP_MODIFICATION_SERVICE_NAME)
        return 0;

    resProps = DiscoveryUtils::readTxtRecord(size, buffer, rdata_offset, rdata_length);

    // ignore if client uuid doesn't match
    if (const auto it = resProps.find("uuid"); it == resProps.end() || it->second != uuid)
        return 0;

    answeredNonMdnsQueryIds.insert(responseQueryId);

    const auto errCodeIt = resProps.find(IpModificationUtils::ERROR_CODE_KEY);
    if (errCodeIt == resProps.end())
        return 0;
    
    const auto errMsgIt = resProps.find(IpModificationUtils::ERROR_MESSAGE_KEY);
    if (errMsgIt == resProps.end())
        return 0;

    ErrCode rpcErrorCodeTmp;
    try
    {
        rpcErrorCodeTmp = static_cast<ErrCode>(std::stoul(errCodeIt->second));
    }
    catch (...)
    {
        return 0;
    }

    // set output parameters only if required txt records are correct
    rpcErrorCode = rpcErrorCodeTmp;
    rpcErrorMessage = errMsgIt->second;

    return 0;
}

inline void MDNSDiscoveryClient::sendNonDiscoveryQuery(const std::vector<mdns_record_t>& requestRecords,
                                                       uint8_t opCode,
                                                       uint16_t queryId,
                                                       QueryCallback callback)
{
    std::chrono::steady_clock::time_point queryingStarted = std::chrono::steady_clock::now();

    std::vector<int> sockets;
    openClientSockets(sockets);
    if (sockets.empty())
        throw std::runtime_error("Failed to open sockets");

    std::vector<int> queryIds(sockets.size());
    {
        constexpr size_t capacity = 2048;
        std::vector<char> buffer(capacity);
        for (size_t isock = 0; isock < sockets.size(); ++isock)
            queryIds[isock] = non_mdns_query_send(sockets[isock], buffer.data(), buffer.size(), queryId, opCode, 0x0,
                                                  requestRecords.data(), requestRecords.size(), 0, 0, 0, 0, 0, 0,
                                                  0, 0, 0);
    }

    auto callbackWrapper = [](int sock,
                              const sockaddr* from,
                              size_t addrlen,
                              mdns_entry_type_t entry,
                              uint16_t query_id,
                              uint16_t rtype,
                              uint16_t rclass,
                              uint32_t ttl,
                              const void* buffer,
                              size_t size,
                              size_t name_offset,
                              size_t name_length,
                              size_t rdata_offset,
                              size_t rdata_length,
                              void* user_data,
                              uint8_t opcode) -> int
    {
        return (*static_cast<decltype(callback)*>(user_data))(sock,
                                                              from,
                                                              addrlen,
                                                              entry,
                                                              query_id,
                                                              rtype,
                                                              rclass,
                                                              ttl,
                                                              buffer,
                                                              size,
                                                              name_offset,
                                                              name_length,
                                                              rdata_offset,
                                                              rdata_length,
                                                              nullptr,
                                                              opcode);
    };

    while(true)
    {
        std::chrono::microseconds elapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - queryingStarted
        );
        std::chrono::microseconds timeoutDuration;
        if (nonDiscoveryReqDuration > elapsedTime)
        {
            timeoutDuration =
                std::chrono::duration_cast<std::chrono::microseconds>(nonDiscoveryReqDuration) - elapsedTime;
        }
        else
        {
            break;
        }

        timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = static_cast<decltype(timeout.tv_usec)>(timeoutDuration.count());

        int nfds = 0;
        fd_set readfs;
        FD_ZERO(&readfs);
        for (int socket : sockets)
        {
            if (socket >= nfds)
                nfds = socket + 1;
            FD_SET((u_int)socket, &readfs);
        }

        if (select(nfds, &readfs, 0, 0, &timeout) <= 0)
            break;
    
        for (size_t isock = 0; isock < sockets.size(); ++isock)
        {
            if (FD_ISSET(sockets[isock], &readfs))
            {
                auto availableData = getAvailableData(sockets[isock]);
                std::vector<char> buffer(availableData);
                mdns_query_recv(sockets[isock], buffer.data(), availableData, callbackWrapper, &callback, queryIds[isock]);
            }
            FD_SET((u_int) sockets[isock], &readfs);
        }
    };

    for (int socket : sockets)
        mdns_socket_close(socket);
}

inline void MDNSDiscoveryClient::sendDiscoveryQuery()
{
    std::scoped_lock lock(requestSync);

    std::chrono::steady_clock::time_point queryingStarted = std::chrono::steady_clock::now();

    std::vector<int> sockets;
    openClientSockets(sockets);

    {
        struct sockaddr_in sock_addr;
        memset(&sock_addr, 0, sizeof(struct sockaddr_in));
        sock_addr.sin_family = AF_INET;
#ifdef _WIN32
        sock_addr.sin_addr = in4addr_any;
#else
        sock_addr.sin_addr.s_addr = INADDR_ANY;
#endif
        sock_addr.sin_port = htons(MDNS_PORT);
#ifdef __APPLE__
        sock_addr.sin_len = sizeof(struct sockaddr_in);
#endif
        int sock = mdns_socket_open_ipv4(&sock_addr);
        if (sock >= 0)
            sockets.push_back(sock);
    }
        
    {
        struct sockaddr_in6 sock_addr;
        memset(&sock_addr, 0, sizeof(struct sockaddr_in6));
        sock_addr.sin6_family = AF_INET6;
        sock_addr.sin6_addr = in6addr_any;
        sock_addr.sin6_port = htons(MDNS_PORT);
#ifdef __APPLE__
        sock_addr.sin6_len = sizeof(struct sockaddr_in6);
#endif
        int sock = mdns_socket_open_ipv6(&sock_addr);
        if (sock >= 0)
            sockets.push_back(sock);
	}

    if (sockets.empty())
        throw std::runtime_error("Failed to open sockets");

    std::vector<int> queryId(sockets.size());
    {
        constexpr size_t capacity = 2048;
        std::vector<char> buffer(capacity);
        for (size_t isock = 0; isock < sockets.size(); ++isock)
            queryId[isock] = mdns_multiquery_send(sockets[isock], discoveryQueries.data(), discoveryQueries.size(), buffer.data(), buffer.size(), 0);
    }

    auto callback = [&](int sock,
                        const sockaddr* from,
                        size_t addrlen,
                        mdns_entry_type_t entry,
                        uint16_t query_id,
                        uint16_t rtype,
                        uint16_t rclass,
                        uint32_t ttl,
                        const void* buffer,
                        size_t size,
                        size_t name_offset,
                        size_t name_length,
                        size_t rdata_offset,
                        size_t rdata_length,
                        void* user_data,
                        uint8_t opcode) -> int
    {
        return discoveryQueryCallback(sock,
                                      from,
                                      addrlen,
                                      entry,
                                      query_id,
                                      rtype,
                                      rclass,
                                      ttl,
                                      buffer,
                                      size,
                                      name_offset,
                                      name_length,
                                      rdata_offset,
                                      rdata_length,
                                      user_data,
                                      opcode);
    };

    auto callbackWrapper = [](int sock,
                              const sockaddr* from,
                              size_t addrlen,
                              mdns_entry_type_t entry,
                              uint16_t query_id,
                              uint16_t rtype,
                              uint16_t rclass,
                              uint32_t ttl,
                              const void* buffer,
                              size_t size,
                              size_t name_offset,
                              size_t name_length,
                              size_t rdata_offset,
                              size_t rdata_length,
                              void* user_data,
                              uint8_t opcode) -> int
    {
        return (*static_cast<decltype(callback)*>(user_data)) (sock,
                                                               from,
                                                               addrlen,
                                                               entry,
                                                               query_id,
                                                               rtype,
                                                               rclass,
                                                               ttl,
                                                               buffer,
                                                               size,
                                                               name_offset,
                                                               name_length,
                                                               rdata_offset,
                                                               rdata_length,
                                                               nullptr,
                                                               opcode);
    };

    while (true)
    {
        std::chrono::microseconds elapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - queryingStarted
        );
        std::chrono::microseconds timeoutDuration;
        if (discoveryDuration > elapsedTime)
        {
            timeoutDuration =
                std::chrono::duration_cast<std::chrono::microseconds>(discoveryDuration) - elapsedTime;
        }
        else
        {
            break;
        }

        timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = static_cast<decltype(timeout.tv_usec)>(timeoutDuration.count());

        int nfds = 0;
        fd_set readfs;
        FD_ZERO(&readfs);
        for (int socket : sockets)
        {
            if (socket >= nfds)
                nfds = socket + 1;
            FD_SET((u_int)socket, &readfs);
        }

        if (select(nfds, &readfs, 0, 0, &timeout) <= 0)
            break;
        
        for (size_t isock = 0; isock < sockets.size(); ++isock)
        {
            if (FD_ISSET(sockets[isock], &readfs))
            {
                auto availableData = getAvailableData(sockets[isock]);
                std::vector<char> buffer(availableData);
                mdns_query_recv(sockets[isock], buffer.data(), availableData, callbackWrapper, &callback, queryId[isock]);
            }
            FD_SET((u_int) sockets[isock], &readfs);
        }
    };

    for (int socket: sockets)
        mdns_socket_close(socket);
}

END_NAMESPACE_DISCOVERY
