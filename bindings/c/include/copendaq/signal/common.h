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

    typedef enum daqDataRuleType
    {
        daqDataRuleTypeOther = 0,  ///< The rule is unknown to openDAQ and cannot be handled automatically.
        daqDataRuleTypeLinear,  ///< The parameters contain a `delta` and `start` parameters member. The value is calculated as: <em>inputValue
                             ///< * delta + start</em> .
        daqDataRuleTypeConstant,  ///< The value is a constant, as defined in the `constant` parameter field.
        daqDataRuleTypeExplicit   ///< The value is explicitly defined and is part of the signal's packet buffers.
    } daqDataRuleType;

    typedef enum daqDimensionRuleType
    {
        daqDimensionRuleTypeOther = 0,  ///< The rule is unknown to openDAQ and cannot be handled automatically.
        daqDimensionRuleTypeLinear,  ///< The parameters contain a `delta`, `start`, and `size` parameters member. Calculated as: <em>index *
                                  ///< delta + start</em> for `size` number of elements.
        daqDimensionRuleTypeLogarithmic,  ///< The parameters contain a `delta`, `start`, `base`, and `size` parameters member. Calculated as:
                                       ///< <em>base ^ (index * delta + start)</em> for `size` number of elements.
        daqDimensionRuleTypeList          ///< The parameters contain a `list` parameters member. The list contains all dimension labels.
    } daqDimensionRuleType;

    typedef enum daqPacketReadyNotification
    {
        daqPacketReadyNotificationNone,                   ///< Ignore the notification.
        daqPacketReadyNotificationSameThread,             ///< Call the listener in the same thread the notification was received.
        daqPacketReadyNotificationScheduler,              ///< Call the listener asynchronously or in another thread.
        daqPacketReadyNotificationSchedulerQueueWasEmpty  ///< Call the listener asynchronously or in another thread only if connection packet
                                                       ///< queue was empty
    } daqPacketReadyNotification;

    typedef enum daqPacketType
    {
        daqPacketTypeNone = 0,  ///< Undefined packet type
        daqPacketTypeData,      ///< Packet is a Data packet
        daqPacketTypeEvent      ///< Packet is an Event packet
    } daqPacketType;

    typedef enum daqTimeProtocol
    {
        daqTimeProtocolUnknown,
        daqTimeProtocolTai,
        daqTimeProtocolGps,
        daqTimeProtocolUtc
    } daqTimeProtocol;

    typedef enum daqUsesOffset
    {
        daqUsesOffsetUnknown,
        daqUsesOffsetTrue,
        daqUsesOffsetFalse
    } daqUsesOffset;

    typedef enum daqScaledSampleType
    {
        daqScaledSampleTypeInvalid = 0,
        daqScaledSampleTypeFloat32,
        daqScaledSampleTypeFloat64,
    } daqScaledSampleType;

    typedef enum daqScalingType
    {
        daqScalingTypeOther = 0,
        daqScalingTypeLinear  ///< The parameters contain a `scale` and `offset`. Calculated as: <em>inputValue * scale + offset</em> .
    } daqScalingType;

#ifdef __cplusplus
}
#endif
