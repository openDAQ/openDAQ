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
#include "coretypes/coretypes.h"
#include "channel_buffer.h"
#include "channel_node.h"

BEGIN_NAMESPACE_DEWESOFT_RT_CORE

struct IChannelBuffer;

enum class ChannelType
{
    Synchronous,
    Asynchronous
};

/*#
 * [templated]
 * [decorated]
 * [templated(IChannelNode, IChannelBuffer)]
 * [propertyClass(ChannelNodeImpl, implTemplated: true)]
 * [propertyClassCtorArgs(const StringPtr& className, const StringPtr& nodeId)]
 * [includeHeader("channel_buffer_ptr.h")]
 * [interfaceSmartPtr(IChannelNode, ChannelNodePtr)]
 * [interfaceNamespace(IChannelNode, "Dewesoft::RT::Core::")]
 * [interfaceNamespace(IChannel, "Dewesoft::RT::Core::")]
 * [addProperty(IsControl, Bool, False)]
 */
DECLARE_RT_INTERFACE(IChannel, IChannelNode)
{
    DEFINE_INTFID("IChannel")

    // [property(value: false)]
    virtual ErrCode INTERFACE_FUNC getActive(Bool* value) const = 0;
    virtual ErrCode INTERFACE_FUNC setActive(Bool value) = 0;

    // [property(value: true)]
    virtual ErrCode INTERFACE_FUNC getUsed(Bool* value) const = 0;
    virtual ErrCode INTERFACE_FUNC setUsed(Bool value) = 0;

    virtual ErrCode INTERFACE_FUNC getUnit(IString** value) = 0;
    virtual ErrCode INTERFACE_FUNC setUnit(IString* value) = 0;

    // [property(value: 1)]
    virtual ErrCode INTERFACE_FUNC getScale(Float* value) const = 0;
    virtual ErrCode INTERFACE_FUNC setScale(Float value) = 0;

    // [property(value: 0)]
    virtual ErrCode INTERFACE_FUNC getOffset(Float* value) const = 0;
    virtual ErrCode INTERFACE_FUNC setOffset(Float value) = 0;

    // [property(value: -10)]
    virtual ErrCode INTERFACE_FUNC getMinValue(Float* value) const = 0;
    virtual ErrCode INTERFACE_FUNC setMinValue(Float value) = 0;

    // [property(value: 10)]
    virtual ErrCode INTERFACE_FUNC getMaxValue(Float* value) const = 0;
    virtual ErrCode INTERFACE_FUNC setMaxValue(Float value) = 0;

    // [property(Proc)]
    virtual ErrCode INTERFACE_FUNC getBuffer(IChannelBuffer** value) = 0;

    // [property(Proc)]
    virtual ErrCode INTERFACE_FUNC getSamplesAcquired(Int* value) const = 0;
    virtual ErrCode INTERFACE_FUNC incrementSamplePosition(SizeT newSamples) = 0;

    virtual ErrCode INTERFACE_FUNC getSampleRate(IRatio** value) const = 0;
    virtual ErrCode INTERFACE_FUNC setSampleRate(IRatio* value) = 0;

    virtual ErrCode INTERFACE_FUNC getChannelId(IString** value) = 0;

    virtual ErrCode INTERFACE_FUNC initialize() = 0;
    virtual ErrCode INTERFACE_FUNC initializeAcquisition() = 0;

    virtual ErrCode INTERFACE_FUNC startAcquisition() = 0;
    virtual ErrCode INTERFACE_FUNC stopAcquisition() = 0;

    virtual ErrCode INTERFACE_FUNC finalizeAcquisition() = 0;

    // [property(Proc)]
    virtual ErrCode INTERFACE_FUNC getCurrentSample(Float* sample) = 0;
    virtual ErrCode INTERFACE_FUNC getSampleAtPos(SizeT position, Float* sample) = 0;
    virtual ErrCode INTERFACE_FUNC getSampleAtRelativePos(Int relativePosition, Float* sample) = 0;

    virtual ErrCode INTERFACE_FUNC hasSamples(Bool* samples) = 0;

    virtual ErrCode INTERFACE_FUNC scaleValue(Float rawValue, Float* scaledValue) = 0;

    virtual ErrCode INTERFACE_FUNC getDescription(IString** description) = 0;
    virtual ErrCode INTERFACE_FUNC setDescription(IString* description) = 0;

    virtual ErrCode INTERFACE_FUNC getSampleDataType(SampleDataType* sampleDataType) = 0;

    virtual ErrCode INTERFACE_FUNC getDimension(Int* dimension) = 0;
    virtual ErrCode INTERFACE_FUNC getType(ChannelType* type) = 0;
};

END_NAMESPACE_DEWESOFT_RT_CORE
