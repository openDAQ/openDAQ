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
#pragma once
#include <coretypes/string_ptr.h>
#include <string>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief Gets the prefix of a connection string.
 * @param connectionString The connection string
 *
 * The prefix is the part before `://`.
 * A connection string without `://` is its own prefix.
 */
inline StringPtr getPrefixFromConnectionString(const StringPtr& connectionString)
{
    const std::string str = connectionString;
    return str.substr(0, str.find("://"));
}

END_NAMESPACE_OPENDAQ
