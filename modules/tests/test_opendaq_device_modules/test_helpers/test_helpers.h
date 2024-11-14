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
    static bool packetsEqual(const ListPtr<IPacket>& listA, const ListPtr<IPacket>& listB, bool skipEventPackets = false)
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

    static InstancePtr connectIntanceWithClientType(const std::string& connectionString, ClientType clientType, bool dropOthers = false)
    {
        auto clientInstance = Instance();

        auto config = clientInstance.createDefaultAddDeviceConfig();
        PropertyObjectPtr generalConfig = config.getPropertyValue("General");

        generalConfig.setPropertyValue("ClientType", (Int) clientType);
        generalConfig.setPropertyValue("ExclusiveControlDropOthers", dropOthers);

        auto device = clientInstance.addDevice(connectionString, config);
        return clientInstance;
    }

}

END_NAMESPACE_OPENDAQ
