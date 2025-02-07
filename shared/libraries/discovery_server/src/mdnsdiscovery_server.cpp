
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
#ifdef _WIN32
    #define _CRT_SECURE_NO_WARNINGS 1
#endif

BEGIN_NAMESPACE_DISCOVERY_SERVICE

MdnsDiscoveredService::MdnsDiscoveredService(const std::string& serviceName,
                                             uint32_t servicePort,
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
                this->properties[key] = value;
                staticRecordSize += key.size() + value.size() + 2;
            }
            else
            {
                dynamicProperties.push_back({key, ""});
            }
        }
    }
    this->recordSize = staticRecordSize;
}

size_t MdnsDiscoveredService::size() const
{
    return properties.size() + dynamicProperties.size();
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
        records.push_back(createTxtRecord(key, value));
        this->recordSize += key.size() + value.size() + 2;
    }
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
    
MDNSDiscoveryServer::MDNSDiscoveryServer(void)
{
#ifdef _WIN32
    WORD versionWanted = MAKEWORD(1, 1);
    WSADATA wsaData;
    if (WSAStartup(versionWanted, &wsaData))
        throw std::runtime_error("MDNSDiscoveryServer: Failed to initialize WinSock");

#endif
    hostName = getHostname();
    openServerSockets(sockets);
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

mdns_record_t MDNSDiscoveryServer::createARecord(const MdnsDiscoveredService& service) const
{
    mdns_record_t recordA;
    recordA.name = {service.serviceQualified.c_str(), service.serviceQualified.size()},
    recordA.type = MDNS_RECORDTYPE_A,
    recordA.data.a.addr = serviceAddressIpv4,
    recordA.rclass = 0,
    recordA.ttl = 0;
    return recordA;
}

mdns_record_t MDNSDiscoveryServer::createAaaaRecord(const MdnsDiscoveredService& service) const
{
    mdns_record_t recordAAA;
    recordAAA.name = {service.serviceQualified.c_str(), service.serviceQualified.size()},
    recordAAA.type = MDNS_RECORDTYPE_AAAA,
    recordAAA.data.aaaa.addr = serviceAddressIpv6,
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

bool MDNSDiscoveryServer::registerService(const std::string& id, MdnsDiscoveredService& service)
{
    std::string serviceName = hostName;

    auto manufacturer = service.properties["manufacturer"];
    auto serialNumber = service.properties["serialNumber"];
    if (!manufacturer.empty() && !serialNumber.empty())
        serviceName = manufacturer + "_" + serialNumber;
    else
        fprintf(stderr, "MDNSDiscoveryServer: Manufacturer and serial number not provided for service %s. It can cause mdns conflict\n", id.c_str());

    service.serviceInstance = serviceName + "." + service.serviceName;
    service.serviceQualified = hostName + ".local.";

    bool success = false;
    {
        std::lock_guard<std::mutex> lock(mx);
        success = services.emplace(id, service).second;
    }
    if (success)
    {
        std::vector<mdns_record_t> records;
        records.reserve(service.size() + 3);
        records.push_back(createSrvRecord(service));
        if (serviceAddressIpv4.sin_family == AF_INET)
            records.push_back(createARecord(service));
        if (serviceAddressIpv6.sin6_family == AF_INET6)
            records.push_back(createAaaaRecord(service));
        service.populateRecords(records);

        std::vector<char> buffer(service.recordSize);
        for (const auto & socket : sockets)
            mdns_announce_multicast(socket, buffer.data(), buffer.size(), createPtrRecord(service), 0, 0, records.data(), records.size());

        start();
    }

    return success;
}

void MDNSDiscoveryServer::goodbyeMulticast(const MdnsDiscoveredService& service)
{
    std::vector<mdns_record_t> records;
    records.reserve(service.size() + 3);
    records.push_back(createSrvRecord(service));
    if (serviceAddressIpv4.sin_family == AF_INET)
        records.push_back(createARecord(service));
    if (serviceAddressIpv6.sin6_family == AF_INET6)
        records.push_back(createAaaaRecord(service));
    service.populateRecords(records);

    std::vector<char> buffer(service.recordSize);
    for (const auto & socket : sockets)
        mdns_goodbye_multicast(socket, buffer.data(), buffer.size(), createPtrRecord(service), 0, 0, records.data(), records.size());
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
                    mdns_socket_listen(socket, buffer.data(), buffer.size(), callbackWrapper, &callback);
                FD_SET((u_int) socket, &readfs);
            }
        } 
        else 
        {
            break;
        }
    }
}

