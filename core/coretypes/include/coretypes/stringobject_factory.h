/*
 * Copyright 2022-2023 Blueberry d.o.o.
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
#include <coretypes/string_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

inline StringPtr String(ConstCharPtr str)
{
    StringPtr obj(String_Create(str));
    return obj;
}

inline StringPtr String(ConstCharPtr str, SizeT length)
{
    StringPtr obj(StringN_Create(str, length));
    return obj;
}

inline StringPtr String(const std::string& str)
{
    StringPtr obj(String_Create(str.c_str()));
    return obj;
}

inline StringPtr operator"" _daq(const char* str)
{
    return String(str);
}

inline StringPtr operator"" _daq(const char* str, std::size_t length)
{
    return String(str, length);
}

inline StringPtr operator+(const char lhs[], const StringPtr& rhs)
{
    return String(std::string(lhs) + rhs.toStdString());
}

inline StringPtr operator+(const std::string& lhs, const StringPtr& rhs)
{
    return String(lhs + rhs.toStdString());
}

END_NAMESPACE_OPENDAQ
