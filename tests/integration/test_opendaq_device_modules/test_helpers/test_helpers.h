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

#include <chrono>
#include <thread>
#include <future>
#include <fstream>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_service.hpp>

#include <opendaq/opendaq.h>
#include <testutils/testutils.h>
#include "testutils/memcheck_listener.h"

#include <opendaq/event_packet_params.h>
#include <opendaq/custom_log.h>
#include <opendaq/client_type.h>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif

// MAC CI issue
#if !defined(SKIP_TEST_MAC_CI)
#   if defined(__clang__)
#       define SKIP_TEST_MAC_CI GTEST_SKIP() << "Skipping test on MacOs"
#   else
#       define SKIP_TEST_MAC_CI
#   endif
#endif

// In Modern LT module operations are processed asynchronously.
// We have to give the client time to complete asynchronous processing related to signal creation.
// Otherwise getSignal() on the client may not find it yet.
#if defined(DAQMODULES_LT_LEGACY_MODULES)
#   define CONDITIONAL_SLEEP
#elif defined(__APPLE__)
#   define CONDITIONAL_SLEEP std::this_thread::sleep_for(std::chrono::seconds(5))
#else
#   define CONDITIONAL_SLEEP std::this_thread::sleep_for(std::chrono::seconds(1))
#endif

BEGIN_NAMESPACE_OPENDAQ

namespace test_helpers
{
    [[maybe_unused]]
    inline void setupSubscribeAckHandler(
        std::promise<StringPtr>& acknowledgementPromise,
        std::future<StringPtr>& acknowledgementFuture,
        MirroredSignalConfigPtr& signal
    )
    {
        acknowledgementFuture = acknowledgementPromise.get_future();
        signal.getOnSubscribeComplete() +=
            [&acknowledgementPromise]
            (MirroredSignalConfigPtr&, SubscriptionEventArgsPtr& args)
            {
                try
                {
                    acknowledgementPromise.set_value(args.getStreamingConnectionString());
                }
                catch (const std::future_errc&)
                {
                    ADD_FAILURE()  << " Set already satisfied unsubscribe ack promise for streaming: "
                                   << args.getStreamingConnectionString();
                }
            };
    }

    [[maybe_unused]]
    inline void setupUnsubscribeAckHandler(
        std::promise<StringPtr>& acknowledgementPromise,
        std::future<StringPtr>& acknowledgementFuture,
        MirroredSignalConfigPtr& signal
    )
    {
        acknowledgementFuture = acknowledgementPromise.get_future();
        signal.getOnUnsubscribeComplete() +=
            [&acknowledgementPromise]
            (MirroredSignalConfigPtr&, SubscriptionEventArgsPtr& args)
            {
                try
                {
                    acknowledgementPromise.set_value(args.getStreamingConnectionString());
                }
                catch (const std::future_errc&)
                {
                    ADD_FAILURE()  << " Set already satisfied unsubscribe ack promise for streaming: "
                                   << args.getStreamingConnectionString();
                }
            };
    }

    [[maybe_unused]]
    inline bool waitForAcknowledgement(
        std::future<StringPtr>& acknowledgementFuture,
        std::chrono::seconds timeout = std::chrono::seconds(5))
    {
        return acknowledgementFuture.wait_for(timeout) == std::future_status::ready;
    }

    // Successfully resolving the localhost address confirms that an IPv6 connection is possible with the localhost address.
    // However, this does not guarantee connectivity via an external IPv6 addresses (e.g., when connecting to a discovered
    // address of a local simulator).
    // To verify external IPv6 connectivity as well, resolving a real external address like "ipv6.google.com" is a reliable workaround.
    [[maybe_unused]]
    inline bool Ipv6IsDisabled(bool testExternalAddrConnectivity = false)
    {
        std::string hostName = testExternalAddrConnectivity ? "ipv6.google.com" : "localhost";

        boost::asio::io_service service;
        boost::asio::ip::tcp::resolver resolver(service);

        // Resolve a localhost (or external) address. If IPv6 is available, it should resolve to an IPv6 address.
        boost::asio::ip::tcp::resolver::query query(boost::asio::ip::tcp::v6(), hostName, "");

        boost::system::error_code ec;
        auto it = resolver.resolve(query, ec);
        return ec.failed();
    }

