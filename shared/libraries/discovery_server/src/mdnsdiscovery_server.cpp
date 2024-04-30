
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

#include <discovery_server/mdnsdiscovery_server.h>
#ifdef _WIN32
    #define _CRT_SECURE_NO_WARNINGS 1
#endif

BEGIN_NAMESPACE_DISCOVERY_SERVICE

MdnsDiscoveredDevice::MdnsDiscoveredDevice(const std::string& serviceName, uint32_t servicePort, const std::unordered_map<std::string, std::string>& properties)
    : serviceName(serviceName)
    , servicePort(servicePort)
    , properties(std::move(properties))
{
    if (this->serviceName.back() != '.')
        this->serviceName += ".";
}

void MdnsDiscoveredDevice::populateRecords(std::vector<mdns_record_t>& records) const 
{
    for (const auto & [key, value] : properties)
    {
        mdns_record_t record;
        record.name = {serviceInstance.c_str(), serviceInstance.size()},
        record.type = MDNS_RECORDTYPE_TXT,
        record.data.txt.key = {key.c_str(), key.size()},
        record.data.txt.value = {value.c_str(), value.size()},
        record.rclass = 0,
        record.ttl = 0;
        records.push_back(record);
    }
}

std::string MDNSDiscoveryServer::getHostname() 
{
    char hostname_buffer[256];
    std::string hostname;

#ifdef _WIN32
    DWORD hostname_size = sizeof(hostname_buffer);
    if (GetComputerNameA(hostname_buffer, &hostname_size)) {
        hostname = hostname_buffer;
    }
#else
    if (gethostname(hostname_buffer, sizeof(hostname_buffer)) == 0) {
        hostname = hostname_buffer;
    }
#endif

    return hostname;
}
    
MDNSDiscoveryServer::MDNSDiscoveryServer(void)
{
#ifdef _WIN32
    WORD versionWanted = MAKEWORD(1, 1);
    WSADATA wsaData;
    if (WSAStartup(versionWanted, &wsaData))
    {
        printf("Failed to initialize WinSock\n");
    }
#endif
    hostName = getHostname();
    openServerSockets(sockets);
}

void MDNSDiscoveryServer::start()
{
    if (!running)
    {
        running = true;
        serviceThread = std::thread(&MDNSDiscoveryServer::serviceLoop, this);
    }
}

mdns_record_t MDNSDiscoveryServer::createPtrRecord(const MdnsDiscoveredDevice& device) const
{
    mdns_record_t recordPtr;
    recordPtr.name = {device.serviceName.c_str(), device.serviceName.size()},
    recordPtr.type = MDNS_RECORDTYPE_PTR,
    recordPtr.data.ptr.name = {device.serviceInstance.c_str(), device.serviceInstance.size()},
    recordPtr.rclass = 0,
    recordPtr.ttl = 0;
    return recordPtr;
}
mdns_record_t MDNSDiscoveryServer::createSrvRecord(const MdnsDiscoveredDevice& device) const
{

    mdns_record_t recordSrv;
    recordSrv.name = {device.serviceInstance.c_str(), device.serviceInstance.size()},
    recordSrv.type = MDNS_RECORDTYPE_SRV,
    recordSrv.data.srv.name = {device.serviceQualified.c_str(), device.serviceQualified.size()},
    recordSrv.data.srv.port = device.servicePort,
    recordSrv.data.srv.priority = 0,
    recordSrv.data.srv.weight = 0,
    recordSrv.rclass = 0,
    recordSrv.ttl = 0;
    return recordSrv;
}
mdns_record_t MDNSDiscoveryServer::createARecord(const MdnsDiscoveredDevice& device) const
{
    mdns_record_t recordA;
    recordA.name = {device.serviceQualified.c_str(), device.serviceQualified.size()},
    recordA.type = MDNS_RECORDTYPE_A,
    recordA.data.a.addr = serviceAddressIpv4,
    recordA.rclass = 0,
    recordA.ttl = 0;
    return recordA;
}
mdns_record_t MDNSDiscoveryServer::createAaaaRecord(const MdnsDiscoveredDevice& device) const
{
    mdns_record_t recordAAA;
    recordAAA.name = {device.serviceQualified.c_str(), device.serviceQualified.size()},
    recordAAA.type = MDNS_RECORDTYPE_AAAA,
    recordAAA.data.aaaa.addr = serviceAddressIpv6,
    recordAAA.rclass = 0,
    recordAAA.ttl = 0;
    return recordAAA;
}

