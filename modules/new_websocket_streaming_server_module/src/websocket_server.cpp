#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <system_error>
#include <utility>

#include <boost/asio.hpp>

#include <opendaq/opendaq.h>

#include "metadata.hpp"
#include "websocket_client.hpp"
#include "websocket_client_established.hpp"
#include "websocket_client_negotiating.hpp"
#include "websocket_server.hpp"
#include "websocket_signal_listener_impl.hpp"

constexpr unsigned BACKLOG = 8;
constexpr unsigned EVENT_DEPTH = 1;

daq::ws_streaming::server::server(daq::DevicePtr device, std::uint16_t ws_port, std::uint16_t control_port)
    : ws_port(ws_port)
    , control_port(control_port)
    , available(nlohmann::json::array())
{
    unsigned signo = 1;

    for (const auto& signal : device.getSignalsRecursive())
    {
        if (listeners.find(signal.getGlobalId()) == listeners.end())
        {
            try
            {
                daq::InputPortNotificationsPtr notifications = WebSocketSignalListener_Create(device, signal, signo++);
                listeners.emplace(std::piecewise_construct, std::make_tuple(signal.getGlobalId()), std::make_tuple(std::move(notifications)));
            }

            catch (const std::exception& ex)
            {
                std::cerr << "[ws-streaming] ignoring signal '"
                    << signal.getGlobalId()
                    << "': " << ex.what() << std::endl;
            }

            available.push_back(signal.getGlobalId());
        }

        if (auto domainSignal = signal.getDomainSignal(); domainSignal.assigned() && listeners.find(domainSignal.getGlobalId()) == listeners.end())
        {
            try
            {
                daq::InputPortNotificationsPtr notifications = WebSocketSignalListener_Create(device, domainSignal, signo++);
                listeners.emplace(std::piecewise_construct, std::make_tuple(domainSignal.getGlobalId()), std::make_tuple(std::move(notifications)));
            }

            catch (const std::exception& ex)
            {
                std::cerr << "[ws-streaming] ignoring signal '"
                    << domainSignal.getGlobalId()
                    << "': " << ex.what() << std::endl;
            }

            available.push_back(domainSignal.getGlobalId());
        }
    }

    auto find_listener = [&](const std::string& id) -> WebSocketSignalListenerImpl *
    {
        for (const auto& [_, listener] : listeners)
            if (id == reinterpret_cast<WebSocketSignalListenerImpl *>(listener.getObject())->getSignal().getGlobalId())
                return reinterpret_cast<WebSocketSignalListenerImpl *>(listener.getObject());
        return nullptr;
    };

    for (const auto& [_, listener] : listeners)
        reinterpret_cast<WebSocketSignalListenerImpl *>(listener.getObject())->linkDomainSignal(find_listener);

    for (const auto& [_, listener] : listeners)
        reinterpret_cast<WebSocketSignalListenerImpl *>(listener.getObject())->start();

    // All listeners have been created for all signals. Start the main thread.
    thread = std::thread([this]() { thread_main(); });
}

daq::ws_streaming::server::~server()
{
    stop();
}

void daq::ws_streaming::server::stop()
{
    if (thread.joinable())
    {
        ioc.stop();
        thread.join();
    }
}

void daq::ws_streaming::server::async_accept_connection(boost::asio::ip::tcp::acceptor& acceptor)
{
    acceptor.async_accept([this, &acceptor](boost::system::error_code ec, boost::asio::ip::tcp::socket&& client_socket)
    {
        auto client = std::make_shared<websocket_client_negotiating>(std::move(client_socket));
        auto client_it = clients.emplace(clients.end(), client);
        std::list<std::shared_ptr<websocket_client>>::iterator it;

        client->on_establish = [this, weak_client = std::weak_ptr(client), client_it]()
            { return on_establish(weak_client, client_it); };

        client->on_finish = [this, client, client_it]()
        {
            for (const auto& [_, listener_ptr] : listeners)
            {
                auto& listener = *reinterpret_cast<WebSocketSignalListenerImpl *>(listener_ptr.getObject());
                listener.removeClient(client->get_socket().native_handle(), false);
            }

            clients.erase(client_it);
        };

        client->on_request = [this](const nlohmann::json& content)
            { return on_request(content); };

        async_accept_connection(acceptor);
    });
}

