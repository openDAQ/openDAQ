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

BEGIN_NAMESPACE_OPENDAQ

namespace test_helpers
{
    [[maybe_unused]]
    static void setupSubscribeAckHandler(
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
    static void setupUnsubscribeAckHandler(
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
    static bool waitForAcknowledgement(
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
    static bool Ipv6IsDisabled(bool testExternalAddrConnectivity = false)
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
    static bool isIpv6ConnectionString(const StringPtr& connectionString)
    {
        if (connectionString.assigned() && connectionString.getLength())
            return connectionString.toStdString().find('[') != std::string::npos &&
                   connectionString.toStdString().find(']') != std::string::npos;
        else
            return false;
    }

    [[maybe_unused]]
    static bool isIpv4ConnectionString(const StringPtr& connectionString)
    {
        if (connectionString.assigned() && connectionString.getLength())
            return !isIpv6ConnectionString(connectionString);
        else
            return false;
    }

    [[maybe_unused]]
    static bool isSufix(const std::string & str, const std::string & suffix)
    {
        return str.size() >= suffix.size() && 
                str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
    }

    [[maybe_unused]]
    static Finally CreateConfigFile(const std::string& configFilename, const std::string& data)
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
    static ListPtr<IPacket> tryReadPackets(const PacketReaderPtr& reader,
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
    static DataDescriptorPtr getLTAlignedDescriptor(const DataDescriptorPtr& descA,
                                                    const DataDescriptorPtr& descB)
    {
        // no post scaling and desc metadata transferred within modern LT modules
        auto alignedDescB =
            DataDescriptorBuilderCopy(descB)
                .setMetadata(descA.getMetadata())
                .setPostScaling(descA.getPostScaling())
                .build();

        return alignedDescB;
    }

    [[maybe_unused]]
    static bool equalPackets(const DataPacketPtr& packetA,
                             const DataPacketPtr& packetB,
                             const LoggerComponentPtr& loggerComponent)
    {
        bool result = true;

        auto dumpPacketData = [](const DataPacketPtr& packet)
        {
            const void* address = packet.getData();
            auto count = packet.getDataSize();
            auto* bytes = static_cast<const uint8_t*>(address);

            fmt::memory_buffer buf;
            for (std::size_t i = 0; i < count; ++i) {
                fmt::format_to(std::back_inserter(buf), "{:02X} ", bytes[i]);
            }

            return fmt::to_string(buf);
        };

        auto dumpA = dumpPacketData(packetA);
        auto dumpB = dumpPacketData(packetB);
        if (dumpA != dumpB)
        {
            LOG_E("Data in packets differs: A: \n{}\n, B:\n{}\n", dumpA, dumpB);
            result = false;
        }
        else if (packetA.getDomainPacket().assigned() && packetB.getDomainPacket().assigned() &&
                 packetA.getDomainPacket().getOffset() != packetB.getDomainPacket().getOffset())
        {
            LOG_E("Domain packets in packets differs: A: \n{}\n, B:\n{}\n",
                  packetA.getDomainPacket().getOffset().toString(),
                  packetB.getDomainPacket().getOffset().toString());
            result = false;
        }

        return result;
    }

    [[maybe_unused]]
    static bool equalDescriptors(const DataDescriptorPtr& descA,
                                 const DataDescriptorPtr& descB,
                                 const LoggerComponentPtr& loggerComponent,
                                 bool skipLTMissingDescFields = false)
    {
        if (!BaseObjectPtr::Equals(descA, descB))
        {
            if (descA.assigned() && descB.assigned())
            {
                if (skipLTMissingDescFields)
                {
                    auto alignedDescB = getLTAlignedDescriptor(descA, descB);
                    if (BaseObjectPtr::Equals(descA, alignedDescB))
                        return true;
                }
                auto serializer = JsonSerializer(True);
                descA.serialize(serializer);
                auto valueDataDescStrA = serializer.getOutput();
                serializer.reset();
                descB.serialize(serializer);
                auto valueDataDescStrB = serializer.getOutput();
                serializer.reset();

                LOG_E("decs A - \nvalue:\n\"{}\"", valueDataDescStrA);
                LOG_E("decs B - \nvalue:\n\"{}\"", valueDataDescStrB);
            }
            else
            {
                LOG_E("decs A - \nvalue:\n\"{}\"", descA.assigned() ? descA.toString() : "null");
                LOG_E("decs B - \nvalue:\n\"{}\"", descB.assigned() ? descB.toString() : "null");
            }
            return false;
        }
        else
        {
            return true;
        }
    }

    [[maybe_unused]]
    static bool packetsEqual(
        const ListPtr<IPacket>& inputListA,
        const ListPtr<IPacket>& inputListB,
        bool skipEventPackets = false,
        bool skipLTMissingDescFields = false)
    {
        auto context = NullContext();
        auto loggerComponent = context.getLogger().getOrAddComponent("packetsEqual");

        bool result = true;
        ListPtr<IPacket> listA;
        ListPtr<IPacket> listB;

        if (skipEventPackets)
        {
            // keep only data packets
            listA = List<IPacket>();
            listB = List<IPacket>();

            for (const auto& item : inputListA)
                if (item.getType() == PacketType::Data)
                    listA.pushBack(item);

            for (const auto& item : inputListB)
                if (item.getType() == PacketType::Data)
                    listB.pushBack(item);
        }
        else
        {
            listA = inputListA;
            listB = inputListB;
        }

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
                    auto eventPacketA = listA.getItemAt(i).asPtr<IEventPacket>(true);
                    auto eventPacketB = listB.getItemAt(i).asPtr<IEventPacket>(true);

                    if (eventPacketA.getEventId() != eventPacketB.getEventId())
                    {
                        LOG_E("Event id of packets at index {} differs: A - \"{}\", B - \"{}\"",
                              i, eventPacketA.getEventId(), eventPacketB.getEventId());
                        result = false;
                    }
                    else if(eventPacketA.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED &&
                             eventPacketB.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
                    {
                        const DataDescriptorPtr valueDataDescA = eventPacketA.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
                        const DataDescriptorPtr valueDataDescB = eventPacketB.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
                        bool valueDescEqual = equalDescriptors(valueDataDescA, valueDataDescB, loggerComponent, skipLTMissingDescFields);
                        if (!valueDescEqual)
                        {
                            LOG_E("Event parameter value descriptors of packets at index {} differs:", i);
                            result = false;
                        }

                        const DataDescriptorPtr domainDataDescA = eventPacketA.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);
                        const DataDescriptorPtr domainDataDescB = eventPacketB.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);
                        bool domainDescEqual = equalDescriptors(domainDataDescA, domainDataDescB, loggerComponent, skipLTMissingDescFields);
                        if (!domainDescEqual)
                        {
                            LOG_E("Event parameter domain descriptors of packets at index {} differs:", i);
                            result = false;
                        }
                    }
                    else
                    {
                        LOG_E("Event packets at index {} differs: A - \"{}\", B - \"{}\"",
                              i, listA.getItemAt(i).toString(), listB.getItemAt(i).toString());
                        result = false;
                    }
                }
                else if (listA.getItemAt(i).getType() == PacketType::Data &&
                         listB.getItemAt(i).getType() == PacketType::Data)
                {
                    DataPacketPtr packetA = listA.getItemAt(i);
                    DataPacketPtr packetB = listB.getItemAt(i);

                    const DataDescriptorPtr valueDataDescA = packetA.getDataDescriptor();
                    const DataDescriptorPtr valueDataDescB = packetB.getDataDescriptor();
                    if (equalDescriptors(valueDataDescA, valueDataDescB, loggerComponent, skipLTMissingDescFields))
                    {
                        if (packetA.getDomainPacket().assigned() && packetB.getDomainPacket().assigned())
                        {
                            auto domainPacketDescA = packetA.getDomainPacket().getDataDescriptor();
                            auto domainPacketDescB = packetB.getDomainPacket().getDataDescriptor();

                            if (!equalDescriptors(domainPacketDescA,
                                                  domainPacketDescB,
                                                  loggerComponent,
                                                  skipLTMissingDescFields))
                            {
                                LOG_E("Descriptors in domain packets of data packets at index {} differs", i);
                            }
                        }

                        if (!skipLTMissingDescFields && !BaseObjectPtr::Equals(packetA, packetB) ||
                            !equalPackets(packetA, packetB, loggerComponent))
                        {
                            LOG_E("Data packets at index {} differs", i);
                            result = false;
                        }
                    }
                    else
                    {
                        LOG_E("Data descriptors in data packets at index {} differs", i);
                        result = false;
                    }
                }
                else
                {
                    LOG_E("Type of packets at index {} differs: A - \"{}\", B - \"{}\"",
                          i, listA.getItemAt(i).toString(), listB.getItemAt(i).toString());
                    result = false;
                }
            }
        }

        return result;
    }

