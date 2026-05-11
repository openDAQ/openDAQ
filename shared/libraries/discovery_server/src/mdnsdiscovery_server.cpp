
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

#include <discovery_server/mdnsdiscovery_server.h>
#include <discovery_common/daq_discovery_common.h>
#include <stdexcept>
#include <opendaq/utils/thread_name.h>

#ifdef _WIN32
    #define _CRT_SECURE_NO_WARNINGS 1
#else
#include <arpa/inet.h>
#endif

template <>
struct fmt::formatter<daq::discovery_server::UniqueQuerier> : fmt::formatter<std::string_view>
{
    template <typename FormatContext>
    auto format(const daq::discovery_server::UniqueQuerier& q, FormatContext& ctx) const
    {
        char host[NI_MAXHOST] = {};
        char service[NI_MAXSERV] = {};

        std::string result;

        if (q.addr.ss_family == AF_INET)
        {
            const auto* addr = reinterpret_cast<const sockaddr_in*>(&q.addr);

            if (getnameinfo(reinterpret_cast<const sockaddr*>(addr),
                            sizeof(sockaddr_in),
                            host,
                            sizeof(host),
                            service,
                            sizeof(service),
                            NI_NUMERICHOST | NI_NUMERICSERV) == 0)
            {
                result = fmt::format("{}:{} (sock={})", host, ntohs(addr->sin_port), q.sock);
            }
        }
        else if (q.addr.ss_family == AF_INET6)
        {
            const auto* addr = reinterpret_cast<const sockaddr_in6*>(&q.addr);

            if (getnameinfo(reinterpret_cast<const sockaddr*>(addr),
                            sizeof(sockaddr_in6),
                            host,
                            sizeof(host),
                            service,
                            sizeof(service),
                            NI_NUMERICHOST | NI_NUMERICSERV) == 0)
            {
                result = fmt::format("[{}]:{} (sock={})", host, ntohs(addr->sin6_port), q.sock);
            }
        }

        if (result.empty())
            result = "<unknown>";

        return fmt::formatter<std::string_view>::format(result, ctx);
    }
};

BEGIN_NAMESPACE_DISCOVERY_SERVICE

MdnsDiscoveredService::MdnsDiscoveredService(const std::string& serviceName,
                                             uint16_t servicePort,
                                             const std::unordered_map<std::string, std::string>& properties,
                                             const daq::PropertyObjectPtr& deviceInfo)
    : serviceName(serviceName)
    , servicePort(servicePort)
    , properties(std::move(properties))
    , deviceInfo(deviceInfo)
{
    if (this->serviceName.back() != '.')
        this->serviceName += ".";

    this->staticRecordSize = 1024;
    for (const auto& prop : deviceInfo.getAllProperties())
    {
        if ((Int)prop.getValueType() <= (Int)CoreType::ctString)
        {
            std::string key = prop.getName();
            if (prop.getReadOnly())
            {
                std::string value = prop.getValue();
                if (value.empty())
                    continue;

                this->properties[key] = value;
                staticRecordSize += key.size() + value.size() + 2;
            }
            else
            {
                dynamicProperties.push_back({key, ""});
            }
        }
    }
}

size_t MdnsDiscoveredService::updateConnectedClientsAndGetPropsCount() const
{
    using namespace discovery_common;

    const PropertyObjectPtr connectedClientsInfo = deviceInfo.getPropertyValue("activeClientConnections");
    connectedClientsProperties = DiscoveryUtils::connectedClientsInfoToTxt(connectedClientsInfo);

    return properties.size() + dynamicProperties.size() + connectedClientsProperties.size();
}

mdns_record_t MdnsDiscoveredService::createTxtRecord(const std::string& key, const std::string& value) const
{
    mdns_record_t record;
    record.name = {serviceInstance.c_str(), serviceInstance.size()},
    record.type = MDNS_RECORDTYPE_TXT,
    record.data.txt.key = {key.c_str(), key.size()},
    record.data.txt.value = {value.c_str(), value.size()},
    record.rclass = 0,
    record.ttl = 0;
    return record;
}

void MdnsDiscoveredService::populateRecords(std::vector<mdns_record_t>& records) const
{
    this->recordSize = this->staticRecordSize;
    for (const auto & [key, value] : properties)
    {
        records.push_back(createTxtRecord(key, value));
    }

    for (auto & [key, value] : dynamicProperties)
    {
        value = (std::string)deviceInfo.getPropertyValue(key);
        if (value.empty())
            continue;

        records.push_back(createTxtRecord(key, value));
        this->recordSize += key.size() + value.size() + 2;
    }

    // connectedClientsProperties updated earlier
    for (const auto& [key, value] : connectedClientsProperties)
    {
        records.push_back(createTxtRecord(key, value));
        this->recordSize += key.size() + value.size() + 2;
    }
}

static size_t toMs(const AtomicSteadyTimePoint::TimePointType& point)
{
    static auto baseTp = AtomicSteadyTimePoint::Clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(point - baseTp).count();
}

std::string MDNSDiscoveryServer::getHostname() 
{
    char hostname_buffer[256];
    std::string hostname;

#ifdef _WIN32
    DWORD hostname_size = sizeof(hostname_buffer);
    if (GetComputerNameA(hostname_buffer, &hostname_size))
    {
        hostname = hostname_buffer;
    }
#else
    if (gethostname(hostname_buffer, sizeof(hostname_buffer)) == 0)
    {
        hostname = hostname_buffer;
    }
#endif

    return hostname;
}
    
