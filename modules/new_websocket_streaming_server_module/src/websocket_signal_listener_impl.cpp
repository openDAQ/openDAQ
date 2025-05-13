#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <utility>

#include <boost/asio.hpp>

#include <opendaq/event_packet_params.h>
#include <opendaq/opendaq.h>

#include "metadata.hpp"
#include "signal_writer_factory.hpp"
#include "websocket_client_established.hpp"
#include "websocket_protocol.hpp"
#include "websocket_signal_listener_impl.hpp"

using namespace std::chrono_literals;

daq::ws_streaming::WebSocketSignalListenerImpl::WebSocketSignalListenerImpl(
        daq::IDevice *device_ptr,
        daq::ISignal *signal_ptr,
        unsigned signo)
    : signal(signal_ptr)
    , port(daq::InputPort(daq::DevicePtr(device_ptr).getContext(), nullptr, daq::String("hbk-ws-streaming")))
    , signo(signo)
    , writer_factory(create_signal_writer_factory(signal))
{
    // XXX TODO: We should lazy-initialize metadata and handle signals that
    // change writer types (maybe even from unsupported to supported) at runtime.
    if (!writer_factory)
        throw std::runtime_error("Signal '" + signal.getGlobalId() + "' is of an unsupported type");

    internalAddRef();
}

daq::ErrCode daq::ws_streaming::WebSocketSignalListenerImpl::acceptsSignal(daq::IInputPort *port, daq::ISignal *signal, daq::Bool *accept)
{
    daq::SignalPtr signalPtr(signal);

    *accept = true;

    return OPENDAQ_SUCCESS;
};

daq::ErrCode daq::ws_streaming::WebSocketSignalListenerImpl::connected(daq::IInputPort *port)
{
    return OPENDAQ_SUCCESS;
}

daq::ErrCode daq::ws_streaming::WebSocketSignalListenerImpl::disconnected(daq::IInputPort *port)
{
    return OPENDAQ_SUCCESS;
}

