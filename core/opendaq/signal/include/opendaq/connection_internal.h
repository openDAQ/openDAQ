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
#include <coretypes/common.h>
#include <coretypes/baseobject.h>

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
};

/*!@}*/


END_NAMESPACE_OPENDAQ
