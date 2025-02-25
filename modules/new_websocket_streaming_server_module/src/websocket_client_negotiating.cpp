#include <exception>
#include <iostream>
#include <utility>

#include <boost/asio.hpp>

#include "httpparser_utils.hpp"
#include "websocket_client.hpp"
#include "websocket_client_negotiating.hpp"
#include "websocket_protocol.hpp"

daq::ws_streaming::websocket_client_negotiating::websocket_client_negotiating(boost::asio::ip::tcp::socket&& socket)
    : websocket_client(std::move(socket))
{
}

bool daq::ws_streaming::websocket_client_negotiating::service()
{
    boost::system::error_code ec;

    std::size_t bytes_read = socket.read_some(boost::asio::buffer(read_buffer), ec);

    if (ec)
    {
        if (ec == boost::asio::error::would_block)
            return true;

        std::cerr << "[ws-streaming] client (negotiating): recv returned errno " << ec << std::endl;
        return false;
    }

    else if (bytes_read == 0)
    {
        std::cerr << "[ws-streaming] client (negotiating): client disconnected (recv 0)" << std::endl;
        return false;
    }

    auto parse_result = http_request_parser.parse(http_request, read_buffer.data(), read_buffer.data() + bytes_read);
    switch (parse_result)
    {
        case httpparser::HttpRequestParser::ParsingCompleted:
        {
            auto key_header = get_header(http_request, "Sec-WebSocket-Key");
            if (!key_header || get_header(http_request, "Upgrade") != "websocket")
            {
                nlohmann::json content;

                try
                {
                    content = nlohmann::json::parse(http_request.content);
                }

                catch (const nlohmann::json::exception& ex)
                {
                    std::cout << "[ws-streaming] json parse error: " << ex.what() << std::endl;
                    return false;
                }

                if (on_request)
                {
                    std::ostringstream response_stream;
                    std::string response_content;

                    if (on_request(content))
                    {
                        response_stream << "HTTP/1.1 200 OK\r\n";
                        response_content = "Succeeded";
                    }

                    else
                    {
                        response_stream << "HTTP/1.1 400 Bad Request\r\n";
                        response_content = "Unspecified Error";
                    }

                    response_stream << "Content-Type: text/plain\r\n";
                    response_stream << "Content-Length: " << response_content.length() << "\r\n";
                    response_stream << "\r\n";

                    auto response_str = response_stream.str();

                    boost::asio::write(socket, boost::asio::buffer(response_str.data(), response_str.length()), ec);
                    boost::asio::write(socket, boost::asio::buffer(response_content.data(), response_content.length()), ec);
                }

                return false;
            }

            std::ostringstream response_stream;
            websocket_protocol::generate_upgrade_response(response_stream, key_header.value());
            auto response_str = response_stream.str();

            boost::asio::write(socket, boost::asio::buffer(response_str.data(), response_str.length()), ec);
            if (ec)
                return false;

            if (on_establish)
            {
                try
                {
                    return on_establish();                    
                }

                catch (const std::exception& ex)
                {
                    std::cerr << "[ws-streaming] on_establish threw an exception: " << ex.what() << std::endl;
                    return false;
                }
            }

            break;
        }

        case httpparser::HttpRequestParser::ParsingIncompleted:
            break;

        default:
            return false;
    }

    return true;
}
