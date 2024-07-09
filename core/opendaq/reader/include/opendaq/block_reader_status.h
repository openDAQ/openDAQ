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
#include <opendaq/reader_status.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_readers
 * @addtogroup opendaq_reader Block reader status
 * @{
 */

/*#
 * [interfaceSmartPtr(IReaderStatus, GenericReaderStatusPtr)]
 */

/*!
 * @brief IBlockReaderStatus inherits from IReaderStatus to expand information returned read function
 */
DECLARE_OPENDAQ_INTERFACE(IBlockReaderStatus, IReaderStatus)
{
    /*!
     * @brief Returns the number of samples that were read. 
     * Sometimes, during the process of reading, an event packet may occur that stops the reading of remaining samples. 
     * Developers can use this function to determine how many samples were actually read.
     * @param[out] samplesCount the amount of samples that were read.
     */
    virtual ErrCode INTERFACE_FUNC getReadSamples(SizeT* readSamples) = 0;
};
/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY (
    LIBRARY_FACTORY, BlockReaderStatus,
    IEventPacket*, eventPacket,
    Bool, valid,
    SizeT, readSamples
)

END_NAMESPACE_OPENDAQ