MDNSDiscoveryServer::MDNSDiscoveryServer(const DictPtr<IString, IBaseObject>& options)
    : options(options)
    , perQuerierBucketLimit(options.hasKey("RateLimitPerQuerier") ? static_cast<size_t>(options.get("RateLimitPerQuerier")) : 10)
    , maxQueriers(options.hasKey("MaxQueriersLimit") ? static_cast<uint32_t>(options.get("MaxQueriersLimit")) : 100)
    , maxBurst(std::max(options.hasKey("MaxBurst") ? static_cast<size_t>(options.get("MaxBurst")) : 50, perQuerierBucketLimit))
    , querierBucketsTable(maxQueriers)
{
#ifdef _WIN32
    WORD versionWanted = MAKEWORD(1, 1);
    WSADATA wsaData;
    if (WSAStartup(versionWanted, &wsaData))
        throw std::runtime_error("MDNSDiscoveryServer: Failed to initialize WinSock");

#endif
    hostName = getHostname();
    openServerSockets();
}

void MDNSDiscoveryServer::start()
{
    if (!running)
    {
        if (serviceThread.joinable())
            serviceThread.join();

        std::lock_guard<std::mutex> lock(mx);
        if (!running)
        {
            running = true;
            serviceThread = std::thread(&MDNSDiscoveryServer::serviceLoop, this);
        }
    }
}

bool MDNSDiscoveryServer::getAdapter(int sock, AdapterInfo& adapter)
{
    for (const auto& [_, adapter_] : adapters)
    {
        if (adapter_.ipv4Sock == sock || adapter_.ipv6Sock == sock)
        {
            adapter = adapter_;
            return true;
        }
    }

    return false;
}

mdns_record_t MDNSDiscoveryServer::createPtrRecord(const MdnsDiscoveredService& service) const
{
    mdns_record_t recordPtr;
    recordPtr.name = {service.serviceName.c_str(), service.serviceName.size()},
    recordPtr.type = MDNS_RECORDTYPE_PTR,
    recordPtr.data.ptr.name = {service.serviceInstance.c_str(), service.serviceInstance.size()},
    recordPtr.rclass = 0,
    recordPtr.ttl = 0;
    return recordPtr;
}

mdns_record_t MDNSDiscoveryServer::createSrvRecord(const MdnsDiscoveredService& service) const
{
    mdns_record_t recordSrv;
    recordSrv.name = {service.serviceInstance.c_str(), service.serviceInstance.size()},
    recordSrv.type = MDNS_RECORDTYPE_SRV,
    recordSrv.data.srv.name = {service.serviceQualified.c_str(), service.serviceQualified.size()},
    recordSrv.data.srv.port = service.servicePort,
    recordSrv.data.srv.priority = 0,
    recordSrv.data.srv.weight = 0,
    recordSrv.rclass = 0,
    recordSrv.ttl = 0;
    return recordSrv;
}

mdns_record_t MDNSDiscoveryServer::createARecord(const MdnsDiscoveredService& service, const AdapterInfo& info) const
{
    mdns_record_t recordA;
    recordA.name = {service.serviceQualified.c_str(), service.serviceQualified.size()},
    recordA.type = MDNS_RECORDTYPE_A,
    recordA.data.a.addr = info.ipv4Address,
    recordA.rclass = 0,
    recordA.ttl = 0;
    return recordA;
}

mdns_record_t MDNSDiscoveryServer::createAaaaRecord(const MdnsDiscoveredService& service, const AdapterInfo& info) const
{
    mdns_record_t recordAAA;
    recordAAA.name = {service.serviceQualified.c_str(), service.serviceQualified.size()},
    recordAAA.type = MDNS_RECORDTYPE_AAAA,
    recordAAA.data.aaaa.addr = info.ipv6Address,
    recordAAA.rclass = 0,
    recordAAA.ttl = 0;
    return recordAAA;
}

void MDNSDiscoveryServer::populateTxtRecords(const std::string& recordName,
                                             const discovery_common::TxtProperties& props,
                                             std::vector<mdns_record_t>& records) const
{
    for (const auto & [key, value] : props)
    {
        mdns_record_t record;
        record.name = {recordName.c_str(), recordName.size()},
        record.type = MDNS_RECORDTYPE_TXT,
        record.data.txt.key = {key.c_str(), key.size()},
        record.data.txt.value = {value.c_str(), value.size()},
        record.rclass = MDNS_CLASS_IN,
        record.ttl = 0;
        records.push_back(record);
    }
}

void MDNSDiscoveryServer::announceService(const MdnsDiscoveredService& service, bool goodbye) const
{
    auto capacity = service.updateConnectedClientsAndGetPropsCount() + 3;

    for (const auto& [_, adapter] : adapters)
    {
        std::vector<mdns_record_t> records;
        records.reserve(capacity);
        records.push_back(createSrvRecord(service));
        if (adapter.ipv6Sock >= 0)
            records.push_back(createAaaaRecord(service, adapter));
        if (adapter.ipv4Sock >= 0)
            records.push_back(createARecord(service, adapter));

        service.populateRecords(records);
        std::vector<char> buffer(service.recordSize);

        if (!goodbye)
        {
            if (adapter.ipv4Sock >= 0)
            {
                mdns_announce_multicast(adapter.ipv4Sock,
                                        buffer.data(),
                                        buffer.size(),
                                        createPtrRecord(service),
                                        0,
                                        0,
                                        records.data(),
                                        records.size(),
                                        0);
            }
            
            if (adapter.ipv6Sock >= 0)
            {
                mdns_announce_multicast(adapter.ipv6Sock,
                                        buffer.data(),
                                        buffer.size(),
                                        createPtrRecord(service),
                                        0,
                                        0,
                                        records.data(),
                                        records.size(),
                                        adapter.ipv6ifindex);
            }
        }
        else
        {
            if (adapter.ipv4Sock >= 0)
            {
                mdns_goodbye_multicast(adapter.ipv4Sock,
                                        buffer.data(),
                                        buffer.size(),
                                        createPtrRecord(service),
                                        0,
                                        0,
                                        records.data(),
                                        records.size(),
                                        0);
            }
            
            if (adapter.ipv6Sock >= 0)
            {
                mdns_goodbye_multicast(adapter.ipv6Sock,
                                        buffer.data(),
                                        buffer.size(),
                                        createPtrRecord(service),
                                        0,
                                        0,
                                        records.data(),
                                        records.size(),
                                        adapter.ipv6ifindex);
            }
        }
    }
}

