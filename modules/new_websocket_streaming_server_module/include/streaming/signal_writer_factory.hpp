#pragma once

#include <opendaq/opendaq.h>

#include <functional>
#include <memory>

#include "signal_writer.hpp"
#include "websocket_client_established.hpp"

namespace daq::ws_streaming
{
    /**
     * A std::function specialization for function objects which construct and return a unique
     * pointer to a signal_writer object configured with a specified WebSocket Streaming Protocol
     * "signo" number, a reference to a WebSocket client object, and an openDAQ signal object.
     */
    typedef std::function<
        std::unique_ptr<signal_writer>(
            unsigned signo,
            websocket_client_established& client
        )> signal_writer_factory;

    /**
     * Returns a signal_writer_factory which generates signal_writer objects appropriate for the
     * specified type of signal.
     *
     * @param signal A reference to the openDAQ signal object. The signal's descriptor will be
     *     examined to determine the correct signal_writer implementation for this type of signal.
     *
     * @return A function object which, when invoked, will construct a signal_writer object of the
     *     appropriate type, configured with the provided "signo" number, client reference,
     *     openDAQ signal object reference and sample counter.
     *
     * @throws std::domain_error No signal_writer implementations are appropriate for this signal.
     */
    signal_writer_factory create_signal_writer_factory(daq::SignalPtr& signal);
}
