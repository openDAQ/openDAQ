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
#include <cmath>
#include <cstddef>
#include <cstdint>

#include <opendaq/data_descriptor_factory.h>
#include <opendaq/data_descriptor_ptr.h>
#include <opendaq/sample_type.h>
#include <ref_fb_module/common.h>

BEGIN_NAMESPACE_REF_FB_MODULE

namespace Renderer
{

/// @brief True for a descriptor parameter that stands for "no descriptor". A signal without a domain signal
/// announces a stub descriptor with a Null sample type rather than omitting the event packet parameter.
inline bool isNullDescriptor(const DataDescriptorPtr& descriptor)
{
    return !descriptor.assigned() || descriptor == NullDataDescriptor();
}

/// @brief True for a descriptor whose samples are vectors rather than single values.
inline bool isVectorDescriptor(const DataDescriptorPtr& descriptor)
{
    return descriptor.assigned() && descriptor.getDimensions().assigned() && descriptor.getDimensions().getCount() == 1;
}

/// @brief Value range spanning the constants, for signals whose descriptor carries no value range of its own.
/// Values that are all the same span nothing, so they are given room on either side to be scaled into.
inline void constantValueRange(double low, double high, double& min, double& max)
{
    if (low != high)
    {
        min = low;
        max = high;
        return;
    }

    const double margin = low == 0.0 ? 1.0 : std::fabs(low) * 0.1;
    min = low - margin;
    max = high + margin;
}

/// @brief Value range around a single constant.
inline void constantValueRange(double value, double& min, double& max)
{
    constantValueRange(value, value, min, max);
}

/// @brief One element of a sample as a double, whatever the sample type holds it as.
inline double elementValue(SampleType sampleType, const void* data, std::size_t index)
{
    switch (sampleType)
    {
        case SampleType::Float32:
            return static_cast<const float*>(data)[index];
        case SampleType::Float64:
            return static_cast<const double*>(data)[index];
        case SampleType::UInt8:
            return static_cast<const uint8_t*>(data)[index];
        case SampleType::Int8:
            return static_cast<const int8_t*>(data)[index];
        case SampleType::UInt16:
            return static_cast<const uint16_t*>(data)[index];
        case SampleType::Int16:
            return static_cast<const int16_t*>(data)[index];
        case SampleType::UInt32:
            return static_cast<const uint32_t*>(data)[index];
        case SampleType::Int32:
            return static_cast<const int32_t*>(data)[index];
        case SampleType::UInt64:
            return static_cast<double>(static_cast<const uint64_t*>(data)[index]);
        case SampleType::Int64:
            return static_cast<double>(static_cast<const int64_t*>(data)[index]);
        default:
            return 0.0;
    }
}

/// @brief Height of one plot row once the value strips have taken their share of the available height.
inline float plotItemHeight(float availableHeight, std::size_t plotCount, std::size_t stripCount, float stripHeight)
{
    if (plotCount == 0)
        return 0.0f;

    const float remaining = availableHeight - static_cast<float>(stripCount) * stripHeight;
    if (remaining < 0.0f)
        return 0.0f;

    return remaining / static_cast<float>(plotCount);
}
}

END_NAMESPACE_REF_FB_MODULE
