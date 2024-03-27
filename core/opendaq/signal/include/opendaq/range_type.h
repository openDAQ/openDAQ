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

#pragma pack(push, 1)

BEGIN_NAMESPACE_OPENDAQ

template <typename T>
struct RangeType
{
    using Type = T;

    static const constexpr T ImplicitNext = static_cast<T>(-1);

    RangeType()
        : start(static_cast<T>(0))
        , end(static_cast<T>(0))
    {
    }

    RangeType(T begin, T end)
        : start(begin)
        , end(end)
    {
    }

    RangeType(T begin)
        : RangeType(begin, ImplicitNext)
    {
    }

    friend bool operator==(const RangeType<T>& lhs, const RangeType<T>& rhs)
    {
        return (lhs.start == rhs.start) && (lhs.end == rhs.end);
    }

    friend bool operator!=(const RangeType<T>& lhs, const RangeType<T>& rhs)
    {
        return (lhs.start != rhs.start) || (lhs.end != rhs.end);
    }

    friend RangeType<T> operator+(const RangeType<T>& range, std::int64_t offset)
    {
        T start = range.start + offset;
        T end = -1;
        if (range.end != -1)
        {
            end = range.end + offset;
        }

        return RangeType<T>(start, end);
    }

    friend RangeType<T> operator-(const RangeType<T>& range, std::int64_t offset)
    {
        T start = range.start - offset;
        T end = -1;
        if (range.end != -1)
        {
            end = range.end - offset;
        }

        return RangeType<T>(start, end);
    }

    T start;
    T end;
};

using RangeType64 = RangeType<int64_t>;

static_assert(std::is_standard_layout_v<RangeType64>, "RangeType64 is not standard layout");
static_assert(std::is_trivially_copyable_v<RangeType64>, "RangeType64 is not trivially copyable");

#pragma pack(pop)

END_NAMESPACE_OPENDAQ