bool MDNSDiscoveryServer::registerService(const std::string& id, MdnsDiscoveredService& service)
{
    std::string deviceId = hostName;

    const auto manufacturer = service.properties["manufacturer"];
    const auto serialNumber = service.properties["serialNumber"];
    if (!manufacturer.empty() && !serialNumber.empty())
        deviceId = manufacturer + "_" + serialNumber;
    else
        fprintf(stderr, "MDNSDiscoveryServer: Manufacturer and serial number not provided for service %s. It can cause mdns conflict\n", id.c_str());

    service.serviceInstance = deviceId + "." + service.serviceName;
    service.serviceQualified = deviceId + ".local.";

    bool success;
    {
        std::lock_guard<std::mutex> lock(mx);
        success = services.emplace(id, service).second;
    }
    if (success)
    {
        announceService(service, false);
        start();
    }

    return success;
}

void MDNSDiscoveryServer::goodbyeMulticast(const MdnsDiscoveredService& service) const
{
    announceService(service, true);
}

bool MDNSDiscoveryServer::unregisterService(const std::string& id)
{
    bool success = false;
    std::lock_guard<std::mutex> lock(mx);
    auto it = services.find(id);
    if (it != services.end())
    {
        success = true;
        goodbyeMulticast(it->second);
        services.erase(it);
    }

    return success;
}

bool MDNSDiscoveryServer::registerIpModificationService(MdnsDiscoveredService& service,
                                                        const ModifyIpConfigCallback& modifyIpConfigCb,
                                                        const RetrieveIpConfigCallback& retrieveIpConfigCb)
{
    this->manufacturer = service.properties["manufacturer"];
    this->serialNumber = service.properties["serialNumber"];
    this->modifyIpConfigCallback = modifyIpConfigCb;
    this->retrieveIpConfigCallback = retrieveIpConfigCb;

    return registerService(discovery_common::IpModificationUtils::DAQ_IP_MODIFICATION_SERVICE_ID, service);
}

bool MDNSDiscoveryServer::unregisterIpModificationService()
{
    this->manufacturer = "";
    this->serialNumber = "";
    this->modifyIpConfigCallback = nullptr;
    this->retrieveIpConfigCallback = nullptr;

    return unregisterService(discovery_common::IpModificationUtils::DAQ_IP_MODIFICATION_SERVICE_ID);
}

bool MDNSDiscoveryServer::isServiceRegistered(const std::string& id)
{
    std::lock_guard<std::mutex> lock(mx);
    if (auto it = services.find(id); it != services.end())
        return true;
    return false;
}

void MDNSDiscoveryServer::stop()
{
    running = false;
    if (serviceThread.joinable())
    {
        serviceThread.join();
    }
    for (const auto & [_, service] : services)
        goodbyeMulticast(service);
}

MDNSDiscoveryServer::~MDNSDiscoveryServer(void)
{
    stop();
    
    for (const auto& [_, adapter]: adapters)
    {
        if (adapter.ipv4Sock >= 0)
            mdns_socket_close(adapter.ipv4Sock);
        if (adapter.ipv6Sock >= 0)
            mdns_socket_close(adapter.ipv6Sock);
    }

#ifdef _WIN32
    WSACleanup();
#endif
}

