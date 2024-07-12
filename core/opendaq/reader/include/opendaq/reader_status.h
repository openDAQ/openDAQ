/*
 * Copyright 2022-2024 openDAQ d.o.o.
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
#include <coretypes/number.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_readers
 * @addtogroup opendaq_reader Reader status
 * @{
 */

enum class ReadStatus : uint32_t
{
    Ok = 0,
    Event,
    Fail,
    Unknown = 0xFFFF
};

/*#
  * [templated(defaultAliasName: ReaderStatusPtr)]
  * [interfaceSmartPtr(IReaderStatus, GenericReaderStatusPtr)]
  */
/*!
 * @brief Represents the status of the reading process returned by the reader::read function.
 *
 * The `IReaderStatus` class provides information about the outcome of the reading operation,
 * including the validity of the reader and the potential encounter of event packets during processing.
 * Objects of this class are typically returned as a result of the `read` function of the Readers,
 * allowing the client code to assess and respond to the status of the reading process.
 */
DECLARE_OPENDAQ_INTERFACE(IReaderStatus, IBaseObject)
{
    /*!
     * @brief Retrieves the current reading status, indicating whether the reading process is in an "Ok" state,
     * has encountered an Event, has failed, or is in an Unknown state.
     * @param[out] status  a ReadStatus enum variable where the current reading status will be stored.
     */
    virtual ErrCode INTERFACE_FUNC getReadStatus(ReadStatus* status) = 0;

    /*!
     * @brief Retrieves the event packet from the reading process.
     * @param[out] packet The event packet from the reading process.
     */
    virtual ErrCode INTERFACE_FUNC getEventPacket(IEventPacket** packet) = 0;

    /*!
     * @brief Checks the validity of the reader.
     * @param[out] status Boolean value indicating the validity of the reader
     */
    virtual ErrCode INTERFACE_FUNC getValid(Bool* valid) = 0;

    /*!
     * @brief Retrieves the offset of the the read values
     * @param[out] offset The offset of the read values
     */
    virtual ErrCode INTERFACE_FUNC getOffset(INumber** offset) = 0;
};
/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY (
    LIBRARY_FACTORY, ReaderStatus,
    IEventPacket*, eventPacket,
    Bool, valid,
    INumber*, offset
)

END_NAMESPACE_OPENDAQ
