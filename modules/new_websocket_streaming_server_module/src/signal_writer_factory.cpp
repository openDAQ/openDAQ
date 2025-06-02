#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#include <opendaq/opendaq.h>

#include "constant_signal_writer.hpp"
#include "explicit_signal_writer.hpp"
#include "linear_signal_writer.hpp"
#include "signal_writer_factory.hpp"
#include "websocket_client_established.hpp"

daq::ws_streaming::signal_writer_factory daq::ws_streaming::create_signal_writer_factory(daq::SignalPtr& signal)
{
    if (!signal.getDescriptor().assigned())
    {
        std::cerr << "[ws-streaming] can't determine appropriate signal_writer for '"
            << signal.getGlobalId() << "': signal has no descriptor" << std::endl;
        return {};
    }

    if (!signal.getDescriptor().getRule().assigned())
    {
        std::cerr << "[ws-streaming] can't determine appropriate signal_writer for '"
            << signal.getGlobalId() << "': signal descriptor has no attached rule" << std::endl;
        return {};
    }

    switch (signal.getDescriptor().getRule().getType())
    {
        case daq::DataRuleType::Constant:
            return [](unsigned signo, websocket_client_established& client)
                { return std::make_unique<constant_signal_writer>(signo, client); };

        case daq::DataRuleType::Linear:
            return [](unsigned signo, websocket_client_established& client)
                { return std::make_unique<linear_signal_writer>(signo, client); };

        case daq::DataRuleType::Explicit:
            return [](unsigned signo, websocket_client_established& client)
                { return std::make_unique<explicit_signal_writer>(signo, client); };

        default:
            std::cerr << "[ws-streaming] can't determine appropriate signal_writer for '"
                << signal.getGlobalId() << "': unsupported rule: "
                << std::to_string(static_cast<int>(signal.getDescriptor().getRule().getType())) << std::endl;
            return {};
    }
}