void MDNSDiscoveryServer::serviceLoop()
{
    utils::setThreadName("MDNSDiscovery");

    auto callback = [this](int sock,
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
        return serviceCallback(sock,
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

    std::vector<char> buffer(2048);
    while (running) 
    {
        int nfds = 0;
        fd_set readfs;
        FD_ZERO(&readfs);

        for (const auto& [_, adapter] : adapters)
        {
            if (adapter.ipv4Sock >= 0)
            {
                FD_SET((u_int) adapter.ipv4Sock, &readfs);
                if (adapter.ipv4Sock >= nfds)
                    nfds = adapter.ipv4Sock + 1;
            }
            
            if (adapter.ipv6Sock >= 0)
            {
                FD_SET((u_int) adapter.ipv6Sock, &readfs);
                if (adapter.ipv6Sock >= nfds)
                    nfds = adapter.ipv6Sock + 1;
            }
        }

        timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;

        int ret = select(nfds, &readfs, nullptr, nullptr, &timeout);
        if (ret < 0) {
            // error
            break;
        }
        if (ret == 0) {
            // timeout, nothing ready
            continue;
        }

        for (const auto& [_, adapter] : adapters)
        {
            if (adapter.ipv4Sock >= 0 && FD_ISSET(adapter.ipv4Sock, &readfs))
                mdns_socket_listen(adapter.ipv4Sock, buffer.data(), buffer.size(), callbackWrapper, &callback);
            
            if (adapter.ipv6Sock >= 0 && FD_ISSET(adapter.ipv6Sock, &readfs))
                mdns_socket_listen(adapter.ipv6Sock, buffer.data(), buffer.size(), callbackWrapper, &callback);
        }
    }
}

void MDNSDiscoveryServer::openServerSockets() 
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

                if ((saddr->sin_addr.S_un.S_un_b.s_b1 != 127) ||
                    (saddr->sin_addr.S_un.S_un_b.s_b2 != 0) || 
                    (saddr->sin_addr.S_un.S_un_b.s_b3 != 0) ||
                    (saddr->sin_addr.S_un.S_un_b.s_b4 != 1))
                {
                    sockaddr_in sock_addr{};
                    
                    sock_addr.sin_family = AF_INET;
                    sock_addr.sin_addr = saddr->sin_addr;
                    sock_addr.sin_port = htons(MDNS_PORT);

                    int sock = mdns_socket_open_ipv4(&sock_addr);
                    if (sock >= 0)
                    {
                        ip_mreq mreq{};
                        mreq.imr_multiaddr.s_addr = inet_addr("224.0.0.251");
                        mreq.imr_interface = saddr->sin_addr;

                        // Handle error where ADD_MEMBERSHIP fails, but socket still receives multicast queries
                        if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char*)&mreq, sizeof(mreq)) < 0)
                        {
                            int err = WSAGetLastError();
                            if (err != WSAEADDRINUSE && err != WSAEINVAL)
                            {
                                closesocket(sock);
                                continue;
                            }
                        }

                        if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, (const char*)&saddr->sin_addr, sizeof(saddr->sin_addr)) < 0)
                        {
                            closesocket(sock);
                            continue;
                        }

                        std::string ifname = adapter->AdapterName;
                        AdapterInfo& info = adapters[ifname];
                        info.ipv4Address = *saddr;
                        info.ipv4Sock = sock;
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
                    sockaddr_in6 sock_addr{};
                    sock_addr.sin6_family = AF_INET6;
                    sock_addr.sin6_addr = saddr->sin6_addr;
                    sock_addr.sin6_port = htons(MDNS_PORT);

                    int sock = mdns_socket_open_ipv6(&sock_addr, (unsigned int) adapter->Ipv6IfIndex);
                    if (sock >= 0)
                    {
                        struct ipv6_mreq mreq{};
                        inet_pton(AF_INET6, "ff02::fb", &mreq.ipv6mr_multiaddr);
                        mreq.ipv6mr_interface = adapter->Ipv6IfIndex;

                        if (setsockopt(sock, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, (const char*) &mreq, sizeof(mreq)) < 0)
                        {
                            // Handle error where ADD_MEMBERSHIP fails, but socket still receives multicast queries
                            int err = WSAGetLastError();
                            if (err != WSAEADDRINUSE && err != WSAEINVAL)
                            {
                                closesocket(sock);
                                continue;
                            }
                        }

                        // Set the default outgoing interface for multicast packets
                        DWORD ifindex = adapter->Ipv6IfIndex;
                        if (setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_IF, (const char*) &ifindex, sizeof(ifindex)) < 0)
                        {
                            closesocket(sock);
                            continue;
                        }

                        std::string ifname = adapter->AdapterName;
                        AdapterInfo& info  = adapters[ifname];
                        info.ipv6ifindex = (unsigned int) adapter->Ipv6IfIndex;
                        info.ipv6Address   = *saddr;
                        info.ipv6Sock      = sock;
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
                sockaddr_in sock_addr{};
                sock_addr.sin_family = AF_INET;
                sock_addr.sin_addr = saddr->sin_addr;
                sock_addr.sin_port = htons(MDNS_PORT);

#ifdef __APPLE__
                sock_addr.sin_len = sizeof(sockaddr_in);
#endif

                int sock = mdns_socket_open_ipv4(&sock_addr);
                if (sock >= 0)
                {
                    ip_mreq mreq{};
                    mreq.imr_multiaddr.s_addr = inet_addr("224.0.0.251");
                    mreq.imr_interface = saddr->sin_addr;

                    if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) 
                    {
                        if (errno != EADDRINUSE && errno != EINVAL) 
                        {
                            close(sock);
                            continue;
                        }
                    }

                    // Set the default outgoing address for multicast packets
                    if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, &saddr->sin_addr, sizeof(saddr->sin_addr)) < 0)
                    {               
                        close(sock);
                        continue;
                    }

                    std::string ifname = ifa->ifa_name;
                    AdapterInfo& info = adapters[ifname];
                    info.name = ifname;
                    info.ipv4Address = *saddr;
                    info.ipv4Sock = sock;
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
                sockaddr_in6 sock_addr{};

                sock_addr.sin6_family = AF_INET6;
                sock_addr.sin6_addr = in6addr_any;
                sock_addr.sin6_port = htons(MDNS_PORT);
                sock_addr.sin6_scope_id = if_nametoindex(ifa->ifa_name);

#ifdef __APPLE__
                sock_addr.sin6_len = sizeof(sockaddr_in6);
