/*
 * Copyright 2022-2026 openDAQ d.o.o.
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

#include <coretypes/intfs.h>
#include <opendaq/data_packet_ptr.h>
#include <opendaq/input_port_notifications.h>
#include <opendaq/input_port_ptr.h>

#include <atomic>

namespace daq::test_helpers
{

// Drains the port in the notification and keeps only counts. The port holds the listener weakly,
// so the fixture keeps it alive.
struct CountingListener : ImplementationOfWeak<IInputPortNotifications>
{
    CountingListener(std::atomic<uint64_t>& own,
                     std::atomic<uint64_t>& totalPackets,
                     std::atomic<uint64_t>& totalBytes,
                     std::atomic<uint64_t>& totalEvents)
        : own(own)
        , totalPackets(totalPackets)
        , totalBytes(totalBytes)
        , totalEvents(totalEvents)
    {
    }

    ErrCode INTERFACE_FUNC acceptsSignal(IInputPort*, ISignal*, Bool* accept) override
    {
        *accept = True;
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC connected(IInputPort*) override
    {
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC disconnected(IInputPort*) override
    {
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC packetReceived(IInputPort* port) override
    {
        const auto connection = InputPortPtr::Borrow(port).getConnection();
        if (!connection.assigned())
            return OPENDAQ_SUCCESS;

        for (PacketPtr packet = connection.dequeue(); packet.assigned(); packet = connection.dequeue())
        {
            if (packet.getType() != daq::PacketType::Data)
            {
                totalEvents.fetch_add(1, std::memory_order_relaxed);
                continue;
            }
            own.fetch_add(1, std::memory_order_relaxed);
            totalPackets.fetch_add(1, std::memory_order_relaxed);
            totalBytes.fetch_add(packet.asPtr<IDataPacket>(true).getRawDataSize(), std::memory_order_relaxed);
        }
        return OPENDAQ_SUCCESS;
    }

    std::atomic<uint64_t>& own;
    std::atomic<uint64_t>& totalPackets;
    std::atomic<uint64_t>& totalBytes;
    std::atomic<uint64_t>& totalEvents;
};

}