    [[maybe_unused]]
    inline bool isIpv6ConnectionString(const StringPtr& connectionString)
    {
        if (connectionString.assigned() && connectionString.getLength())
            return connectionString.toStdString().find('[') != std::string::npos &&
                   connectionString.toStdString().find(']') != std::string::npos;
        else
            return false;
    }

    [[maybe_unused]]
    inline bool isIpv4ConnectionString(const StringPtr& connectionString)
    {
        if (connectionString.assigned() && connectionString.getLength())
            return !isIpv6ConnectionString(connectionString);
        else
            return false;
    }

    [[maybe_unused]]
    inline bool isSufix(const std::string & str, const std::string & suffix)
    {
        return str.size() >= suffix.size() && 
                str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
    }

    [[maybe_unused]]
    inline Finally CreateConfigFile(const std::string& configFilename, const std::string& data)
    {
        std::ofstream file;
        file.open(configFilename);
        if (!file.is_open())
            throw std::runtime_error("can not open file for writing");

        file << data;
        file.close();
        return Finally([&configFilename] { remove(configFilename.c_str()); });
    }

    [[maybe_unused]]
    inline ListPtr<IPacket> tryReadPackets(const PacketReaderPtr& reader,
                                           size_t packetCount,
                                           std::chrono::seconds timeout = std::chrono::seconds(60))
    {
        auto allPackets = List<IPacket>();
        auto startPoint = std::chrono::system_clock::now();

        while (allPackets.getCount() < packetCount)
        {
            if (reader.getAvailableCount() == 0)
            {
                auto now = std::chrono::system_clock::now();
                auto timeElapsed = now - startPoint;
                if (timeElapsed > timeout)
                {
                    break;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                continue;
            }

            auto packets = reader.readAll();

            for (const auto& packet : packets)
                allPackets.pushBack(packet);
        }

        return allPackets;
    }

    [[maybe_unused]]
    inline bool packetsEqual(const ListPtr<IPacket>& listA, const ListPtr<IPacket>& listB, bool skipEventPackets = false)
    {
        auto context = NullContext();
        auto loggerComponent = context.getLogger().getOrAddComponent("packetsEqual");

        bool result = true;
        if (listA.getCount() != listB.getCount())
        {
            LOG_E("Compared packets count differs: A {}, B {}", listA.getCount(), listB.getCount());
            result = false;
        }

        auto count = std::min(listA.getCount(), listB.getCount());

        for (SizeT i = 0; i < count; i++)
        {
            if (!BaseObjectPtr::Equals(listA.getItemAt(i), listB.getItemAt(i)))
            {
                if (listA.getItemAt(i).getType() == PacketType::Event &&
                    listB.getItemAt(i).getType() == PacketType::Event)
                {
                    if (skipEventPackets)
                        continue;
                    auto eventPacketA = listA.getItemAt(i).asPtr<IEventPacket>(true);
                    auto eventPacketB = listB.getItemAt(i).asPtr<IEventPacket>(true);

                    if (eventPacketA.getEventId() != eventPacketB.getEventId())
                    {
                        LOG_E("Event id of packets at index {} differs: A - \"{}\", B - \"{}\"",
                              i, eventPacketA.getEventId(), eventPacketB.getEventId());
                    }
                    else if(eventPacketA.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED &&
                             eventPacketB.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
                    {
                        const DataDescriptorPtr valueDataDescA = eventPacketA.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
                        const DataDescriptorPtr domainDataDescA = eventPacketA.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);

                        const DataDescriptorPtr valueDataDescB = eventPacketB.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
                        const DataDescriptorPtr domainDataDescB = eventPacketB.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);

                        LOG_E("Event parameters of packets at index {} differs:", i);
                        LOG_E("packet A - \nvalue:\n\"{}\"\ndomain:\n\"{}\"",
                              valueDataDescA.assigned() ? valueDataDescA.toString() : "null",
                              domainDataDescA.assigned() ? domainDataDescA.toString() : "null");
                        LOG_E("packet B - \nvalue:\n\"{}\"\ndomain:\n\"{}\"",
                              valueDataDescB.assigned() ? valueDataDescB.toString() : "null",
                              domainDataDescB.assigned() ? domainDataDescB.toString() : "null");
                    }
                    else
                    {
                        LOG_E("Event packets at index {} differs: A - \"{}\", B - \"{}\"",
                              i, listA.getItemAt(i).toString(), listB.getItemAt(i).toString());
                    }
                }
                else
                {
                    LOG_E("Data packets at index {} differs: A - \"{}\", B - \"{}\"",
                          i, listA.getItemAt(i).toString(), listB.getItemAt(i).toString());
                }
                result = false;
            }
        }

        return result;
    }