    [[maybe_unused]]
    static InstancePtr connectInstanceWithClientType(const std::string& connectionString, ClientType clientType, bool dropOthers = false)
    {
        auto clientInstance = Instance();

        auto config = clientInstance.createDefaultAddDeviceConfig();
        PropertyObjectPtr generalConfig = config.getPropertyValue("General");

        generalConfig.setPropertyValue("ClientType", (Int) clientType);
        generalConfig.setPropertyValue("ExclusiveControlDropOthers", dropOthers);

        auto device = clientInstance.addDevice(connectionString, config);
        return clientInstance;
    }

    
    [[maybe_unused]]
    static void checkDeviceOperationMode(const daq::DevicePtr& device, OperationModeType expected, bool isServer = false)
    {
        ASSERT_EQ(device.getOperationMode(), expected);
        bool active = expected != OperationModeType::Idle;
        std::string messagePrefix = isServer ? "Server: " : "Client: ";

        for (const auto& ch: device.getChannels())
        {
            ASSERT_EQ(ch.getActive(), active) << messagePrefix << "Checking ch " << ch.getGlobalId() << " for mode " << static_cast<int>(expected);
            for (const auto& sig: ch.getSignals())
                ASSERT_EQ(sig.getActive(), active) << messagePrefix << "Checking ch signal " << sig.getGlobalId() << " for mode " << static_cast<int>(expected);
        }
    }

    [[maybe_unused]]
    static void testPropObjsEquality(const PropertyObjectPtr& configA, const PropertyObjectPtr& configB, std::string path = "")
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
