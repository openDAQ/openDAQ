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

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceSmartPtr(IRecorder, RecorderPtr)]
 */

/*!
 * @ingroup opendaq_function_blocks
 * @addtogroup opendaq_recorder Recorder
 * @{
 */

/*!
 * @brief Recorders represent objects which record input data to a persistent storage medium such
 * as a file, database, or cloud storage bucket. Recorders implement the `IRecorder` interface and
 * may expose additional properties for configuring the storage (such as file type, storage
 * location, etc.). tags.
 */
DECLARE_OPENDAQ_INTERFACE(IRecorder, IBaseObject)
{
    /*!
     * @brief Starts recording data from connected signals to the persistent storage medium.
     * @retval OPENDAQ_SUCCESS if the recording successfully started.
     * @retval OPENDAQ_ERR_INVALIDSTATE if the recording has already been started and the
     *     implementation chooses to treat this scenario as an error.
     */
    virtual ErrCode INTERFACE_FUNC startRecording() = 0;

    /*!
     * @brief Stops recording data from connected signals to the persistent storage medium.
     * @retval OPENDAQ_SUCCESS if the recording successfully stopped.
     * @retval OPENDAQ_ERR_INVALIDSTATE if the recording is not started and the implementation
     *     chooses to treat this scenario as an error.
     */
    virtual ErrCode INTERFACE_FUNC stopRecording() = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
