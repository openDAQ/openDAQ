#pragma once

#include <string>

#include <opendaq/opendaq.h>

#include <nlohmann/json.hpp>

namespace daq::ws_streaming
{
    /**
     * Generates WebSocket Streaming Protocol JSON metadata describing a signal. The description
     * is based on the signal's descriptor object.
     *
     * @param signal The signal to describe.
     *
     * @return A JSON object describing the signal.
     *
     * @throws daq::NotSupportedException This signal type is not supported.
     */
    nlohmann::json to_metadata(
        std::string id,
        const daq::DataDescriptorPtr& descriptor,
        std::string description,
        std::string domain_signal_id);
}
