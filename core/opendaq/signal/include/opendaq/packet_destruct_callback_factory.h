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
#include <opendaq/packet_destruct_callback_ptr.h>
#include <opendaq/packet_destruct_callback_impl.h>
#include <coretypes/objectptr.h>


BEGIN_NAMESPACE_OPENDAQ
    /*!
 * @ingroup opendaq_packets
 * @addtogroup opendaq_packet Packet
 * @{
 */

/*!
 * @brief Creates packet destruct callback that is used to subscribe to packet destruction
 */
template <class Callback>
PacketDestructCallbackPtr PacketDestructCallback(Callback callback)
{
    auto obj = createWithImplementation<IPacketDestructCallback, PacketDestructCallbackImpl<Callback>>(callback);
    return obj;
}

/*!@}*/

END_NAMESPACE_OPENDAQ
