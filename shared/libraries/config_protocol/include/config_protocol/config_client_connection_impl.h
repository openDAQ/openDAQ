/*
 * Copyright 2022-2024 openDAQ d. o. o.
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
#include <opendaq/connection.h>
#include <opendaq/input_port_config_ptr.h>

#include "coretypes/validation.h"

namespace daq::config_protocol
{

class ConfigClientConnectionImpl : public ImplementationOfWeak<IConnection>
{
public:
    explicit ConfigClientConnectionImpl(const InputPortPtr& port, const SignalPtr& signal, const ContextPtr& context);

    ErrCode INTERFACE_FUNC enqueue(IPacket* packet) override;
    ErrCode INTERFACE_FUNC enqueueMultiple(IList* packet) override;
    ErrCode INTERFACE_FUNC enqueueAndStealRef(IPacket* packet) override;
    ErrCode INTERFACE_FUNC enqueueMultipleAndStealRef(IList* packet) override;
    ErrCode INTERFACE_FUNC enqueueOnThisThread(IPacket* packet) override;
    ErrCode INTERFACE_FUNC dequeue(IPacket** packet) override;
    ErrCode INTERFACE_FUNC dequeueAll(IList** packet) override;
    ErrCode INTERFACE_FUNC peek(IPacket** packet) override;
    ErrCode INTERFACE_FUNC getPacketCount(SizeT* packetCount) override;
    ErrCode INTERFACE_FUNC getSignal(ISignal** signal) override;
    ErrCode INTERFACE_FUNC getInputPort(IInputPort** inputPort) override;

    ErrCode INTERFACE_FUNC getAvailableSamples(SizeT* samples) override;
    ErrCode INTERFACE_FUNC getSamplesUntilNextDescriptor(SizeT* samples) override;

    ErrCode INTERFACE_FUNC isRemote(Bool* remote) override;

private:
    InputPortConfigPtr port;
    WeakRefPtr<ISignal> signalRef;
    ContextPtr context;
};

inline ConfigClientConnectionImpl::ConfigClientConnectionImpl(const InputPortPtr& port, const SignalPtr& signal, const ContextPtr& context)
    : port(port)
    , signalRef(signal)
    , context(context)
{
}

inline ErrCode ConfigClientConnectionImpl::enqueue(IPacket* packet)
{
    return OPENDAQ_IGNORED;
}

inline ErrCode ConfigClientConnectionImpl::enqueueMultiple(IList* packet)
{
    return OPENDAQ_IGNORED;
}

inline ErrCode INTERFACE_FUNC ConfigClientConnectionImpl::enqueueAndStealRef(IPacket* packets)
{
    OPENDAQ_PARAM_NOT_NULL(packets);

    packets->releaseRef();

    return OPENDAQ_IGNORED;
}

inline ErrCode INTERFACE_FUNC ConfigClientConnectionImpl::enqueueMultipleAndStealRef(IList* packets)
{
    OPENDAQ_PARAM_NOT_NULL(packets);

    packets->releaseRef();

    return OPENDAQ_IGNORED;
}

inline ErrCode ConfigClientConnectionImpl::enqueueOnThisThread(IPacket* packet)
{
    return OPENDAQ_IGNORED;
}

inline ErrCode ConfigClientConnectionImpl::dequeue(IPacket** packet)
{
    OPENDAQ_PARAM_NOT_NULL(packet);

    *packet = nullptr;
    return OPENDAQ_SUCCESS;
}

inline ErrCode INTERFACE_FUNC ConfigClientConnectionImpl::dequeueAll(IList** packets)
{
    OPENDAQ_PARAM_NOT_NULL(packets);

    *packets = List<IPacket>().detach();
    return OPENDAQ_SUCCESS;
}

inline ErrCode ConfigClientConnectionImpl::peek(IPacket** packet)
{
    OPENDAQ_PARAM_NOT_NULL(packet);

    *packet = nullptr;
    return OPENDAQ_SUCCESS;
}

inline ErrCode ConfigClientConnectionImpl::getPacketCount(SizeT* packetCount)
{
    OPENDAQ_PARAM_NOT_NULL(packetCount);

    *packetCount = 0;
    return OPENDAQ_SUCCESS;
}

inline ErrCode ConfigClientConnectionImpl::getSignal(ISignal** signal)
{
    OPENDAQ_PARAM_NOT_NULL(signal);

    return daqTry(
        [this, &signal]
        {
            *signal = this->signalRef.getRef().detach();
        });
}

inline ErrCode ConfigClientConnectionImpl::getInputPort(IInputPort** inputPort)
{
    OPENDAQ_PARAM_NOT_NULL(inputPort);

    *inputPort = this->port.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

inline ErrCode ConfigClientConnectionImpl::getAvailableSamples(SizeT* samples)
{
    OPENDAQ_PARAM_NOT_NULL(samples);

    *samples = 0;
    return OPENDAQ_SUCCESS;
}

inline ErrCode ConfigClientConnectionImpl::getSamplesUntilNextDescriptor(SizeT* samples)
{
    OPENDAQ_PARAM_NOT_NULL(samples);

    *samples = 0;
    return OPENDAQ_SUCCESS;
}

inline ErrCode ConfigClientConnectionImpl::isRemote(Bool* remote)
{
    OPENDAQ_PARAM_NOT_NULL(remote);

    *remote = True;
    return OPENDAQ_SUCCESS;
}

}
