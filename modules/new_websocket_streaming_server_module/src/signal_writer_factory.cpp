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
        throw std::domain_error("can't determine appropriate signal_writer implementation: signal has no descriptor or no attached rule");
    if (!signal.getDescriptor().getRule().assigned())
        throw std::domain_error("can't determine appropriate signal_writer implementation: signal descriptor has no attached rule");

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
            throw std::domain_error(
                "can't determine appropriate signal_writer implementation: unsupported rule: " +
                std::to_string(static_cast<int>(signal.getDescriptor().getRule().getType())));
    }
}
