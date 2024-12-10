/*
 * Copyright 2022-2024 openDAQ d.o.o.
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
#endif

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

BEGIN_NAMESPACE_DISCOVERY

using namespace std::chrono_literals;

using TxtProperties = std::unordered_map<std::string, std::string>;

struct MdnsDiscoveredDevice
{
    std::string canonicalName;
    std::string serviceName;
    uint32_t servicePriority;
    uint32_t serviceWeight;
    uint32_t servicePort;
    std::string ipv4Address;
    std::string ipv6Address;
    TxtProperties properties;

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

    ErrCode requestIpConfigModification(const std::string& serviceName, const TxtProperties& reqProps);
    ErrCode requestCurrentIpConfiguration(const std::string& serviceName, const TxtProperties& reqProps, TxtProperties& resProps);

    static constexpr const char* DAQ_IP_MODIFICATION_SERVICE_NAME = "_opendaq-ip-modification._udp.local.";

protected:
    typedef struct
    {
        std::string name;
        uint16_t priority;
        uint16_t weight;
        uint16_t port;
    } SRVRecord;

    typedef struct
    {
        std::string PTR;
        SRVRecord SRV;
        std::string A;
        std::string AAAA;
        std::string linkLocalInterface;
        std::vector<std::pair<std::string, std::string>> TXT;
    } DeviceData;

    std::map<std::string, DeviceData> devicesMap;
    std::mutex devicesMapLock;
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

    void encodeNonDiscoveryRequest(const std::string& recordName, const TxtProperties& props, std::vector<mdns_record_t>& records);
    void setupDiscoveryQuery();
    void openClientSockets(std::vector<int>& sockets, int maxSockets);
    void pruneDevices();
    void sendDiscoveryQuery();

    void sendNonDiscoveryQuery(const std::vector<mdns_record_t>& txtRecords,
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
                                     TxtProperties& currentIpConfig);

    std::string ipv4AddressToString(const sockaddr_in* addr, size_t addrlen, bool includePort = true);
    std::string ipv6AddressToString(const sockaddr_in6* addr, size_t addrlen, bool includePort = true);
    std::string ipAddressToString(const sockaddr* addr, size_t addrlen, bool includePort = true);
    MdnsDiscoveredDevice createMdnsDiscoveredDevice(const DeviceData& device);
    bool isValidMdnsDevice(const MdnsDiscoveredDevice& device);

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

    static constexpr const uint8_t IP_MODIFICATION_OPCODE = 0xF;
    static constexpr const uint8_t IP_GET_CONFIG_OPCODE = 0x8;
};

inline MDNSDiscoveryClient::MDNSDiscoveryClient(const ListPtr<IString>& serviceNames)
    : started(false)
{
    this->serviceNames.reserve(serviceNames.getCount());
    for (const auto & service : serviceNames)
        this->serviceNames.push_back(service.toStdString());
    setupDiscoveryQuery();

    boost::uuids::random_generator gen;
    const auto uuidBoost = gen();
    uuid = boost::uuids::to_string(uuidBoost);

#ifdef _WIN32

    WORD versionWanted = MAKEWORD(1, 1);
    WSADATA wsaData;
    if (WSAStartup(versionWanted, &wsaData))
    {
        printf("Failed to initialize WinSock\n");
    }
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
    devicesMap.clear();
    std::vector<MdnsDiscoveredDevice> devices;

    try
    {
        std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - start < discoveryDuration)
            this->sendDiscoveryQuery();
    }
    catch (...)
    {
        return devices;
    }

    pruneDevices();
    for (const auto& device : devicesMap)
        devices.push_back(createMdnsDiscoveredDevice(device.second));

    return devices;
}

inline void MDNSDiscoveryClient::setDiscoveryDuration(std::chrono::milliseconds discoveryDuration)
{
    this->discoveryDuration = discoveryDuration;
}

inline ErrCode MDNSDiscoveryClient::requestIpConfigModification(const std::string& serviceName, const TxtProperties& reqProps)
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

    sendNonDiscoveryQuery(records, IP_MODIFICATION_OPCODE, requestQueryId, callback);

    if (OPENDAQ_FAILED(rpcErrorCode))
        return makeErrorInfo(rpcErrorCode, rpcErrorMessage, nullptr);

    return OPENDAQ_SUCCESS;
}

inline ErrCode MDNSDiscoveryClient::requestCurrentIpConfiguration(const std::string& serviceName,
                                                                  const TxtProperties& reqProps,
                                                                  TxtProperties& resProps)
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

    sendNonDiscoveryQuery(records, IP_GET_CONFIG_OPCODE, requestQueryId, callback);

    if (OPENDAQ_FAILED(rpcErrorCode))
        return makeErrorInfo(rpcErrorCode, rpcErrorMessage, nullptr);

    return OPENDAQ_SUCCESS;
}

inline void MDNSDiscoveryClient::encodeNonDiscoveryRequest(const std::string& recordName,
                                                           const TxtProperties& props,
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
            discoveryQueries[offset + i].length = name.size();
            discoveryQueries[offset + i].type = types[i];
        }
    }
}

inline void MDNSDiscoveryClient::openClientSockets(std::vector<int>& sockets, int maxSockets)
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
        return;
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
                    if (static_cast<int>(sockets.size()) < maxSockets)
                    {
                        saddr->sin_port = htons(static_cast<unsigned short>(0));
                        int sock = mdns_socket_open_ipv4(saddr);
                        if (sock >= 0)
                            sockets.push_back(sock);
                    }
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
                    if (static_cast<int>(sockets.size()) < maxSockets)
                    {
                        saddr->sin6_port = htons(static_cast<unsigned short>(0));
                        int sock = mdns_socket_open_ipv6(saddr);
                        if (sock >= 0)
                            sockets.push_back(sock);
                    }
                }
            }
        }
    }

    free(adapterAddress);
#else
    struct ifaddrs* ifaddr = 0;
    struct ifaddrs* ifa = 0;

    if (getifaddrs(&ifaddr) < 0)
      return;

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
                if (static_cast<int>(sockets.size()) < maxSockets)
                {
                    saddr->sin_port = htons(static_cast<unsigned short>(0));
                    int sock = mdns_socket_open_ipv4(saddr);
                    if (sock >= 0)
                        sockets.push_back(sock);
                }
            }
            break;
        }
        else if (ifa->ifa_addr->sa_family == AF_INET6)
        {
            struct sockaddr_in6* saddr = (struct sockaddr_in6*) ifa->ifa_addr;
            static const unsigned char localhost[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
            static const unsigned char localhost_mapped[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff, 0x7f, 0, 0, 1};
            if (memcmp(saddr->sin6_addr.s6_addr, localhost, 16) && memcmp(saddr->sin6_addr.s6_addr, localhost_mapped, 16))
            {
                if (static_cast<int>(sockets.size()) < maxSockets)
                {
                    saddr->sin6_port = htons(static_cast<unsigned short>(0));
                    int sock = mdns_socket_open_ipv6(saddr);
                    if (sock >= 0)
                        sockets.push_back(sock);
                }
            }
        }
    }

    freeifaddrs(ifaddr);
#endif
}

inline void MDNSDiscoveryClient::pruneDevices()
{
    std::unordered_set<std::string> toPrune;
    for (auto& [addr, data] : devicesMap)
    {
        if (toPrune.count(addr))
            continue;

        for (const auto& [addr1, data1] : devicesMap)
        {
            if (addr == addr1)
                continue;

            if (data.SRV.port != data1.SRV.port)
                continue;

            if (data.AAAA == data1.AAAA && !data.AAAA.empty())
            {
                data.A = data.A.empty() ? data1.A : data.A;
                data.linkLocalInterface = data.linkLocalInterface.empty() ? data1.linkLocalInterface : data.linkLocalInterface;
                toPrune.insert(addr1);
            }

            if (data.A == data1.A && !data.A.empty())
            {
                data.AAAA = data.AAAA.empty() ? data1.AAAA : data.AAAA;
                toPrune.insert(addr1);
            }
        }
    }

    for (const auto& val : toPrune)
        devicesMap.erase(val);
}

inline std::string MDNSDiscoveryClient::ipv4AddressToString(const sockaddr_in* addr, size_t addrlen, bool includePort)
{
    char host[NI_MAXHOST] = {0};
    char service[NI_MAXSERV] = {0};
    int ret = getnameinfo((const struct sockaddr*)addr, (socklen_t)addrlen, host, NI_MAXHOST,
                        service, NI_MAXSERV, NI_NUMERICSERV | NI_NUMERICHOST);
    if (ret != 0)
        return "";

    if (addr->sin_port != 0 && includePort)
        return std::string(host) + ":" + service;
    return std::string(host);
}

inline std::string MDNSDiscoveryClient::ipv6AddressToString(const sockaddr_in6* addr, size_t addrlen, bool includePort)
{
    char host[NI_MAXHOST] = {0};
    char service[NI_MAXSERV] = {0};
    int ret = getnameinfo((const struct sockaddr*)addr, (socklen_t)addrlen, host, NI_MAXHOST,
                          service, NI_MAXSERV, NI_NUMERICSERV | NI_NUMERICHOST);
    if (ret != 0)
        return "";

    if (addr->sin6_port != 0 && includePort)
        return "[" + std::string(host) + "]:" + service;
	return std::string(host);
}

inline std::string MDNSDiscoveryClient::ipAddressToString(const sockaddr* addr, size_t addrlen, bool includePort)
{
    if (addr->sa_family == AF_INET6)
        return ipv6AddressToString(reinterpret_cast<const sockaddr_in6*>(addr), addrlen, includePort);
    return ipv4AddressToString(reinterpret_cast<const sockaddr_in*>(addr), addrlen, includePort);
}

inline MdnsDiscoveredDevice MDNSDiscoveryClient::createMdnsDiscoveredDevice(const DeviceData& data)
{
    MdnsDiscoveredDevice device;
    device.canonicalName = data.PTR;
    device.serviceName = data.SRV.name;
    device.servicePort = data.SRV.port;
    device.servicePriority = data.SRV.priority;
    device.serviceWeight = data.SRV.weight;
    device.ipv4Address = data.A;
    device.ipv6Address = data.AAAA + data.linkLocalInterface;
    if (!device.ipv6Address.empty())
        device.ipv6Address = "[" + device.ipv6Address + "]";

    for (const auto& prop : data.TXT)
        device.properties.insert({prop.first, prop.second});

    return device;
}

inline bool MDNSDiscoveryClient::isValidMdnsDevice(const MdnsDiscoveredDevice& device)
{
    return device.ipv4Address.size() > 0 || device.ipv6Address.size() > 0;
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

    char nameBuffer[256];
    mdns_record_txt_t txtbuffer[128];

    std::lock_guard lg(devicesMapLock);
    
    std::string deviceAddr = ipAddressToString(from, addrlen);

    auto it = devicesMap.insert({deviceAddr, DeviceData{}});
    DeviceData& deviceData = it.first->second;

    if (from->sa_family == AF_INET6)
    {
        auto index = deviceAddr.find("%");
        if (index != std::string::npos)
            deviceData.linkLocalInterface = deviceAddr.substr(index, deviceAddr.find("]") - index);
    }

    if (rtype == MDNS_RECORDTYPE_PTR)
    {
        mdns_string_t namestr = mdns_record_parse_ptr(buffer, size, rdata_offset, rdata_length, nameBuffer, sizeof(nameBuffer));
        deviceData.PTR = std::string(namestr.str, namestr.length);
    }
    else if (rtype == MDNS_RECORDTYPE_SRV)
    {
        mdns_record_srv_t srv = mdns_record_parse_srv(buffer, size, rdata_offset, rdata_length, nameBuffer, sizeof(nameBuffer));
        deviceData.SRV = SRVRecord{std::string(srv.name.str, srv.name.length), srv.priority, srv.weight, srv.port};
    }
    else if (rtype == MDNS_RECORDTYPE_A)
    {
        sockaddr_in addr;
        mdns_record_parse_a(buffer, size, rdata_offset, rdata_length, &addr);
        deviceData.A = ipv4AddressToString(&addr, sizeof(addr));
    }
    else if (rtype == MDNS_RECORDTYPE_AAAA)
    {
        sockaddr_in6 addr;
        mdns_record_parse_aaaa(buffer, size, rdata_offset, rdata_length, &addr);
        deviceData.AAAA = ipv6AddressToString(&addr, sizeof(addr));
    }
    else if (rtype == MDNS_RECORDTYPE_TXT)
    {
        deviceData.TXT.clear();
        size_t parsed =
            mdns_record_parse_txt(buffer, size, rdata_offset, rdata_length, txtbuffer, sizeof(txtbuffer) / sizeof(mdns_record_txt_t));
        for (size_t itxt = 0; itxt < parsed; ++itxt)
        {
            std::string key(txtbuffer[itxt].key.str, txtbuffer[itxt].key.length);
            if (txtbuffer[itxt].value.length)
            {
                std::string value(txtbuffer[itxt].value.str, txtbuffer[itxt].value.length);
                deviceData.TXT.emplace_back(key, value);
            }
            else
                deviceData.TXT.emplace_back(key, "");
        }
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
    if (opcode != IP_MODIFICATION_OPCODE ||
        rtype != MDNS_RECORDTYPE_TXT ||
        entry != MDNS_ENTRYTYPE_ANSWER ||
        responseQueryId != requestQueryId)
        return 0;

    // ignore duplicates of already handled answer
    if (const auto it = answeredNonMdnsQueryIds.find(responseQueryId); it != answeredNonMdnsQueryIds.end())
        return 0;

    std::string recordName(256, '\0');
    mdns_string_extract(buffer, size, &rname_offset, recordName.data(), recordName.capacity());
    recordName.erase(recordName.find_last_not_of('\0') + 1);

    mdns_record_txt_t txtbuffer[128];
    TxtProperties resProps;
    size_t parsed =
        mdns_record_parse_txt(buffer, size, rdata_offset, rdata_length, txtbuffer, sizeof(txtbuffer) / sizeof(mdns_record_txt_t));
    for (size_t itxt = 0; itxt < parsed; ++itxt)
    {
        std::string key(txtbuffer[itxt].key.str, txtbuffer[itxt].key.length);
        if (txtbuffer[itxt].value.length)
        {
            std::string value(txtbuffer[itxt].value.str, txtbuffer[itxt].value.length);
            resProps[key] = value;
        }
        else
            resProps[key] = "";
    }

    // ignore if client uuid doesn't match
    if (const auto it = resProps.find("uuid"); it == resProps.end() || it->second != uuid)
        return 0;

    answeredNonMdnsQueryIds.insert(responseQueryId);

    if (const auto errCodeIt = resProps.find("ErrorCode"); errCodeIt != resProps.end())
    {
        if (const auto errMsgIt = resProps.find("ErrorMessage"); errMsgIt != resProps.end())
        {
            ErrCode rpcErrorCodeTmp;
            try
            {
                rpcErrorCodeTmp = static_cast<ErrCode>(std::stoul(errCodeIt->second));
            }
            catch (const std::exception& e)
            {
                return 0;
            }

            // set output parameters only if required txt records are correct
            rpcErrorCode = rpcErrorCodeTmp;
            rpcErrorMessage = errMsgIt->second;
        }
    }

    return 0;
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
                                                             TxtProperties& currentIpConfig)
{
    if (opcode != IP_GET_CONFIG_OPCODE ||
        rtype != MDNS_RECORDTYPE_TXT ||
        entry != MDNS_ENTRYTYPE_ANSWER ||
        responseQueryId != requestQueryId)
        return 0;

    std::string recordName(256, '\0');
    mdns_string_extract(buffer, size, &rname_offset, recordName.data(), recordName.capacity());
    recordName.erase(recordName.find_last_not_of('\0') + 1);
    if (recordName != DAQ_IP_MODIFICATION_SERVICE_NAME)
        return 0;

    // TODO
    rpcErrorCode = OPENDAQ_ERR_NOTIMPLEMENTED;
    rpcErrorMessage = "Response not parsed - feature is not implemented";

    return 0;
}

inline void MDNSDiscoveryClient::sendNonDiscoveryQuery(const std::vector<mdns_record_t>& requestRecords,
                                                       uint8_t opCode,
                                                       uint16_t queryId,
                                                       QueryCallback callback)
{
    std::chrono::steady_clock::time_point queryingStarted = std::chrono::steady_clock::now();

    constexpr int maxSockets = 32;
    std::vector<int> sockets;
    openClientSockets(sockets, maxSockets);
    if (sockets.empty())
        return;

    const int numSockets = static_cast<int>(sockets.size());
    int queryIds[maxSockets];
    constexpr size_t capacity = 2048;
    void* buffer = malloc(capacity);

    for (int isock = 0; isock < numSockets; ++isock)
        queryIds[isock] = non_mdns_query_send(sockets[isock], buffer, capacity, queryId, opCode, 0x0,
                                              requestRecords.data(), requestRecords.size(), 0, 0, 0, 0, 0, 0,
                                              0, 0, 0);

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

    int res;
    do
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
        timeout.tv_usec = timeoutDuration.count();

        int nfds = 0;
        fd_set readfs;
        FD_ZERO(&readfs);
        for (int isock = 0; isock < numSockets; ++isock)
        {
            if (sockets[isock] >= nfds)
                nfds = sockets[isock] + 1;
            FD_SET((u_int) sockets[isock], &readfs);
        }

        res = select(nfds, &readfs, 0, 0, &timeout);
        if (res > 0)
        {
            for (int isock = 0; isock < numSockets; ++isock)
            {
                if (FD_ISSET(sockets[isock], &readfs))
                    mdns_query_recv(sockets[isock], buffer, capacity, callbackWrapper, &callback, queryIds[isock]);
                FD_SET((u_int) sockets[isock], &readfs);
            }
        }
    }
    while (res > 0);

    free(buffer);
    for (int isock = 0; isock < numSockets; ++isock)
        mdns_socket_close(sockets[isock]);
}

inline void MDNSDiscoveryClient::sendDiscoveryQuery()
{
    std::scoped_lock lock(requestSync);

    std::chrono::steady_clock::time_point queryingStarted = std::chrono::steady_clock::now();

    constexpr int maxSockets = 32;
    std::vector<int> sockets;
    openClientSockets(sockets, maxSockets);
    if (sockets.empty())
        return;

    const int numSockets = static_cast<int>(sockets.size());
    int queryId[maxSockets];
    constexpr size_t capacity = 2048;
    void* buffer = malloc(capacity);

    for (int isock = 0; isock < numSockets; ++isock)
        queryId[isock] = mdns_multiquery_send(sockets[isock], discoveryQueries.data(), discoveryQueries.size(), buffer, capacity, 0);

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

    int res;
    do
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
        timeout.tv_usec = timeoutDuration.count();

        int nfds = 0;
        fd_set readfs;
        FD_ZERO(&readfs);
        for (int isock = 0; isock < numSockets; ++isock)
        {
            if (sockets[isock] >= nfds)
                nfds = sockets[isock] + 1;
            FD_SET((u_int) sockets[isock], &readfs);
        }

        res = select(nfds, &readfs, 0, 0, &timeout);
        if (res > 0)
        {
            for (int isock = 0; isock < numSockets; ++isock)
            {
                if (FD_ISSET(sockets[isock], &readfs))
                    mdns_query_recv(sockets[isock], buffer, capacity, callbackWrapper, &callback, queryId[isock]);
                FD_SET((u_int) sockets[isock], &readfs);
            }
        }
    } while (res > 0);

    free(buffer);
    for (int isock = 0; isock < numSockets; ++isock)
        mdns_socket_close(sockets[isock]);
}

END_NAMESPACE_DISCOVERY
