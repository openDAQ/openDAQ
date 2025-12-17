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
#include <coretypes/baseobject.h>
#include <coretypes/stringobject.h>
#include <opendaq/streaming.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_streamings
 * @addtogroup opendaq_streaming_input_port Mirrored Input Port private
 * @{
 */

DECLARE_OPENDAQ_INTERFACE(IMirroredInputPortPrivate, IBaseObject)
{
    /*!
     * @brief Adds a streaming source for the input port.
     * @param streaming The streaming object representing a client-to-device streaming source.
     */
    virtual ErrCode INTERFACE_FUNC addStreamingSource(IStreaming* streaming) = 0;

    /*!
     * @brief Removes a streaming source from the input port.
     * @param streamingConnectionString The connection string of the streaming source to be removed.
     */
    virtual ErrCode INTERFACE_FUNC removeStreamingSource(IString* streamingConnectionString) = 0;

    /*!
     * @brief Gets the active streaming source of the input port as a streaming object.
     * @param[out] streaming The streaming object representing the active streaming source.
     */
    virtual ErrCode INTERFACE_FUNC getActiveStreamingSourceObject(IStreaming** streaming) = 0;

    // [templateType(objects, IStreaming)]
    /*!
     * @brief Gets a list of objects of all available streaming sources for the input port.
     * @param[out] objects The list of streaming objects representing all streaming sources.
     */
    virtual ErrCode INTERFACE_FUNC getStreamingSourceObjects(IList** objects) = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
