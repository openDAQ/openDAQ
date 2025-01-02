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

#include <native_streaming_protocol/server_session_handler.h>

#include <opendaq/context_ptr.h>
#include <opendaq/logger_ptr.h>
#include <opendaq/logger_component_ptr.h>
#include <opendaq/signal_ptr.h>

#include <packet_streaming/packet_streaming_server.h>
#include <packet_streaming/packet_streaming_client.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL

using SendPacketBufferCallback = std::function<void(const std::string& subscribedClientId,
                                                    packet_streaming::PacketBufferPtr&& packetBuffer)>;

class StreamingManager
{
public:
    explicit StreamingManager(const ContextPtr& context);

    /// Pushes a packet associated with a specified signal ID to the packet streaming servers
    /// associated with clients subscribed to this signal. Retrieves all ready packet buffers from those
    /// servers and sends them to clients using the specified callback.
    /// @param signalStringId The unique string ID of the signal.
    /// @param packet The openDAQ packet to be processed and delivered to the client.
    /// @param sendPacketBufferCb The callback used to send the created packet buffer to the client.
    /// @throw NativeStreamingProtocolException if the signal is not registered.
    void sendPacketToSubscribers(const std::string& signalStringId,
                                 PacketPtr&& packet,
                                 const SendPacketBufferCallback& sendPacketBufferCb);

    /// Registers a signal using its global ID as a unique key
    /// and assigns a numeric ID to it.
    /// @param signal The openDAQ signal to register.
    /// @return The assigned numeric ID, which is unique within the server.
    /// @throw NativeStreamingProtocolException if the signal is already registered
    SignalNumericIdType registerSignal(const SignalPtr& signal);

    /// Removes a registered signal, usually when the signal is being removed from a device.
    /// @param signal The openDAQ signal to unregister.
    /// @return true if the removed signal was subscribed, false otherwise.
    /// @throw NativeStreamingProtocolException if the signal is not registered
    bool removeSignal(const SignalPtr& signal);

    /// Registers a connected client as a streaming client.
    /// @param clientId The unique string ID provided by the client or automatically assigned by the server.
    /// @param reconnected true if the client was reconnected, false otherwise.
    /// @param enablePacketBufferTimestamps enables timestamp creation for PacketBuffers
    /// @throw NativeStreamingProtocolException if the client is already registered.
    void registerClient(const std::string& clientId, bool reconnected, bool enablePacketBufferTimestamps);

    /// Removes a registered client on disconnection.
    /// @param clientId The unique string ID provided by the client or automatically assigned by the server.
    /// @return A list of openDAQ signals which were subscribed to by the client
    /// and do not have more subscribers after the client's disconnection.
    /// Returns an empty list if the disconnected client was not registered as a streaming client.
    ListPtr<ISignal> unregisterClient(const std::string& clientId);

    /// Adds a client with the specified ID to the list of signal subscribers.
    /// @param signalStringId The unique string ID of the signal.
    /// @param subscribedClientId The ID of the client to be registered as a subscriber.
    /// @param sendPacketBufferCb The callback used to send the packet buffer created from initial event packet to the client.
    /// @return true if no subscribers were registered before, false otherwise.
    /// @throw NativeStreamingProtocolException if the signal or client is not registered.
    bool registerSignalSubscriber(const std::string& signalStringId,
                                  const std::string& subscribedClientId,
                                  const SendPacketBufferCallback& sendPacketBufferCb);

    /// Removes a client with the specified ID from the list of signal subscribers.
    /// @param signalStringId The unique string ID of the signal.
    /// @param subscribedClientId The ID of the client to be removed from the subscribers.
    /// @return true if the list of signal subscribers becomes empty after removing the client with the specified ID,
    /// false otherwise.
    /// @throw NativeStreamingProtocolException if the signal or client is not registered.
    bool removeSignalSubscriber(const std::string& signalStringId, const std::string& subscribedClientId);

    /// Gets an openDAQ signal registered under the specified ID.
    /// @param signalStringId The unique string ID of the signal.
    /// @return The found openDAQ signal.
    /// @throw NativeStreamingProtocolException if the signal is not registered.
    SignalPtr findRegisteredSignal(const std::string& signalStringId);

    /// Gets a unique numeric ID assigned to a specified registered openDAQ signal.
    /// @param signal The registered openDAQ signal.
    /// @return The unique numeric ID of the registered signal.
    /// @throw NativeStreamingProtocolException if the signal is not registered.
    SignalNumericIdType findSignalNumericId(const SignalPtr& signal);

    /// Gets a std::map of registered signals where the openDAQ signal is a value and the unique numeric ID of the signal
    /// is a key. This way, signals are automatically sorted by the order of registration in streaming.
    /// @return A std::map with signal numeric IDs as keys and openDAQ signals as values.
    std::map<SignalNumericIdType, SignalPtr> getRegisteredSignals();

    /// Gets the IDs of registered streaming clients.
    /// @return A std::vector containing the client IDs of the registered streaming clients.
    std::vector<std::string> getRegisteredClientsIds();

private:
    using PacketStreamingServerPtr = std::shared_ptr<packet_streaming::PacketStreamingServer>;
    struct RegisteredSignal
    {
        explicit RegisteredSignal(SignalPtr daqSignal, SignalNumericIdType numericId);

        SignalPtr daqSignal;
        SignalNumericIdType numericId;
        std::unordered_set<std::string> subscribedClientsIds;
        DataDescriptorPtr lastDataDescriptorParam;
        DataDescriptorPtr lastDomainDescriptorParam;
    };

    void sendDaqPacket(const SendPacketBufferCallback& sendPacketBufferCb,
                       const PacketStreamingServerPtr& registeredSignal,
                       PacketPtr&& packet,
                       const std::string& clientId,
                       SignalNumericIdType singalNumericId);

    bool removeSignalSubscriberNoLock(const std::string& signalStringId, const std::string& subscribedClientId);

    ContextPtr context;
    LoggerComponentPtr loggerComponent;
    SignalNumericIdType signalNumericIdCounter;

    // key: signal global id
    std::unordered_map<std::string, RegisteredSignal> registeredSignals;

    std::unordered_map<std::string, PacketStreamingServerPtr> packetStreamingServers;
    std::unordered_set<std::string> streamingClientsIds;

    std::mutex sync;
};

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL
