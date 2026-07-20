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
#include <coretypes/common.h>
#include <coretypes/baseobject.h>
#include <coretypes/listobject.h>
#include <opendaq/packet.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_signal_path
 * @addtogroup opendaq_connection ConnectionInternal
 * @{
 */

DECLARE_OPENDAQ_INTERFACE(IConnectionInternal, IBaseObject)
{
    /*!
     * @brief Enqueues an event packet with the last descriptor at the front of the queue.
     */
    virtual ErrCode INTERFACE_FUNC enqueueLastDescriptor() = 0;
	virtual ErrCode INTERFACE_FUNC dequeueUpTo(IPacket** packetPtr, SizeT* count) = 0;

    /*!
     * @brief Lock-free enqueue reserved for the signal's single producer thread.
     *
     * Signals guarantee a single producer thread per signal; these entry points may use
     * producer-thread-private state (a private free-node cache) and MUST NOT be called
     * from any other thread. All other callers use the generic enqueue methods.
     */
    virtual ErrCode INTERFACE_FUNC enqueueProducer(IPacket* packet) = 0;

    /*!
     * @brief Same as enqueueProducer, but steals the packet reference.
     */
    virtual ErrCode INTERFACE_FUNC enqueueProducerAndStealRef(IPacket* packet) = 0;

    /*!
     * @brief Lock-free multi-packet enqueue reserved for the signal's single producer thread.
     */
    virtual ErrCode INTERFACE_FUNC enqueueMultipleProducer(IList* packets) = 0;

    /*!
     * @brief Same as enqueueMultipleProducer, but steals the list reference.
     */
    virtual ErrCode INTERFACE_FUNC enqueueMultipleProducerAndStealRef(IList* packets) = 0;

    /*!
     * @brief Permanently closes the queue: drops all queued packets and rejects all future
     * enqueues. Called by the signal when the connection is disconnected so that no packet
     * can remain pinned in (or be added to) an orphaned queue.
     */
    virtual ErrCode INTERFACE_FUNC closeQueue() = 0;
};

/*!@}*/


END_NAMESPACE_OPENDAQ
