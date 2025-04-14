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

#ifdef __cplusplus
extern "C"
{
#endif

    enum SampleType
    {
        SampleTypeInvalid = 0,
        SampleTypeUndefined = 0,
        SampleTypeFloat32,
        SampleTypeFloat64,
        SampleTypeUInt8,
        SampleTypeInt8,
        SampleTypeUInt16,
        SampleTypeInt16,
        SampleTypeUInt32,
        SampleTypeInt32,
        SampleTypeUInt64,
        SampleTypeInt64,
        SampleTypeRangeInt64,
        SampleTypeComplexFloat32,
        SampleTypeComplexFloat64,
        SampleTypeBinary,
        SampleTypeString,
        SampleTypeStruct,
        SampleTypeNull,
        SampleType_count
    };

    enum ReadMode
    {
        ReadModeUnscaled,
        ReadModeScaled,
        ReadModeRawValue
    };

    enum ReadTimeoutType
    {
        ReadTimeoutTypeAny, /*!< When some segments are available return them immediately.
                             *   When no segments are available return immediately when any arrive or time-out is exceeded.
                             */
        ReadTimeoutTypeAll  /*!< Wait for the requested amount or until time-out is exceeded.*/
    };

    enum ReadStatus
    {
        ReadStatusOk = 0,
        ReadStatusEvent,
        ReadStatusFail,
        ReadStatusUnknown = 0xFFFF
    };

#ifdef __cplusplus
}
#endif