    [[maybe_unused]]
    inline bool packetsEqualWithComparators(const ListPtr<IPacket>& listA,
                                            const ListPtr<IPacket>& listB,
                                            std::function<bool(const PacketPtr&, const PacketPtr&)> dataPacketComparator,
                                            std::function<bool(const PacketPtr&, const PacketPtr&)> eventPacketComparator)
    {
        auto context = NullContext();
        auto loggerComponent = context.getLogger().getOrAddComponent("packetsEqual");

        bool result = true;
        if (listA.getCount() != listB.getCount())
        {
            LOG_E("Compared packets count differs: A {}, B {}", listA.getCount(), listB.getCount());
            result = false;
        }

        auto count = std::min(listA.getCount(), listB.getCount());

        for (SizeT i = 0; i < count; i++)
        {
            if (listA.getItemAt(i).getType() == PacketType::Event &&
                listB.getItemAt(i).getType() == PacketType::Event)
            {
                bool eventResult = eventPacketComparator(listA.getItemAt(i), listB.getItemAt(i));
                result &= eventResult;

                if (!eventResult)
                {
                    LOG_E("Event packets at index {} differs: A - \"{}\", B - \"{}\"",
                          i,
                          listA.getItemAt(i).toString(),
                          listB.getItemAt(i).toString());
                }
            }
            else if (listA.getItemAt(i).getType() == PacketType::Data &&
                listB.getItemAt(i).getType() == PacketType::Data)
            {
                bool dataResult = dataPacketComparator(listA.getItemAt(i), listB.getItemAt(i));
                result &= dataResult;

                if (!dataResult)
                {
                    LOG_E("Data packets at index {} differs: A - \"{}\", B - \"{}\"",
                          i, listA.getItemAt(i).toString(), listB.getItemAt(i).toString());
                }
            }
            else if (!BaseObjectPtr::Equals(listA.getItemAt(i), listB.getItemAt(i)))
            {
                result = false;
                LOG_E("Packets at index {} differs: A - \"{}\", B - \"{}\"",
                      i, listA.getItemAt(i).toString(), listB.getItemAt(i).toString());
            }
        }

        return result;
    }

