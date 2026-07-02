/*
 * Copyright 2022-2026 openDAQ d.o.o.
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

template <typename Enum>
class EnumFlags
{
    static_assert(std::is_enum_v<Enum>, "EnumFlags requires an enum type");

public:
    using Underlying = std::underlying_type_t<Enum>;

    constexpr EnumFlags() = default;

    constexpr EnumFlags(Enum value)
        : value(static_cast<Underlying>(value))
    {
    }

    constexpr bool empty() const
    {
        return value == 0;
    }

    constexpr bool contains(Enum flag) const
    {
        return (value & static_cast<Underlying>(flag)) != 0;
    }

    constexpr void add(Enum flag)
    {
        value |= static_cast<Underlying>(flag);
    }

    constexpr void remove(Enum flag)
    {
        value &= ~static_cast<Underlying>(flag);
    }

    constexpr void set(Enum flag, bool active)
    {
        if (active)
            add(flag);
        else
            remove(flag);
    }

    constexpr void clear()
    {
        value = 0;
    }

    constexpr Underlying raw() const
    {
        return value;
    }

private:
    Underlying value = 0;
};