#endif
                
                unsigned int ifindex = if_nametoindex(ifa->ifa_name);
                int sock = mdns_socket_open_ipv6(&sock_addr, ifindex);
                if (sock >= 0)
                {
                    ipv6_mreq mreq{};
                    inet_pton(AF_INET6, "ff02::fb", &mreq.ipv6mr_multiaddr);
                    mreq.ipv6mr_interface = ifindex;

                    if (setsockopt(sock, IPPROTO_IPV6, IPV6_JOIN_GROUP, &mreq, sizeof(mreq)) < 0)
                    {
                        if (errno != EADDRINUSE && errno != EINVAL) 
                        {
                            close(sock);
                            continue; 
                        }
                    }

                    // Set the default outgoing interface for multicast packets
                    if (setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_IF, &ifindex, sizeof(ifindex)) < 0)
                    {
                        close(sock);
                        continue;
                    }

                    std::string ifname = ifa->ifa_name;
                    AdapterInfo& info = adapters[ifname];
                    info.ipv6ifindex = ifindex;
                    info.ipv6Address = *saddr;
                    info.ipv6Sock = sock;
                }
            }
        }
    }
    
    freeifaddrs(ifaddr);
#endif

    if (adapters.empty())
    {
#ifdef _WIN32
        WSACleanup();
#endif
        throw std::runtime_error("MDNSDiscoveryServer: Network interfaces not found");
    }
}

bool MDNSDiscoveryServer::getAdapter(int sock, AdapterInfo& info) const
{
    for (const auto& [_, adapter] : adapters)
    {
        if (adapter.ipv4Sock == sock || adapter.ipv6Sock == sock)
        {
            info = adapter;
            return true;
        }
    }

    return false;
}

void send_mdns_query_answer(bool unicast, int sock, const sockaddr* from, socklen_t addrlen, 
                            std::vector<char>& buffer, uint16_t query_id, uint16_t rtype, 
                            const std::string& name, mdns_record_t answer, const std::vector<mdns_record_t>& records, AdapterInfo& adapter) 
{
    if (unicast) 
    {
        mdns_query_answer_unicast(sock, from, addrlen, buffer.data(), buffer.size(),
                                  query_id, mdns_record_type(rtype), name.c_str(), name.size(), answer, 0, 0, records.data(), records.size());
    } 
    else 
    {
        mdns_query_answer_multicast(sock, buffer.data(), buffer.size(), answer, 0, 0, records.data(), records.size(), adapter.ipv6ifindex);
    }
}

std::string rtypeToString(mdns_record_type rtype)
{
    switch(rtype)
    {
        case MDNS_RECORDTYPE_PTR:
            return "PTR";
        case MDNS_RECORDTYPE_SRV:
            return "SRV";
        case MDNS_RECORDTYPE_A:
            return "A";
        case MDNS_RECORDTYPE_AAAA:
            return "AAAA";
        case MDNS_RECORDTYPE_TXT:
            return "TXT";
        case MDNS_RECORDTYPE_ANY:
            return "ANY";
        default:
            return "UNKNOWN";
    }
}

int MDNSDiscoveryServer::serviceCallback(
    int sock, const sockaddr* from, size_t addrlen, mdns_entry_type_t entry,
    uint16_t query_id, uint16_t rtype, uint16_t rclass, uint32_t ttl, const void* buffer,
    size_t size, size_t name_offset, size_t name_length, size_t rdata_offset,
    size_t rdata_length, void* user_data, uint8_t opcode)
{
    (void)sizeof(ttl);

    if (opcode)
        return nonDiscoveryCallback(sock, from, addrlen, entry,
                                    query_id, rtype, rclass, buffer,
                                    size, name_offset, name_length, rdata_offset,
                                    rdata_length, user_data, opcode);
    else
        return discoveryCallback(sock, from, addrlen, entry,
                                 query_id, rtype, rclass, buffer,
                                 size, name_offset, name_length, rdata_offset,
                                 rdata_length, user_data);
}

void MDNSDiscoveryServer::sendIpConfigResponse(int sock,
                                               const sockaddr* to,
                                               size_t addrlen,
                                               uint16_t query_id,
                                               discovery_common::TxtProperties& resProps,
                                               uint8_t opcode,
                                               bool unicast,
                                               const std::string& uuid)
{
    resProps["uuid"] = uuid;

    std::vector<mdns_record_t> txtRecords;
    std::string recordName(discovery_common::IpModificationUtils::DAQ_IP_MODIFICATION_SERVICE_NAME);
    populateTxtRecords(recordName, resProps, txtRecords);

    std::vector<char>sendBuffer(2048);

    unsigned int if_index = 0;
    AdapterInfo adapter;
    if (getAdapter(sock, adapter))
        if_index = adapter.ipv6ifindex;
    non_mdns_query_send(sock, sendBuffer.data(), sendBuffer.size(), query_id, opcode, 0x1, 0, 0, txtRecords.data(), txtRecords.size(), 0, 0, 0, 0, unicast ? 0x1 : 0x0, to, addrlen, if_index);
}

static uint32_t xxHash32(const UniqueQuerier& querier)
{
    XXH32_state_t* state = XXH32_createState();
    XXH32_reset(state, 123456789);

    XXH32_update(state, &querier.sock, sizeof(querier.sock));
    XXH32_update(state, &querier.addr.ss_family, sizeof(querier.addr.ss_family));

    if (querier.addr.ss_family == AF_INET)
    {
        auto* addr = reinterpret_cast<const sockaddr_in*>(&querier.addr);
        XXH32_update(state, &addr->sin_addr, sizeof(addr->sin_addr));
        XXH32_update(state, &addr->sin_port, sizeof(addr->sin_port));
    }
    else if (querier.addr.ss_family == AF_INET6)
    {
        auto* addr = reinterpret_cast<const sockaddr_in6*>(&querier.addr);
        XXH32_update(state, &addr->sin6_addr, sizeof(addr->sin6_addr));
        XXH32_update(state, &addr->sin6_port, sizeof(addr->sin6_port));
    }

    uint32_t result = XXH32_digest(state);
    XXH32_freeState(state);
    return result;
}

