/*
 * Copyright 2022-2023 Blueberry d.o.o.
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
#include <opendaq/event_packet.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_readers
 * @addtogroup opendaq_reader Reader status
 * @{
 */

DECLARE_OPENDAQ_INTERFACE(IReaderStatus, IBaseObject)
{
    virtual ErrCode INTERFACE_FUNC isOk(Bool* status) = 0;

    virtual ErrCode INTERFACE_FUNC getEventPacket(IEventPacket** packet) = 0;

    virtual ErrCode INTERFACE_FUNC isEventEncountered(Bool* status) = 0;

    virtual ErrCode INTERFACE_FUNC isConvertable(Bool* status) = 0;   
};
/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY (
    LIBRARY_FACTORY, ReaderStatus,
    IEventPacket*, eventPacket,
    Bool, convertable
)

END_NAMESPACE_OPENDAQ