daq::ErrCode daq::ws_streaming::WebSocketSignalListenerImpl::packetReceived(daq::IInputPort *port)
{
    while (true)
    {
        auto packet = this->port.getConnection().dequeue();
        if (!packet.assigned())
            break;

        if (packet.getType() == daq::PacketType::Data)
        {
            auto data = daq::DataPacketPtr(packet);
            auto domain_packet = data.getDomainPacket();

            if (data.getRawData())
            {
                std::scoped_lock lock(mutex);

                auto it = clients.begin();
                while (it != clients.end())
                {
                    auto client = it->client.lock();

                    if (!client)
                    {
                        auto jt = it++;
                        clients.erase(jt);
                        continue;
                    }

                    if (domain_packet.assigned()
                        && domain_listener
                        && domain_packet.getDataDescriptor().assigned()
                        && domain_packet.getDataDescriptor().getRule().assigned()
                        && domain_packet.getDataDescriptor().getRule().getType() == daq::DataRuleType::Linear
                        && data.getDataDescriptor().getRule().assigned()
                        && data.getDataDescriptor().getRule().getType() == daq::DataRuleType::Explicit)
                    {
                        auto [start, delta] = checkDomainPacket(domain_packet);
                        std::int64_t offset = domain_packet.getOffset().getIntValue();

                        if (!it->domain_value.has_value() ||
                            it->domain_value.value() + it->samples_since_signal_update * delta != start + offset)
                        {
                            it->domain_value = start + offset - delta * it->samples_since_signal_update;
                            daq::ws_streaming::websocket_protocol::constant_value_packet data_packet { };
                            data_packet.index = 0;
                            data_packet.value = it->domain_value.value();
                            if (!client->send_data(domain_listener->signo, &data_packet, sizeof(data_packet)))
                            {
                                boost::system::error_code ec;
                                client->get_socket().shutdown(boost::asio::socket_base::shutdown_both, ec);
                                auto jt = it++;
                                clients.erase(jt);
                                continue;
                            }
                        }
                    }

                    if (!it->writer->write(data))
                    {
                        boost::system::error_code ec;
                        client->get_socket().shutdown(boost::asio::socket_base::shutdown_both, ec);
                        auto jt = it++;
                        clients.erase(jt);
                        continue;
                    }

                    it->samples_since_signal_update += data.getSampleCount();
                    ++it;
                }
            }
        }

        else if (packet.getType() == daq::PacketType::Event)
        {
            auto event = daq::EventPacketPtr(packet);

            if (event.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
            {
                std::scoped_lock lock(mutex);

                daq::DataDescriptorPtr descriptor = event.getParameters().get(event_packet_param::DATA_DESCRIPTOR);

                auto it = clients.begin();
                while (it != clients.end())
                {
                    auto client = it->client.lock();

                    nlohmann::json metadata;
                    bool success = false;

                    try
                    {
                        metadata = to_metadata(it->id, descriptor, it->description, it->domain_signal_id);
                        success = true;
                    }

                    catch (const std::exception& ex)
                    {
                        std::cerr << "[ws-streaming] failed to generate metadata for modified signal " << it->id << ": " << ex.what() << std::endl;
                    }

                    if (!success || !client || !client->send_metadata(signo, {
                        { "method", "signal" },
                        { "params", metadata }
                    }))
                    {
                        if (client)
                        {
                            boost::system::error_code ec;
                            client->get_socket().shutdown(boost::asio::socket_base::shutdown_both, ec);
                        }
                        auto jt = it++;
                        clients.erase(jt);
                    }

                    else
                    {
                        it->samples_since_signal_update = 0;
                        ++it;
                    }
                }
            }
        }
    }

    return OPENDAQ_SUCCESS;
}

bool daq::ws_streaming::WebSocketSignalListenerImpl::addClient(
    std::shared_ptr<websocket_client_established> client,
    bool isImplicit)
{
    std::scoped_lock lock(mutex);

    auto it = std::find_if(clients.begin(), clients.end(),
        [this, socket = client->get_socket().native_handle()](const subscribed_client& s)
        {
            auto client = s.client.lock();
            return client && client->get_socket().native_handle() == socket;
        });

    if (it != clients.end())
    {
        if (it->is_implicit && !isImplicit)
            it->is_implicit = false;
        return true;
    }

    if (!client->send_metadata(signo, {
        { "method", "subscribe" },
        { "params", { { "signalId", signal.getGlobalId() } } },
    })) return false;

    std::string id = signal.getGlobalId();
    std::string description = signal.getDescription();
    std::string domain_signal_id = signal.getDomainSignal().assigned() ? signal.getDomainSignal().getGlobalId() : "";

    nlohmann::json metadata;

    try
    {
        metadata = to_metadata(id, signal.getDescriptor(), description, domain_signal_id);
    }

    catch (const std::exception& ex)
    {
        std::cerr << "[ws-streaming] failed to generate metadata for requested signal " << id << ": " << ex.what() << std::endl;
        return false;
    }

    if (!client->send_metadata(signo, {
        { "method", "signal" },
        { "params", metadata }
    })) return false;

    auto writer = writer_factory(signo, *client);
    auto& subscribed_client = clients.emplace_back(client, std::move(writer));

    subscribed_client.id = id;
    subscribed_client.description = description;
    subscribed_client.domain_signal_id = domain_signal_id;
    subscribed_client.is_implicit = isImplicit;

    return true;
}

void daq::ws_streaming::WebSocketSignalListenerImpl::removeClient(
    boost::asio::detail::socket_type socket,
    bool onlyIfImplicit)
{
    std::scoped_lock lock(mutex);

    clients.remove_if([this, socket, onlyIfImplicit](const subscribed_client& s)
    {
        auto client = s.client.lock();
        if (!client)
            return true;

        if (onlyIfImplicit && !s.is_implicit)
            return false;

        else if (client->get_socket().native_handle() == socket)
        {
            if (!client->send_metadata(signo, {
                { "method", "unsubscribe" },
                { "params", { { "signalId", signal.getGlobalId() } } },
            })) return false;

            return true;
        }

        return false;
    });
}

void daq::ws_streaming::WebSocketSignalListenerImpl::linkDomainSignal(
    const std::function<WebSocketSignalListenerImpl *(const std::string& name)>& findListener)
{
    if (!signal.getDomainSignal().assigned() ||
            !signal.getDescriptor().getRule().assigned())
        return;

    domain_listener = findListener(signal.getDomainSignal().getGlobalId());
}

void daq::ws_streaming::WebSocketSignalListenerImpl::start()
{
    port.setListener(this->template thisPtr<daq::InputPortNotificationsPtr>());
    port.setNotificationMethod(daq::PacketReadyNotification::SameThread);
    port.connect(signal);
}

std::pair<std::int64_t, std::int64_t>
daq::ws_streaming::WebSocketSignalListenerImpl::checkDomainPacket(daq::DataPacketPtr packet)
{
    if (packet != last_packet)
    {
        last_packet = packet;

        auto descriptor = packet.getDataDescriptor();
        if (descriptor != last_descriptor)
        {
            last_descriptor = descriptor;

            auto rule = descriptor.getRule();
            if (rule.assigned() && rule.getType() == daq::DataRuleType::Linear)
            {
                auto params = rule.getParameters();
                if (params.assigned())
                {
                    daq::BaseObjectPtr start_ptr;
                    if (params.tryGet("start", start_ptr))
                        linear_start = start_ptr;

                    daq::BaseObjectPtr delta_ptr;
                    if (params.tryGet("delta", delta_ptr))
                        linear_delta = delta_ptr;
                }
            }
        }
    }

    return std::make_pair(linear_start, linear_delta);
}

namespace daq::ws_streaming
{
    OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
        INTERNAL_FACTORY, WebSocketSignalListener, daq::IInputPortNotifications,
        daq::IDevice *, device,
        daq::ISignal *, signal,
        unsigned, signo
    )
}
