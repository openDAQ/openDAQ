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

    typedef enum daqSampleType
    {
        daqSampleTypeInvalid = 0,
        daqSampleTypeUndefined = 0,
        daqSampleTypeFloat32,
        daqSampleTypeFloat64,
        daqSampleTypeUInt8,
        daqSampleTypeInt8,
        daqSampleTypeUInt16,
        daqSampleTypeInt16,
        daqSampleTypeUInt32,
        daqSampleTypeInt32,
        daqSampleTypeUInt64,
        daqSampleTypeInt64,
        daqSampleTypeRangeInt64,
        daqSampleTypeComplexFloat32,
        daqSampleTypeComplexFloat64,
        daqSampleTypeBinary,
        daqSampleTypeString,
        daqSampleTypeStruct,
        daqSampleTypeNull,
        daqSampleType_count
    } daqSampleType;

    typedef enum daqReadMode
    {
        daqReadModeUnscaled,
        daqReadModeScaled,
        daqReadModeRawValue
    } daqReadMode;

    typedef enum daqReadTimeoutType
    {
        daqReadTimeoutTypeAny, /*!< When some segments are available return them immediately.
                             *   When no segments are available return immediately when any arrive or time-out is exceeded.
                             */
        daqReadTimeoutTypeAll  /*!< Wait for the requested amount or until time-out is exceeded.*/
    } daqReadTimeoutType;

    typedef enum daqReadStatus
    {
        daqReadStatusOk = 0,
        daqReadStatusEvent,
        daqReadStatusFail,
        daqReadStatusUnknown = 0xFFFF
    } daqReadStatus;

#ifdef __cplusplus
}
#endif