void MDNSDiscoveryServer::addDevice(const std::string& id, MdnsDiscoveredDevice& device)
{
    device.serviceInstance = hostName + "." + device.serviceName;
    device.serviceQualified = hostName + ".local.";

    std::vector<mdns_record_t> records;
    records.reserve(device.properties.size() + 3);
    records.push_back(createSrvRecord(device));
    if (serviceAddressIpv4.sin_family == AF_INET)
        records.push_back(createARecord(device));
    if (serviceAddressIpv6.sin6_family == AF_INET6)
        records.push_back(createAaaaRecord(device));
    device.populateRecords(records);

    std::vector<char> buffer(2048);
    for (const auto & socket : sockets)
    {
        mdns_announce_multicast(socket, buffer.data(), buffer.size(), createPtrRecord(device), 0, 0, records.data(), records.size());
    }

    std::lock_guard<std::mutex> lock(mx);
    devices.emplace(id, device);

    start();
}

void MDNSDiscoveryServer::removeDevice(const std::string& id)
{
    std::lock_guard<std::mutex> lock(mx);
    auto it = devices.find(id);
    if (it != devices.end())
    {
        auto& device = it->second;
        std::vector<mdns_record_t> records;
        records.reserve(device.properties.size() + 3);
        records.push_back(createSrvRecord(device));
        if (serviceAddressIpv4.sin_family == AF_INET)
            records.push_back(createARecord(device));
        if (serviceAddressIpv6.sin6_family == AF_INET6)
            records.push_back(createAaaaRecord(device));
        device.populateRecords(records);

        std::vector<char> buffer(2048);
        for (const auto & socket : sockets)
        {
            mdns_goodbye_multicast(socket, buffer.data(), buffer.size(), createPtrRecord(device), 0, 0, records.data(), records.size());
        }

        devices.erase(it);
    }

    if (devices.empty())
        stop();
}

void MDNSDiscoveryServer::stop()
{
    if (running == false)
        return;

    running = false;
    if (serviceThread.joinable())
        serviceThread.join();

    for (const auto & [_, device] : devices)
    {
        std::vector<mdns_record_t> records;
        records.reserve(device.properties.size() + 3);
        records.push_back(createSrvRecord(device));
        if (serviceAddressIpv4.sin_family == AF_INET)
            records.push_back(createARecord(device));
        if (serviceAddressIpv6.sin6_family == AF_INET6)
            records.push_back(createAaaaRecord(device));
        device.populateRecords(records);

        std::vector<char> buffer(2048);
        for (const auto & socket : sockets)
        {
            mdns_goodbye_multicast(socket, buffer.data(), buffer.size(), createPtrRecord(device), 0, 0, records.data(), records.size());
        }
    }
}

MDNSDiscoveryServer::~MDNSDiscoveryServer(void)
{
    stop();
    
    for (const auto & socket : sockets)
        mdns_socket_close(socket);

#ifdef _WIN32
    WSACleanup();
#endif
}

