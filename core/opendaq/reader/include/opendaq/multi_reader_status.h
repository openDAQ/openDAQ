/*
 * Copyright 2022-2024 Blueberry d.o.o.
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
#include <opendaq/reader_status.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_readers
 * @addtogroup opendaq_reader Multi reader status
 * @{
 */

/*#
 * [interfaceSmartPtr(IReaderStatus, GenericReaderStatusPtr)]
 */

/*!
 * @brief IMultiReaderStatus inherits from IReaderStatus to expand information returned read function
 */
DECLARE_OPENDAQ_INTERFACE(IMultiReaderStatus, IReaderStatus)
{
    // [elementType(eventPackets, IEventPacket)]
    /*!
     * @brief Retrieves the event packets from the reading process ordered by signals (Have to used instead of IReaderStatus::getEventPacket()).
     * @param[out] eventPackets The event packet from the reading process ordered by signals.
     */
    virtual ErrCode INTERFACE_FUNC getEventPackets(IList** eventPackets) = 0;
};
/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY (
    LIBRARY_FACTORY, MultiReaderStatus,
    IList*, eventPackets,
    Bool, valid
)

END_NAMESPACE_OPENDAQ