void daq::ws_streaming::server::thread_main()
{
    try
    {
        boost::asio::ip::tcp::acceptor ws_socket(ioc, boost::asio::ip::tcp::endpoint({}, ws_port), true);
        boost::asio::ip::tcp::acceptor control_socket(ioc, boost::asio::ip::tcp::endpoint({}, control_port), true);

        ws_socket.listen();
        control_socket.listen();

        async_accept_connection(ws_socket);
        async_accept_connection(control_socket);

        ioc.run();
    }

    catch (const std::exception& ex)
    {
        std::cout << "[ws-streaming] main thread error: " << ex.what() << std::endl;
    }
}

bool daq::ws_streaming::server::subscribe(
    std::weak_ptr<websocket_client_established> weak_client,
    const std::string& signal_id,
    bool is_implicit)
{
    std::shared_ptr<websocket_client_established> client = weak_client.lock();
    if (!client)
        return false;

    auto it = listeners.find(signal_id);
    if (it == listeners.end())
        return false;

    auto& listener = *reinterpret_cast<WebSocketSignalListenerImpl *>(it->second.getObject());

    if (listener.getSignal().getDomainSignal().assigned() &&
            !subscribe(weak_client, listener.getSignal().getDomainSignal().getGlobalId(), true))
        return false;

    return listener.addClient(client, is_implicit);
}

bool daq::ws_streaming::server::unsubscribe(
    std::weak_ptr<websocket_client_established> weak_client,
    const std::string& signal_id,
    bool is_implicit)
{
    std::shared_ptr<websocket_client_established> client = weak_client.lock();
    if (!client)
        return false;

    auto it = listeners.find(signal_id);
    if (it != listeners.end())
    {
        auto& listener = *reinterpret_cast<WebSocketSignalListenerImpl *>(it->second.getObject());
        listener.removeClient(client->get_socket().native_handle(), is_implicit);

        if (listener.getSignal().getDomainSignal().assigned() &&
                !unsubscribe(weak_client, listener.getSignal().getDomainSignal().getGlobalId(), true))
            return false;
    }

    return true;
}

bool daq::ws_streaming::server::on_establish(
    std::weak_ptr<websocket_client_negotiating> weak_client,
    std::list<std::shared_ptr<websocket_client>>::iterator client_it)
{
    std::shared_ptr<websocket_client_negotiating> client = weak_client.lock();
    if (!client)
        return false;

    auto client_socket = (*client_it)->release();
    auto new_client = std::make_shared<websocket_client_established>(std::move(client_socket));
    auto new_client_it = clients.emplace(clients.end(), new_client);

    new_client->on_finish = [this, new_client, new_client_it]()
    {
        for (const auto& [_, listener_ptr] : listeners)
        {
            auto& listener = *reinterpret_cast<WebSocketSignalListenerImpl *>(listener_ptr.getObject());
            listener.removeClient(new_client->get_socket().native_handle(), false);
        }

        clients.erase(new_client_it);
    };

    if (!new_client->send_metadata(0, {
        { "method", "apiVersion" },
        { "params",
            { { "version", "1.2.0" } },
        },
    })) return false;

    if (!new_client->send_metadata(0, {
        { "method", "init" },
        { "params",
            {
                { "commandInterfaces",
                    {
                        { "jsonrpc-http",
                            {
                                { "httpMethod", "POST" },
                                { "httpPath", "/" },
                                { "httpVersion", "1.1" },
                                { "port", "7438" },
                            },
                        },
                    },
                },
                { "streamId", new_client->get_stream_id() },
            },
        },
    })) return false;

    if (!new_client->send_metadata(0, {
        { "method", "available" },
        { "params",
            { { "signalIds", available } }
        },
    })) return false;

    return true;
}

bool daq::ws_streaming::server::on_request(const nlohmann::json& content)
{
    if (!content["method"].is_string())
        return false;

    for (const auto& client : clients)
    {
        if (const auto& established_client = std::dynamic_pointer_cast<websocket_client_established>(client))
        {
            const std::string& stream_id = established_client->get_stream_id();
            if (content["method"] == stream_id + ".subscribe" &&
                    content["params"].is_array())
            {
                for (const auto& param : content["params"])
                    if (param.is_string() && !subscribe(established_client, (std::string) param))
                        return false;
                return true;
            }
            else if (content["method"] == stream_id + ".unsubscribe" &&
                    content["params"].is_array() &&
                    content["params"][0].is_string())
            {
                for (const auto& param : content["params"])
                    if (param.is_string() && !unsubscribe(established_client, (std::string) param))
                        return false;
                return true;
            }
        }
    }

    return false;
};