static inline uint32_t mulHigh32(uint32_t a, uint32_t b)
{
    uint32_t a_hi = a >> 16;
    uint32_t a_lo = a & 0xFFFF;
    uint32_t b_hi = b >> 16;
    uint32_t b_lo = b & 0xFFFF;

    uint32_t p0 = a_lo * b_lo;
    uint32_t p1 = a_lo * b_hi;
    uint32_t p2 = a_hi * b_lo;
    uint32_t p3 = a_hi * b_hi;

    uint32_t carry = (p0 >> 16) + (p1 & 0xFFFF) + (p2 & 0xFFFF);

    return p3 + (p1 >> 16) + (p2 >> 16) + (carry >> 16);
}

static inline uint32_t hashToIndex(uint32_t hash, uint32_t indexLimit)
{
    uint32_t x = hash;

    x ^= x >> 16;
    x *= 0x7feb352d;
    x ^= x >> 15;
    x *= 0x846ca68b;
    x ^= x >> 16;

    return mulHigh32(x, indexLimit);
}

bool MDNSDiscoveryServer::allowQuery(int sock, const sockaddr* from)
{
    sockaddr_storage storage{};
    if (from->sa_family == AF_INET)
        std::memcpy(&storage, from, sizeof(sockaddr_in));
    else
        std::memcpy(&storage, from, sizeof(sockaddr_in6));

    UniqueQuerier querier{storage, sock};
    const auto now = AtomicSteadyTimePoint::Clock::now();

    const uint32_t hash = xxHash32(querier);

    const uint32_t bucketIndex = hashToIndex(hash, maxQueriers);
    auto& bucketSlot = querierBucketsTable[bucketIndex];

    // Empty slot
    if (!bucketSlot.has_value())
    {
        bucketSlot.emplace(
            querier,
            hash,
            maxBurst,
            perQuerierBucketLimit,
            now);

        //fmt::print("Arrived query from: {} at {}; hash {}, slot index {}", querier, toMs(now), hash, bucketIndex);
        //fmt::print("...allowed as completely new querier, tokens {}\n", bucketSlot->tokens.load(std::memory_order_relaxed));
        return true;
    }

    auto& bucket = *bucketSlot;
    AtomicSteadyTimePoint::TimePointType lastQueried = bucket.lastQueried;

    // Different querier mapped to same slot
    if (!(bucket.primaryQuerierHash == hash && bucket.primaryQuerier == querier))
    {
        // Replace stale bucket
        if (isStaleBucket(bucket, now))
        {
            fmt::print("Arrived query from: {} at {}; hash {}, slot index {}", querier, toMs(now), hash, bucketIndex);
            fmt::print("...allowed as replacement querier for {} last queried at {}; hash {}, slot index {}\n", bucket.primaryQuerier, toMs(lastQueried), bucket.primaryQuerierHash, bucketIndex);
            bucketSlot.emplace(
                querier,
                hash,
                maxBurst,
                perQuerierBucketLimit,
                now);

            return true;
        }

        // Share bucket
        fmt::print("Arrived query from: {} at {}; hash {}, slot index {}", querier, toMs(now), hash, bucketIndex);
        fmt::print("...will share bucket with {} last queried at {}; hash {}, slot index {}\n", bucket.primaryQuerier, toMs(lastQueried), bucket.primaryQuerierHash, bucketIndex);
        bucketSlot.emplace(
            querier,
            hash,
            maxBurst,
            perQuerierBucketLimit,
            now);
    }

    refillTokens(bucket, now);

    bool result = tryConsumeToken(bucket, now);
    if (result)
    {
        //fmt::print("Arrived query from: {} at {}; hash {}, slot index {}", querier, toMs(now), hash, bucketIndex);
        //fmt::print("...allowed bcs tokens available for {} last queried at {}; hash {}, slot index {}, tokens {}\n", bucket.querier, toMs(lastQueried), bucket.hashFingerprint, bucketIndex, bucket.tokens.load(std::memory_order_relaxed));
    }
    else
    {
        fmt::print("Arrived query from: {} at {}; hash {}, slot index {}", querier, toMs(now), hash, bucketIndex);
        fmt::print("...rejected bcs tokens are not available for {} last queried at {}; hash {}, slot index {}, tokens {}\n", bucket.primaryQuerier, toMs(lastQueried), bucket.primaryQuerierHash, bucketIndex, bucket.tokens.load(std::memory_order_relaxed));
    }
    return result;
}

void MDNSDiscoveryServer::refillTokens(QuerierBucket& bucket, AtomicSteadyTimePoint::TimePointType now)
{
    const AtomicSteadyTimePoint::TimePointType lastQueried = bucket.lastQueried;
    const auto elapsed = now - lastQueried;
    const auto elapsedSeconds = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();

    if (elapsedSeconds == 0)
        return;

    const size_t refillAmount = elapsedSeconds * perQuerierBucketLimit;
    const size_t currentTokens = bucket.tokens.load(std::memory_order_relaxed);
    const size_t newTokenCount = std::min(currentTokens + refillAmount, maxBurst);

    bucket.tokens.store(newTokenCount, std::memory_order_relaxed);
    bucket.lastQueried = now;
}

bool MDNSDiscoveryServer::tryConsumeToken(QuerierBucket& bucket, AtomicSteadyTimePoint::TimePointType now)
{
    auto currentTokens = bucket.tokens.load(std::memory_order_relaxed);

    if (currentTokens == 0)
        return false;

    bucket.tokens.store(currentTokens - 1, std::memory_order_relaxed);
    bucket.lastQueried = now;

    return true;
}