    [[maybe_unused]]
    inline bool packetBehaviorComparison(const ListPtr<IPacket>& listA,
                                            const ListPtr<IPacket>& listB,
                                            std::function<bool(const PacketPtr&, const PacketPtr&)> dataPacketComparator,
                                            std::function<bool(const DataDescriptorPtr&, const DataDescriptorPtr&)> decriptorComparator)
    {
        // This overload is used for more complex comparison of packets in which data packets are
        // compared with one comparator, and event packets are not compared directly,
        // but instead used to extract descriptors which are compared with another comparator
        // The idea is to compare descriptors that were set before data packets
        auto context = NullContext();
        auto loggerComponent = context.getLogger().getOrAddComponent("packetsEqual");


        if (!listA.assigned() && !listB.assigned())
            return true;
        if (listA.assigned() && listB.assigned() && listA.getCount() == 0 && listB.getCount() == 0)
            return true;

        bool result = true;

        struct Entity {
            Entity(std::string name, const ListPtr<IPacket>& list)
                : name(std::move(name))
                , list(list)
                , lastDataDesc(nullptr)
                , lastDomainDesc(nullptr)
                , currentPacket(nullptr)
                , index(0)
                , count(list.getCount()) {};
            void extractPacket()
            {
                if (count > index)
                    currentPacket = list.getItemAt(index);
                else
                    currentPacket = nullptr;
            }

            const std::string name;
            const ListPtr<IPacket>& list;
            DataDescriptorPtr lastDataDesc;
            DataDescriptorPtr lastDomainDesc;
            PacketPtr currentPacket;
            SizeT index = 0;
            SizeT count = 0;

        };
        Entity entityA("A", listA);
        Entity entityB("B", listB);

        bool comparing = true;
        while (result && comparing)
        {
            entityA.extractPacket();
            entityB.extractPacket();

            {
                // event section
                // Here we set data and domain descriptors from event packets to later compare them with each other
                auto handleEvent = [&](Entity& entity)
                {
                    if (entity.currentPacket.assigned())
                    {
                        if (entity.currentPacket.getType() == PacketType::Event)
                        {
                            // extract descriptors
                            auto eventPacket = entity.currentPacket.asPtr<IEventPacket>(true);

                            if (eventPacket.getEventId() != event_packet_id::DATA_DESCRIPTOR_CHANGED)
                            {
                                // unsupported event id
                                result = false;
                                LOG_E("Event packets at index {} in {} has wrong event id", entity.index, entity.name);
                            }
                            else
                            {
                                if (auto valueDataDesc = eventPacket.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
                                    valueDataDesc.assigned())
                                    entity.lastDataDesc = std::move(valueDataDesc);
                                if (auto domainDataDesc = eventPacket.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);
                                    domainDataDesc.assigned())
                                    entity.lastDomainDesc = std::move(domainDataDesc);
                            }
                        }
                        else if (entity.currentPacket.getType() == PacketType::Data)
                        {
                           // skip
                        }
                        else
                        {
                            // wrong packet type
                            result = false;
                            LOG_E("Event packets at index {} in {} has wrong event type", entity.index, entity.name);
                        }
                    }
                };

                handleEvent(entityA);
                handleEvent(entityB);
                if (!result)
                    break;
            }

            if (entityA.currentPacket.assigned() && entityB.currentPacket.assigned())
            {
                if (entityA.currentPacket.getType() == PacketType::Data && entityB.currentPacket.getType() == PacketType::Data)
                {
                    // if both packets are data packets we compare descriptors that were set before them and then compare the data packets themselves
                    // compare last descriptors
                    bool dataDescResult = decriptorComparator(entityA.lastDataDesc, entityB.lastDataDesc);
                    if (!dataDescResult)
                    {
                        LOG_E("Data descriptors are not synchronized: A - \"{}\", B - \"{}\"",
                              entityA.lastDataDesc.assigned() ? entityA.lastDataDesc.toString() : "null",
                              entityB.lastDataDesc.assigned() ? entityB.lastDataDesc.toString() : "null");
                    }

                    bool domainDescResult = decriptorComparator(entityA.lastDomainDesc, entityB.lastDomainDesc);
                    if (!domainDescResult)
                    {
                        LOG_E("Domain data descriptors are not synchronized: A - \"{}\", B - \"{}\"",
                              entityA.lastDomainDesc.assigned() ? entityA.lastDomainDesc.toString() : "null",
                              entityB.lastDomainDesc.assigned() ? entityB.lastDomainDesc.toString() : "null");
                    }
                    result &= dataDescResult && domainDescResult;

                    // compare data packets
                    bool dataResult = dataPacketComparator(entityA.currentPacket, entityB.currentPacket);
                    if (!dataResult)
                    {
                        LOG_E("Data packets at index A - \"{}\", B - \"{}\"",
                              entityA.currentPacket.toString(), entityB.currentPacket.toString());
                    }
                    result &= dataResult;
                }
                // if at least one packet is not a data packet we just skip it and move to the next one
                // because we have extracted descriptors on the previous step and we will compare them with the next data packets
                if (entityA.currentPacket.getType() != PacketType::Data)
                    ++entityA.index;
                if (entityB.currentPacket.getType() != PacketType::Data)
                    ++entityB.index;
                // if both packets are data packets we move to the next ones for future comparison
                if (entityA.currentPacket.getType() == PacketType::Data && entityB.currentPacket.getType() == PacketType::Data)
                {
                    ++entityA.index;
                    ++entityB.index;
                }
            }
            else if (!entityA.currentPacket.assigned() && !entityB.currentPacket.assigned())
            {
                // if both packets are not assigned we have reached the end of both lists and we need to compare last descriptors that were set before the last data packets
                // the idea is that we must have the same descriptors
                comparing = false;

                bool dataDescResult = decriptorComparator(entityA.lastDataDesc, entityB.lastDataDesc);
                if (!dataDescResult)
                {
                    LOG_E("Data descriptors are not synchronized: A - \"{}\", B - \"{}\"",
                          entityA.lastDataDesc.assigned() ? entityA.lastDataDesc.toString() : "null",
                          entityB.lastDataDesc.assigned() ? entityB.lastDataDesc.toString() : "null");
                }

                bool domainDescResult = decriptorComparator(entityA.lastDomainDesc, entityB.lastDomainDesc);
                if (!domainDescResult)
                {
                    LOG_E("Domain data descriptors are not synchronized: A - \"{}\", B - \"{}\"",
                          entityA.lastDomainDesc.assigned() ? entityA.lastDomainDesc.toString() : "null",
                          entityB.lastDomainDesc.assigned() ? entityB.lastDomainDesc.toString() : "null");
                }
                result &= dataDescResult && domainDescResult;
            }
            else
            {
                // at least one packet not assigned
                // if we have a data packet only for one entity then it is an error state (we have more data packets in one list than in the other)
                // if we have an event packet only for one entity then we just skip it and move to the next one for future comparison

                auto process = [&](Entity& entity)
                {
                    if (entity.currentPacket.assigned())
                    {
                        if (entity.currentPacket.getType() == PacketType::Event)
                        {
                            ++entity.index;
                        }
                        else
                        {
                            comparing = false;
                            result = false;
                            LOG_E("Container {} has additional data packet at index {}:  \"{}\"",
                                  entity.name,
                                  entity.index,
                                  entity.currentPacket.toString());
                        }
                    }
                };
                process(entityA);
                process(entityB);
            }
        }

