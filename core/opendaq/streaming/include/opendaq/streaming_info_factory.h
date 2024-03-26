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
#include <opendaq/streaming_info_config_ptr.h>
#include <opendaq/context_ptr.h>

BEGIN_NAMESPACE_OPENDAQ
/*!
 * @ingroup opendaq_streaming_info
 * @addtogroup opendaq_streaming_info_factories Factories
 * @{
 */

/*!
 * @brief Creates a StreamingInfoConfigPtr with a given protocol type ID string.
 * @param protocolId The unique streaming protocol type ID serves as the local ID of the created component.
 * @param context The Context. Most often the creating device passes its own Context to the StreamingInfo.
 * @param parent The parent component.
 */
inline StreamingInfoConfigPtr StreamingInfo(const StringPtr& protocolId)
{
    StreamingInfoConfigPtr obj(StreamingInfoConfig_Create(protocolId));
    return obj;
}
/*!@}*/
END_NAMESPACE_OPENDAQ