void MDNSDiscoveryServer::openServerSockets(std::vector<int>& sockets) 
{
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

    if (sockets.empty())
    {
#ifdef _WIN32
        WSACleanup();
#endif
        throw std::runtime_error("MDNSDiscoveryServer: Failed to open sockets");
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

                if ((saddr->sin_addr.S_un.S_un_b.s_b1 != 127) ||
                    (saddr->sin_addr.S_un.S_un_b.s_b2 != 0) || 
                    (saddr->sin_addr.S_un.S_un_b.s_b3 != 0) ||
                    (saddr->sin_addr.S_un.S_un_b.s_b4 != 1))
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

    if (!hasIpv4 && !hasIpv6)
    {
#ifdef _WIN32
        WSACleanup();
#endif
        throw std::runtime_error("MDNSDiscoveryServer: Network interfaces not found");
    }
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
    non_mdns_query_send(sock, sendBuffer.data(), sendBuffer.size(), query_id, opcode, 0x1, 0, 0, txtRecords.data(), txtRecords.size(), 0, 0, 0, 0, unicast ? 0x1 : 0x0, to, addrlen);
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
    if (entry != MDNS_ENTRYTYPE_QUESTION)
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
                send_mdns_query_answer(unicast, sock, from, addrlen, sendBuffer, query_id, rtype, name, answer, {});
            }
        } 
        else if (name == serviceName) 
        {
            if ((rtype == MDNS_RECORDTYPE_PTR) || (rtype == MDNS_RECORDTYPE_ANY)) 
            {
                mdns_record_t answer = createPtrRecord(service);

                std::vector<mdns_record_t> records;
                records.reserve(service.size() + 3);

                records.push_back(createSrvRecord(service));
                if (serviceAddressIpv4.sin_family == AF_INET && from->sa_family == AF_INET)
                    records.push_back(createARecord(service));
                if (serviceAddressIpv6.sin6_family == AF_INET6)
                    records.push_back(createAaaaRecord(service));
                service.populateRecords(records);

                std::vector<char>sendBuffer(service.recordSize);
                send_mdns_query_answer(unicast, sock, from, addrlen, sendBuffer, query_id, rtype, name, answer, records);
            }
        } 
        else if (name == service.serviceInstance)
        {
            if ((rtype == MDNS_RECORDTYPE_SRV) || (rtype == MDNS_RECORDTYPE_ANY)) 
            {
                mdns_record_t answer = createSrvRecord(service);

                std::vector<mdns_record_t> records;
                records.reserve(service.size() + 2);

                if (serviceAddressIpv4.sin_family == AF_INET && from->sa_family == AF_INET)
                    records.push_back(createARecord(service));
                if (serviceAddressIpv6.sin6_family == AF_INET6)
                    records.push_back(createAaaaRecord(service));
                service.populateRecords(records);

                std::vector<char>sendBuffer(service.recordSize);
                send_mdns_query_answer(unicast, sock, from, addrlen, sendBuffer, query_id, rtype, name, answer, records);
            }
        } 
        else if (name == service.serviceQualified)
        {
            if (((rtype == MDNS_RECORDTYPE_A) || (rtype == MDNS_RECORDTYPE_ANY)) && (serviceAddressIpv4.sin_family == AF_INET) && from->sa_family == AF_INET)
            {
                mdns_record_t answer = createARecord(service);

                std::vector<mdns_record_t> records;
                records.reserve(service.size() + 1);

                records.push_back(answer);
                service.populateRecords(records);

                std::vector<char>sendBuffer(service.recordSize);
                send_mdns_query_answer(unicast, sock, from, addrlen, sendBuffer, query_id, rtype, name, answer, records);
            } 
            else if (((rtype == MDNS_RECORDTYPE_AAAA) || (rtype == MDNS_RECORDTYPE_ANY)) && (serviceAddressIpv6.sin6_family == AF_INET6))
            {
                mdns_record_t answer = createAaaaRecord(service);

                std::vector<mdns_record_t> records;
                records.reserve(service.size() + 1);

                records.push_back(answer);
                service.populateRecords(records);

                std::vector<char>sendBuffer(service.recordSize);
                send_mdns_query_answer(unicast, sock, from, addrlen, sendBuffer, query_id, rtype, name, answer, records);
            }
        }
    }
    return 0;
}

END_NAMESPACE_DISCOVERY_SERVICE