bool MDNSDiscoveryServer::isStaleBucket(const QuerierBucket& bucket, AtomicSteadyTimePoint::TimePointType now) const
{
    constexpr auto bucketTimeout = std::chrono::seconds(10);
    AtomicSteadyTimePoint::TimePointType lastQueried = bucket.lastQueried;
    const auto inactiveDuration = now - lastQueried;
    return inactiveDuration > bucketTimeout;
}

int MDNSDiscoveryServer::nonDiscoveryCallback(
    int sock, const sockaddr* from, size_t addrlen, mdns_entry_type_t entry,
    uint16_t query_id, uint16_t rtype, uint16_t rclass, const void* buffer,
    size_t size, size_t name_offset, size_t name_length, size_t rdata_offset,
    size_t rdata_length, void* user_data, uint8_t opcode)
{
    using namespace discovery_common;

    if (entry != MDNS_ENTRYTYPE_QUESTION)
        return 0;

    if (isServiceRegistered(IpModificationUtils::DAQ_IP_MODIFICATION_SERVICE_ID) &&
        DiscoveryUtils::extractRecordName(buffer, name_offset, size) == IpModificationUtils::DAQ_IP_MODIFICATION_SERVICE_NAME &&
        rtype == MDNS_RECORDTYPE_TXT &&
        (opcode == IpModificationUtils::IP_MODIFICATION_OPCODE || opcode == IpModificationUtils::IP_GET_CONFIG_OPCODE))
    {
        uint16_t responseUnicast = (rclass & MDNS_UNICAST_RESPONSE);
        TxtProperties reqProps = DiscoveryUtils::readTxtRecord(size, buffer, rdata_offset, rdata_length);

        // manufacturer & serialNumber pair is not specified or does not match those values of the device - ignore request
        if (const auto it = reqProps.find("manufacturer"); it == reqProps.end() || it->second != manufacturer)
            return 0;
        if (const auto it = reqProps.find("serialNumber"); it == reqProps.end() || it->second != serialNumber)
            return 0;

        std::string clientUuid;
        if (const auto it = reqProps.find("uuid"); it == reqProps.end())
        {
            // uuid is not specified - ignore request
            return 0;
        }
        else
        {
            clientUuid = it->second;
            // ignore already processed requests
            std::string requestId = clientUuid + std::to_string(query_id);
            if(const auto requestIdIt = processedIpConfigReqIds.find(requestId); requestIdIt != processedIpConfigReqIds.end())
                return 0;
            else
                processedIpConfigReqIds.insert(requestId);
        }

        if (const auto it = reqProps.find("ifaceName"); it == reqProps.end())
        {
            return 0;
        }
        else
        {
            std::string interfaceName = it->second;
            if (opcode == IpModificationUtils::IP_MODIFICATION_OPCODE && modifyIpConfigCallback)
            {
                TxtProperties resProps = modifyIpConfigCallback(interfaceName, reqProps);
                sendIpConfigResponse(sock, from, addrlen, query_id, resProps, opcode, responseUnicast, clientUuid);
            }
            else if (opcode == IpModificationUtils::IP_GET_CONFIG_OPCODE && retrieveIpConfigCallback)
            {
                TxtProperties resProps = retrieveIpConfigCallback(interfaceName);
                resProps["manufacturer"] = manufacturer;
                resProps["serialNumber"] = serialNumber;
                resProps["ifaceName"] = interfaceName;
                sendIpConfigResponse(sock, from, addrlen, query_id, resProps, opcode, responseUnicast, clientUuid);
            }
        }
    }

    return 0;
}

