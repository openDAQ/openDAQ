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

    typedef enum DataRuleType
    {
        DataRuleTypeOther = 0,  ///< The rule is unknown to openDAQ and cannot be handled automatically.
        DataRuleTypeLinear,  ///< The parameters contain a `delta` and `start` parameters member. The value is calculated as: <em>inputValue
                             ///< * delta + start</em> .
        DataRuleTypeConstant,  ///< The value is a constant, as defined in the `constant` parameter field.
        DataRuleTypeExplicit   ///< The value is explicitly defined and is part of the signal's packet buffers.
    } DataRuleType;

    typedef enum DimensionRuleType
    {
        DimensionRuleTypeOther = 0,  ///< The rule is unknown to openDAQ and cannot be handled automatically.
        DimensionRuleTypeLinear,  ///< The parameters contain a `delta`, `start`, and `size` parameters member. Calculated as: <em>index *
                                  ///< delta + start</em> for `size` number of elements.
        DimensionRuleTypeLogarithmic,  ///< The parameters contain a `delta`, `start`, `base`, and `size` parameters member. Calculated as:
                                       ///< <em>base ^ (index * delta + start)</em> for `size` number of elements.
        DimensionRuleTypeList          ///< The parameters contain a `list` parameters member. The list contains all dimension labels.
    } DimensionRuleType;

    typedef enum PacketReadyNotification
    {
        PacketReadyNotificationNone,                   ///< Ignore the notification.
        PacketReadyNotificationSameThread,             ///< Call the listener in the same thread the notification was received.
        PacketReadyNotificationScheduler,              ///< Call the listener asynchronously or in another thread.
        PacketReadyNotificationSchedulerQueueWasEmpty  ///< Call the listener asynchronously or in another thread only if connection packet
                                                       ///< queue was empty
    } PacketReadyNotification;

    typedef enum PacketType
    {
        PacketTypeNone = 0,  ///< Undefined packet type
        PacketTypeData,      ///< Packet is a Data packet
        PacketTypeEvent      ///< Packet is an Event packet
    } PacketType;

    typedef enum TimeSource
    {
        TimeSourceUnknown,
        TimeSourceTai,
        TimeSourceGps,
        TimeSourceUtc
    } TimeSource;

    typedef enum UsesOffset
    {
        UsesOffsetUnknown,
        UsesOffsetTrue,
        UsesOffsetFalse
    } UsesOffset;

    typedef enum ScaledSampleType
    {
        ScaledSampleTypeInvalid = 0,
        ScaledSampleTypeFloat32,
        ScaledSampleTypeFloat64,
    } ScaledSampleType;

    typedef enum ScalingType
    {
        ScalingTypeOther = 0,
        ScalingTypeLinear  ///< The parameters contain a `scale` and `offset`. Calculated as: <em>inputValue * scale + offset</em> .
    } ScalingType;

#ifdef __cplusplus
}
#endif