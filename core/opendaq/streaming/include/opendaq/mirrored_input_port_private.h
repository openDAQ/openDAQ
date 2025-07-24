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
#include <opendaq/streaming_to_device.h>

BEGIN_NAMESPACE_OPENDAQ

DECLARE_OPENDAQ_INTERFACE(IMirroredInputPortPrivate, IBaseObject)
{
    virtual ErrCode INTERFACE_FUNC addStreamingSource(IStreamingToDevice* streaming) = 0;
    virtual ErrCode INTERFACE_FUNC removeStreamingSource(IString* streamingConnectionString) = 0;

    virtual ErrCode INTERFACE_FUNC getActiveStreamingSourceObject(IStreamingToDevice** streaming) = 0;

    // [templateType(objects, IStreamingToDevice)]
    virtual ErrCode INTERFACE_FUNC getStreamingSourceObjects(IList** objects) = 0;
};

END_NAMESPACE_OPENDAQ