int MDNSDiscoveryServer::discoveryCallback(
    int sock, const sockaddr* from, size_t addrlen, mdns_entry_type_t entry,
    uint16_t query_id, uint16_t rtype, uint16_t rclass, const void* buffer,
    size_t size, size_t name_offset, size_t name_length, size_t rdata_offset,
    size_t rdata_length, void* user_data)
{
    if (!allowQuery(sock, from))
        return 0;

    if (entry != MDNS_ENTRYTYPE_QUESTION)
        return 0;

    AdapterInfo adapter;
    if (!getAdapter(sock, adapter))
        return 0;

    std::string dns_sd = "_services._dns-sd._udp.local.";
    std::string name = discovery_common::DiscoveryUtils::extractRecordName(buffer, name_offset, size);

    auto recordTypeName = rtypeToString(mdns_record_type(rtype));
    if (recordTypeName == "UNKNOWN")
        return 0;

    std::lock_guard lock(mx);
    if (services.empty())
    {
        running = false;
        return 0;
    }

    uint16_t unicast = (rclass & MDNS_UNICAST_RESPONSE);

    for (const auto& [_, service]: services)
    {
        auto serviceName = service.serviceName;
        if (name == dns_sd) 
        {
            if ((rtype == MDNS_RECORDTYPE_PTR) || (rtype == MDNS_RECORDTYPE_ANY)) 
            {
                mdns_record_t answer;
                answer.name = {name.c_str(), name.size()}, 
                answer.type = MDNS_RECORDTYPE_PTR, 
                answer.data.ptr.name = {serviceName.c_str(), serviceName.size()};

                std::vector<char>sendBuffer(1024);
                send_mdns_query_answer(unicast, sock, from, addrlen, sendBuffer, query_id, rtype, name, answer, {}, adapter);
            }
        } 
        else if (name == serviceName) 
        {
            if ((rtype == MDNS_RECORDTYPE_PTR) || (rtype == MDNS_RECORDTYPE_ANY)) 
            {
                mdns_record_t answer = createPtrRecord(service);

                std::vector<mdns_record_t> records;
                records.reserve(service.updateConnectedClientsAndGetPropsCount() + 3);

                records.push_back(createSrvRecord(service));
                if (adapter.ipv4Sock >= 0 && from->sa_family == AF_INET)
                    records.push_back(createARecord(service, adapter));
                if (adapter.ipv6Sock >= 0)
                    records.push_back(createAaaaRecord(service, adapter));
                service.populateRecords(records);

                std::vector<char>sendBuffer(service.recordSize);
                send_mdns_query_answer(unicast, sock, from, addrlen, sendBuffer, query_id, rtype, name, answer, records, adapter);
            }
        } 
        else if (name == service.serviceInstance)
        {
            if ((rtype == MDNS_RECORDTYPE_SRV) || (rtype == MDNS_RECORDTYPE_ANY)) 
            {
                mdns_record_t answer = createSrvRecord(service);

                std::vector<mdns_record_t> records;
                records.reserve(service.updateConnectedClientsAndGetPropsCount() + 2);

                if (adapter.ipv4Sock >= 0 && from->sa_family == AF_INET)
                    records.push_back(createARecord(service, adapter));
                if (adapter.ipv6Sock >= 0)
                    records.push_back(createAaaaRecord(service, adapter));
                service.populateRecords(records);

                std::vector<char>sendBuffer(service.recordSize);
                send_mdns_query_answer(unicast, sock, from, addrlen, sendBuffer, query_id, rtype, name, answer, records, adapter);
            }
        } 
        else if (name == service.serviceQualified)
        {
            if ((rtype == MDNS_RECORDTYPE_A || rtype == MDNS_RECORDTYPE_ANY) && adapter.ipv4Sock >= 0 && from->sa_family == AF_INET)
            {
                mdns_record_t answer = createARecord(service, adapter);

                std::vector<mdns_record_t> records;
                records.reserve(service.updateConnectedClientsAndGetPropsCount() + 1);

                records.push_back(answer);
                service.populateRecords(records);

                std::vector<char>sendBuffer(service.recordSize);
                send_mdns_query_answer(unicast, sock, from, addrlen, sendBuffer, query_id, rtype, name, answer, records, adapter);
            } 
            else if ((rtype == MDNS_RECORDTYPE_AAAA || rtype == MDNS_RECORDTYPE_ANY) && adapter.ipv6Sock >= 0)
            {
                mdns_record_t answer = createAaaaRecord(service, adapter);

                std::vector<mdns_record_t> records;
                records.reserve(service.updateConnectedClientsAndGetPropsCount() + 1);

                records.push_back(answer);
                service.populateRecords(records);

                std::vector<char>sendBuffer(service.recordSize);
                send_mdns_query_answer(unicast, sock, from, addrlen, sendBuffer, query_id, rtype, name, answer, records, adapter);
            }
        }
    }
    return 0;
}

bool UniqueQuerier::operator==(const UniqueQuerier& other) const noexcept
{
    if (sock != other.sock)
        return false;

    if (addr.ss_family != other.addr.ss_family)
        return false;

    if (addr.ss_family == AF_INET)
    {
        auto* aIpV4 = reinterpret_cast<const sockaddr_in*>(&addr);
        auto* bIpV4 = reinterpret_cast<const sockaddr_in*>(&other.addr);

        return aIpV4->sin_port == bIpV4->sin_port &&
               aIpV4->sin_addr.s_addr == bIpV4->sin_addr.s_addr;
    }

    if (addr.ss_family == AF_INET6)
    {
        auto* aIpV6 = reinterpret_cast<const sockaddr_in6*>(&addr);
        auto* bIpV6 = reinterpret_cast<const sockaddr_in6*>(&other.addr);

        return aIpV6->sin6_port == bIpV6->sin6_port &&
               std::memcmp(&aIpV6->sin6_addr, &bIpV6->sin6_addr, sizeof(in6_addr)) == 0;
    }

    return false;
}

AtomicSteadyTimePoint::AtomicSteadyTimePoint(TimePointType tp)
    : value(toRep(tp))
{}

AtomicSteadyTimePoint::operator TimePointType() const
{
    return fromRep(value.load(std::memory_order_relaxed));
}

AtomicSteadyTimePoint& AtomicSteadyTimePoint::operator=(TimePointType tp)
{
    value.store(toRep(tp), std::memory_order_relaxed);
    return *this;
}

AtomicSteadyTimePoint::TimePointType AtomicSteadyTimePoint::fromRep(Rep v)
{
    return TimePointType(Clock::duration(v));
}

AtomicSteadyTimePoint::Rep AtomicSteadyTimePoint::toRep(AtomicSteadyTimePoint::TimePointType tp)
{
    return tp.time_since_epoch().count();
}

void AtomicSteadyTimePoint::store(AtomicSteadyTimePoint::TimePointType tp)
{
    value.store(toRep(tp), std::memory_order_relaxed);
}

AtomicSteadyTimePoint::TimePointType AtomicSteadyTimePoint::load() const
{
    return fromRep(value.load(std::memory_order_relaxed));
}

QuerierBucket::QuerierBucket(const UniqueQuerier& querier, uint32_t hashFingerprint, size_t maxBurst, size_t refillRate, AtomicSteadyTimePoint::TimePointType lastQueried)
    : primaryQuerier(querier)
    , primaryQuerierHash(hashFingerprint)
    , tokens(maxBurst)
    , maxBurst(maxBurst)
    , refillRate(refillRate)
    , lastQueried(lastQueried)
{
}

END_NAMESPACE_DISCOVERY_SERVICE