        return result;
    }

    [[maybe_unused]]
    inline std::string valueToString(void* data, const SampleType st)
    {
        std::string result;
        switch (st)
        {
            case SampleType::Int8:
                result = std::to_string(*static_cast<SampleTypeToType<SampleType::Int8>::Type*>(data));
                break;
            case SampleType::UInt8:
                result = std::to_string(*static_cast<SampleTypeToType<SampleType::UInt8>::Type*>(data));
                break;
            case SampleType::Int16:
                result = std::to_string(*static_cast<SampleTypeToType<SampleType::Int16>::Type*>(data));
                break;
            case SampleType::UInt16:
                result = std::to_string(*static_cast<SampleTypeToType<SampleType::UInt16>::Type*>(data));
                break;
            case SampleType::Int32:
                result = std::to_string(*static_cast<SampleTypeToType<SampleType::Int32>::Type*>(data));
                break;
            case SampleType::UInt32:
                result = std::to_string(*static_cast<SampleTypeToType<SampleType::UInt32>::Type*>(data));
                break;
            case SampleType::Int64:
                result = std::to_string(*static_cast<SampleTypeToType<SampleType::Int64>::Type*>(data));
                break;
            case SampleType::UInt64:
                result = std::to_string(*static_cast<SampleTypeToType<SampleType::UInt64>::Type*>(data));
                break;
            case SampleType::Float32:
                result = std::to_string(*static_cast<SampleTypeToType<SampleType::Float32>::Type*>(data));
                break;
            case SampleType::Float64:
                result = std::to_string(*static_cast<SampleTypeToType<SampleType::Float64>::Type*>(data));
                break;
            default:
                result = "";
        }
        return result;
    }

    [[maybe_unused]]
    inline void printPackets(const std::string& label, const ListPtr<IPacket>& packets, const bool skipValues = false)
    {
        std::cout << "=== " << label << " (" << packets.getCount() << " packets) ===\n";
        for (SizeT i = 0; i < packets.getCount(); ++i)
        {
            const auto packet = packets.getItemAt(i);
            std::cout << "[" << i << "] ";

            if (packet.getType() == PacketType::Event)
            {
                auto ev = packet.asPtr<IEventPacket>(true);
                std::cout << "Event id=" << ev.getEventId();
                if (ev.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
                {
                    const DataDescriptorPtr valueDesc = ev.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
                    const DataDescriptorPtr domainDesc = ev.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);
                    std::cout << "\n    value : " << (valueDesc.assigned() ? valueDesc.toString() : "null")
                              << "\n    domain: " << (domainDesc.assigned() ? domainDesc.toString() : "null");
                }
                std::cout << "\n";
            }
            else if (packet.getType() == PacketType::Data)
            {
                auto printValues = [skipValues](const DataPacketPtr& packet)
                {
                    const auto dataDesc = packet.getDataDescriptor();
                    const auto sampleCount = packet.getSampleCount();
                    std::cout << "[";
                    if (dataDesc.assigned() && !skipValues)
                    {
                        std::byte* values = static_cast<std::byte*>(packet.getData());
                        for (SizeT s = 0; s < sampleCount; ++s)
                            std::cout << (s ? ", " : "") << valueToString(values + (s * dataDesc.getRawSampleSize()), dataDesc.getSampleType());
                    }
                    else
                    {
                        std::cout << "<...>";
                    }
                    std::cout << "]";
                };

                auto dataPacket = packet.asPtr<IDataPacket>(true);

                std::cout << "Data sampleCount=" << dataPacket.getSampleCount() << " values=";

                printValues(dataPacket);

                if (auto domainPacket = dataPacket.getDomainPacket().asPtr<IDataPacket>(false); domainPacket.assigned())
                {
                    std::cout << "; domain=";
                    printValues(domainPacket);
                }
                std::cout << "\n";
            }
            else
            {
                std::cout << packet.toString() << "\n";
            }
        }
        std::cout << std::flush;
    }

    [[maybe_unused]]
    inline InstancePtr connectInstanceWithClientType(const InstancePtr& clientInstance, const std::string& connectionString, ClientType clientType, bool dropOthers = false)
    {
        auto config = clientInstance.createDefaultAddDeviceConfig();
        PropertyObjectPtr generalConfig = config.getPropertyValue("General");

        generalConfig.setPropertyValue("ClientType", (Int) clientType);
        generalConfig.setPropertyValue("ExclusiveControlDropOthers", dropOthers);

        auto device = clientInstance.addDevice(connectionString, config);
        return clientInstance;
    }

    [[maybe_unused]]
    inline InstancePtr connectInstanceWithClientType(const std::string& connectionString, ClientType clientType, bool dropOthers = false)
    {
        return connectInstanceWithClientType(Instance(), connectionString, clientType, dropOthers);
    }


    [[maybe_unused]]
    inline void checkDeviceOperationMode(const daq::DevicePtr& device, OperationModeType expected, bool isServer = false, bool skipChannelAndSignalCheck = false)
    {
        ASSERT_EQ(device.getOperationMode(), expected) << "Device: " << device.getGlobalId();
        bool active = expected != OperationModeType::Idle;
        std::string messagePrefix = isServer ? "Server: " : "Client: ";
        if (skipChannelAndSignalCheck)
            return;
        for (const auto& ch: device.getChannels())
        {
            ASSERT_EQ(ch.getActive(), active) << messagePrefix << "Checking ch " << ch.getGlobalId() << " for mode " << static_cast<int>(expected);
            for (const auto& sig: ch.getSignals())
                ASSERT_EQ(sig.getActive(), active) << messagePrefix << "Checking ch signal " << sig.getGlobalId() << " for mode " << static_cast<int>(expected);
        }
    }

    [[maybe_unused]]
    inline void testPropObjsEquality(const PropertyObjectPtr& configA, const PropertyObjectPtr& configB, std::string path = "")
    {
        auto allPropsA = configA.getAllProperties();
        auto allPropsB = configB.getAllProperties();
        ASSERT_EQ(allPropsA.getCount(), allPropsB.getCount())
            << "\"" << path << "\" count of properties differs: A - " << allPropsA.getCount() << "; B - " << allPropsB.getCount();
        for (const auto& propA : allPropsA)
        {
            StringPtr propName = propA.getName();
            ASSERT_TRUE(configB.hasProperty(propName))
                << "\"" << path << "\" A property \"" << propName << "\" missing in B";
            ASSERT_EQ(configB.getProperty(propName).getValueType(), propA.getValueType())
                << "\"" << path << "\" property \"" << propName << "\" type in A - " << propA.getValueType()
                << " doesn't equal to type in B - " << configB.getProperty(propName).getValueType();

            auto propValueA = propA.getValue();
            auto propValueB = configB.getPropertyValue(propName);
            if (propValueA.supportsInterface<IPropertyObject>())
            {
                testPropObjsEquality(propValueA, propValueB, path + (path.empty() ? "" : ".") + propName.toStdString());
            }
            else
            {
                ASSERT_TRUE(BaseObjectPtr::Equals(propValueA, propValueB))
                    << "\"" << path << "\" property \"" << propName << "\" value in A \"" << propValueA
                    << "\" doesn't equal to value in B \"" << propValueB << "\"";
            }
        }
    }
}

inline void removeDeviceDomainSignal(ListPtr<ISignal>& list)
{
    for (size_t i = 0; i < list.getCount(); ++i)
    {
        if (list[i].getDescriptor().getName() == "Time")
        {
            list.deleteAt(i);
            break;
        }
    }
}

END_NAMESPACE_OPENDAQ