void MDNSDiscoveryServer::serviceLoop()
{
    auto callback = [this](int sock,
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
        return serviceCallback(sock,
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

    std::vector<char> buffer(2048);
    while (running) 
    {
        int nfds = 0;
        fd_set readfs;
        FD_ZERO(&readfs);
        for (const auto & socket : sockets)
        {
            if (socket >= nfds)
                nfds = socket + 1;
            FD_SET((u_int) socket, &readfs);
        }

        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;

        if (select(nfds, &readfs, 0, 0, &timeout) >= 0) 
        {
            for (const auto & socket : sockets)
            {
                if (FD_ISSET(socket, &readfs))
                {
                    mdns_socket_listen(socket, buffer.data(), buffer.size(), callbackWrapper, &callback);
                }
                FD_SET((u_int) socket, &readfs);
            }
        } 
        else 
        {
            break;
        }
    }
}

void MDNSDiscoveryServer::openServerSockets(std::vector<int>& sockets) {
    sockets.reserve(2);
    openClientSockets();
    
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
}

inline void MDNSDiscoveryServer::openClientSockets()
{
    bool hasIpv4 = false;
    bool hasIpv6 = false;
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
            if (!hasIpv4 && unicast->Address.lpSockaddr->sa_family == AF_INET)
            {
                auto saddr = reinterpret_cast<sockaddr_in*>(unicast->Address.lpSockaddr);
                
                if ((saddr->sin_addr.S_un.S_un_b.s_b1 != 127) || (saddr->sin_addr.S_un.S_un_b.s_b2 != 0) ||
                    (saddr->sin_addr.S_un.S_un_b.s_b3 != 0) || (saddr->sin_addr.S_un.S_un_b.s_b4 != 1))
                {
                    serviceAddressIpv4 = *saddr;
                    hasIpv4 = true;
                }
            }
            else if (!hasIpv6 && unicast->Address.lpSockaddr->sa_family == AF_INET6)
            {
                auto saddr = reinterpret_cast<sockaddr_in6*>(unicast->Address.lpSockaddr);
                
                const unsigned char localhost[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
                const unsigned char localhostMapped[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff, 0x7f, 0, 0, 1};
                if ((unicast->DadState == NldsPreferred) && memcmp(saddr->sin6_addr.s6_addr, localhost, 16) &&
                    memcmp(saddr->sin6_addr.s6_addr, localhostMapped, 16))
                {
                    serviceAddressIpv6 = *saddr;
                    hasIpv6 = true;
                }
            }
        }
        if (hasIpv4 && hasIpv6)
            break;
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
        
        if (!hasIpv4 && ifa->ifa_addr->sa_family == AF_INET)
        {
            struct sockaddr_in* saddr = (struct sockaddr_in*) ifa->ifa_addr;
            if (saddr->sin_addr.s_addr != htonl(INADDR_LOOPBACK))
            {
                serviceAddressIpv4 = *saddr;
                hasIpv4 = true;
            }
        }
        else if (!hasIpv6 && ifa->ifa_addr->sa_family == AF_INET6)
        {
            struct sockaddr_in6* saddr = (struct sockaddr_in6*) ifa->ifa_addr;
            static const unsigned char localhost[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
            static const unsigned char localhost_mapped[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff, 0x7f, 0, 0, 1};
            if (memcmp(saddr->sin6_addr.s6_addr, localhost, 16) && memcmp(saddr->sin6_addr.s6_addr, localhost_mapped, 16))
            {
                serviceAddressIpv6 = *saddr;
                hasIpv6 = true;
            }
        }
    
        if (hasIpv4 && hasIpv6)
            break;
    }
    
    freeifaddrs(ifaddr);
#endif
}

void send_mdns_query_answer(bool unicast, int sock, const sockaddr* from, socklen_t addrlen, 
                            std::vector<char>& buffer, uint16_t query_id, uint16_t rtype, 
                            const std::string& name, mdns_record_t answer, const std::vector<mdns_record_t>& records) 
{
    if (unicast) 
    {
        mdns_query_answer_unicast(sock, from, addrlen, buffer.data(), buffer.size(),
                                  query_id, mdns_record_type(rtype), name.c_str(), name.size(), answer, 0, 0, records.data(), records.size());
    } 
    else 
    {
        mdns_query_answer_multicast(sock, buffer.data(), buffer.size(), answer, 0, 0, records.data(), records.size());
    }
}

int MDNSDiscoveryServer::serviceCallback(int sock, const sockaddr* from, size_t addrlen, mdns_entry_type_t entry,
                 uint16_t query_id, uint16_t rtype, uint16_t rclass, uint32_t ttl, const void* data,
                 size_t size, size_t name_offset, size_t name_length, size_t record_offset,
                 size_t record_length, void* user_data) 
{
    (void)sizeof(ttl);
    if (entry != MDNS_ENTRYTYPE_QUESTION)
        return 0;

    std::string dns_sd = "_services._dns-sd._udp.local.";
    size_t offset = name_offset;

    std::vector<char> nameBuffer(256);
    std::string name;
    {
      mdns_string_t nameTmp = mdns_string_extract(data, size, &offset, nameBuffer.data(), nameBuffer.size());
      name = std::string(MDNS_STRING_ARGS(nameTmp));
    }
    
    std::vector<char>sendBuffer(1024);

    std::lock_guard lock(mx);

    uint16_t unicast = (rclass & MDNS_UNICAST_RESPONSE);

    for (const auto& [_, device]: devices)
    {
        auto serviceName = device.serviceName;
        if (name == dns_sd) 
        {
            if ((rtype == MDNS_RECORDTYPE_PTR) || (rtype == MDNS_RECORDTYPE_ANY)) 
            {
                mdns_record_t answer;
                answer.name = {name.c_str(), name.size()}, 
                answer.type = MDNS_RECORDTYPE_PTR, 
                answer.data.ptr.name = {serviceName.c_str(), serviceName.size()};

                send_mdns_query_answer(unicast, sock, from, addrlen, sendBuffer, query_id, rtype, name, answer, {});
            }
        } 
        else if (name == serviceName) 
        {
            if ((rtype == MDNS_RECORDTYPE_PTR) || (rtype == MDNS_RECORDTYPE_ANY)) 
            {
                mdns_record_t answer = createPtrRecord(device);

                std::vector<mdns_record_t> records;
                records.reserve(device.properties.size() + 3);

                records.push_back(createSrvRecord(device));
                if (serviceAddressIpv4.sin_family == AF_INET)
                    records.push_back(createARecord(device));
                if (serviceAddressIpv6.sin6_family == AF_INET6)
                    records.push_back(createAaaaRecord(device));
                device.populateRecords(records);

                send_mdns_query_answer(unicast, sock, from, addrlen, sendBuffer, query_id, rtype, name, answer, records);
            }
        } 
        else if (name == device.serviceInstance) 
        {
            if ((rtype == MDNS_RECORDTYPE_SRV) || (rtype == MDNS_RECORDTYPE_ANY)) 
            {
                mdns_record_t answer = createSrvRecord(device);

                std::vector<mdns_record_t> records;
                records.reserve(device.properties.size() + 2);

                if (serviceAddressIpv4.sin_family == AF_INET)
                    records.push_back(createARecord(device));
                if (serviceAddressIpv6.sin6_family == AF_INET6)
                    records.push_back(createAaaaRecord(device));
                device.populateRecords(records);

                send_mdns_query_answer(unicast, sock, from, addrlen, sendBuffer, query_id, rtype, name, answer, records);
            }
        } 
        else if (name == device.serviceQualified) 
        {
            if (((rtype == MDNS_RECORDTYPE_A) || (rtype == MDNS_RECORDTYPE_ANY)) && (serviceAddressIpv4.sin_family == AF_INET)) 
            {
                mdns_record_t answer = createARecord(device);

                std::vector<mdns_record_t> records;
                records.reserve(device.properties.size() + 1);

                if (serviceAddressIpv6.sin6_family == AF_INET6)
                    records.push_back(answer);
                device.populateRecords(records);

                send_mdns_query_answer(unicast, sock, from, addrlen, sendBuffer, query_id, rtype, name, answer, records);
            } 
            else if (((rtype == MDNS_RECORDTYPE_AAAA) || (rtype == MDNS_RECORDTYPE_ANY)) && (serviceAddressIpv6.sin6_family == AF_INET6)) 
            {
                mdns_record_t answer = createAaaaRecord(device);

                std::vector<mdns_record_t> records;
                records.reserve(device.properties.size() + 1);

                if (serviceAddressIpv4.sin_family == AF_INET)
                    records.push_back(answer);
                device.populateRecords(records);

                send_mdns_query_answer(unicast, sock, from, addrlen, sendBuffer, query_id, rtype, name, answer, records);
            }
        }
    }
    return 0;
}

END_NAMESPACE_DISCOVERY_SERVICE