#pragma once
/*
 * Copyright 2022-2024 openDAQ d.o.o.
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
#include <coretypes/string_ptr.h>
#include <coretypes/objectptr.h>
#include <fmt/format.h>

template <>
struct fmt::formatter<daq::StringPtr> : fmt::formatter<std::string_view>
{
    using Base = fmt::formatter<std::string_view>;

    template <typename FormatContext>
    auto format(const daq::StringPtr& c, FormatContext& ctx) const
    {
        if (!c.assigned())
        {
            return Base::format("<empty>", ctx);
        }

        return Base::format(c.toView(), ctx);
    }
};

template <typename T>
struct fmt::formatter<daq::ObjectPtr<T>> : fmt::formatter<std::string>
{
    template <typename FormatContext>
    auto format(const daq::ObjectPtr<T> c, FormatContext& ctx) const
    {
        if (!c.assigned())
        {
            return formatter<std::string>::format("<empty>", ctx);
        }

        return formatter<std::string>::format(c.template getValue<std::string>(""), ctx);
    }
};
