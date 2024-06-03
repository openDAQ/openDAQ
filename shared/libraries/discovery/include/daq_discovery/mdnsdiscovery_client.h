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

#include "coretypes/string_ptr.h"
#include "daq_discovery/common.h"

#ifdef _WIN32
#include <iphlpapi.h>
#include <WinSock2.h>
#else
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#endif

BEGIN_NAMESPACE_DISCOVERY

using namespace std::chrono_literals;

struct MdnsDiscoveredDevice
{
    std::string canonicalName;
    std::string serviceName;
    uint32_t servicePriority;
    uint32_t serviceWeight;
    uint32_t servicePort;
    std::string ipv4Address;
    std::string ipv6Address;
    std::unordered_map<std::string, std::string> properties;

    std::string getPropertyOrDefault(const std::string& name, const std::string& def = "")
    {
        return (properties.count(name) > 0) ? properties[name] : def;
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
        std::vector<std::pair<std::string, std::string>> TXT;
    } DeviceData;

    std::map<std::string, DeviceData> devicesMap;
    std::mutex devicesMapLock;
    std::atomic_bool started;

private:
    void setupQuery();
    void openClientSockets(std::vector<int>& sockets, int maxSockets);
    void pruneDevices();
    void sendMdnsQuery();
    int queryCallback(int sock,
                      const sockaddr* from,
                      size_t addrlen,
                      mdns_entry_type_t entry,
                      uint16_t query_id,
                      uint16_t rtype,
                      uint16_t rclass,
                      uint32_t ttl,
                      const void* data,
                      size_t size,
                      size_t name_offset,
                      size_t name_length,
                      size_t record_offset,
                      size_t record_length,
                      void* user_data);

    std::string ipv4AddressToString(const sockaddr_in* addr, size_t addrlen, bool includePort = true);
    std::string ipv6AddressToString(const sockaddr_in6* addr, size_t addrlen, bool includePort = true);
    std::string ipAddressToString(const sockaddr* addr, size_t addrlen, bool includePort = true);
    MdnsDiscoveredDevice createMdnsDiscoveredDevice(const DeviceData& device);
    bool isValidMdnsDevice(const MdnsDiscoveredDevice& device);

    std::vector<mdns_query_t> query;
    std::vector<std::string> serviceNames;
    std::thread discoveryThread;
    std::chrono::milliseconds discoveryDuration = 0ms;
};

inline MDNSDiscoveryClient::MDNSDiscoveryClient(const ListPtr<IString>& serviceNames)
    : started(false)
{
    this->serviceNames.reserve(serviceNames.getCount());
    for (const auto & service : serviceNames)
        this->serviceNames.push_back(service.toStdString());
    setupQuery();

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
            this->sendMdnsQuery();
    }
    catch (...)
    {
        return devices;
    }

    for (const auto& device : devicesMap)
        devices.push_back(createMdnsDiscoveredDevice(device.second));

    return devices;
}

inline void MDNSDiscoveryClient::setDiscoveryDuration(std::chrono::milliseconds discoveryDuration)
{
    this->discoveryDuration = discoveryDuration;
}

inline void MDNSDiscoveryClient::setupQuery()
{
    std::vector<mdns_record_type> types {MDNS_RECORDTYPE_PTR, MDNS_RECORDTYPE_SRV, MDNS_RECORDTYPE_A, MDNS_RECORDTYPE_AAAA};
    query.resize(serviceNames.size() * types.size());
    for (size_t nameIdx = 0; nameIdx < serviceNames.size(); nameIdx++)
    {
        const auto& name = serviceNames[nameIdx];
        size_t offset = nameIdx * types.size();
        for (size_t i = 0; i < types.size(); i++)
        {
            query[offset + i].name = name.c_str();
            query[offset + i].length = name.size();
            query[offset + i].type = types[i];
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
    device.ipv6Address = data.AAAA;

    for (const auto& prop : data.TXT)
        device.properties.insert({prop.first, prop.second});

    return device;
}

inline bool MDNSDiscoveryClient::isValidMdnsDevice(const MdnsDiscoveredDevice& device)
{
    return device.ipv4Address.size() > 0 || device.ipv6Address.size() > 0;
}

inline int MDNSDiscoveryClient::queryCallback(int sock,
                                              const sockaddr* from,
                                              size_t addrlen,
                                              mdns_entry_type_t entry,
                                              uint16_t query_id,
                                              uint16_t rtype,
                                              uint16_t rclass,
                                              uint32_t ttl,
                                              const void* data,
                                              size_t size,
                                              size_t name_offset,
                                              size_t name_length,
                                              size_t record_offset,
                                              size_t record_length,
                                              void* user_data)
{
    char nameBuffer[256];
    mdns_record_txt_t txtbuffer[128];

    std::string deviceAddr = ipAddressToString(from, addrlen);
    std::string deviceAddrNoPort = ipAddressToString(from, addrlen, false);

    std::lock_guard lg(devicesMapLock);

    auto it = devicesMap.insert({deviceAddr, DeviceData{}});
    DeviceData& deviceData = it.first->second;

    if (from->sa_family == AF_INET6 && deviceData.AAAA.empty())
        deviceData.AAAA = deviceAddrNoPort;
    else if (from->sa_family == AF_INET && deviceData.A.empty())
        deviceData.A = deviceAddrNoPort;

    if (rtype == MDNS_RECORDTYPE_PTR)
    {
        mdns_string_t namestr = mdns_record_parse_ptr(data, size, record_offset, record_length, nameBuffer, sizeof(nameBuffer));
        deviceData.PTR = std::string(namestr.str, namestr.length);
    }
    else if (rtype == MDNS_RECORDTYPE_SRV)
    {
        mdns_record_srv_t srv = mdns_record_parse_srv(data, size, record_offset, record_length, nameBuffer, sizeof(nameBuffer));
        deviceData.SRV = SRVRecord{std::string(srv.name.str, srv.name.length), srv.priority, srv.weight, srv.port};
    }
    else if (rtype == MDNS_RECORDTYPE_A)
    {
        sockaddr_in addr;
        mdns_record_parse_a(data, size, record_offset, record_length, &addr);
        deviceData.A = ipv4AddressToString(&addr, sizeof(addr));
    }
    else if (rtype == MDNS_RECORDTYPE_AAAA)
    {
        sockaddr_in6 addr;
        mdns_record_parse_aaaa(data, size, record_offset, record_length, &addr);
        deviceData.AAAA = ipv6AddressToString(&addr, sizeof(addr));
    }
    else if (rtype == MDNS_RECORDTYPE_TXT)
    {
        deviceData.TXT.clear();
        size_t parsed =
            mdns_record_parse_txt(data, size, record_offset, record_length, txtbuffer, sizeof(txtbuffer) / sizeof(mdns_record_txt_t));
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

inline void MDNSDiscoveryClient::sendMdnsQuery()
{
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
        queryId[isock] = mdns_multiquery_send(sockets[isock], query.data(), query.size(), buffer, capacity, 0);

    auto callback = [&](int sock,
                        const sockaddr* from,
                        size_t addrlen,
                        mdns_entry_type_t entry,
                        uint16_t query_id,
                        uint16_t rtype,
                        uint16_t rclass,
                        uint32_t ttl,
                        const void* data,
                        size_t size,
                        size_t name_offset,
                        size_t name_length,
                        size_t record_offset,
                        size_t record_length,
                        void* user_data) -> int
    {
        return queryCallback(sock,
                             from,
                             addrlen,
                             entry,
                             query_id,
                             rtype,
                             rclass,
                             ttl,
                             data,
                             size,
                             name_offset,
                             name_length,
                             record_offset,
                             record_length,
                             user_data);
    };

    auto callbackWrapper = [](int sock,
                              const sockaddr* from,
                              size_t addrlen,
                              mdns_entry_type_t entry,
                              uint16_t query_id,
                              uint16_t rtype,
                              uint16_t rclass,
                              uint32_t ttl,
                              const void* data,
                              size_t size,
                              size_t name_offset,
                              size_t name_length,
                              size_t record_offset,
                              size_t record_length,
                              void* user_data) -> int
    {
        return (*static_cast<decltype(callback)*>(user_data)) (sock,
                                                               from,
                                                               addrlen,
                                                               entry,
                                                               query_id,
                                                               rtype,
                                                               rclass,
                                                               ttl,
                                                               data,
                                                               size,
                                                               name_offset,
                                                               name_length,
                                                               record_offset,
                                                               record_length,
                                                               nullptr);
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
