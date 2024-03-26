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
#include <coretypes/version.h>
#include <coretypes/stringobject_factory.h>

#include <fmt/format.h>

BEGIN_NAMESPACE_OPENDAQ

struct LibraryVersion
{
    uint32_t major;
    uint32_t minor;
    uint32_t patch;
};

using VersionCheckFunc = decltype(daqCoreTypesGetVersion);

// ReSharper disable once CppNonInlineFunctionDefinitionInHeaderFile
bool isCompatibleVersion(const std::string& dependency,
                         VersionCheckFunc getDependencyVersion,
                         const LibraryVersion& compiledVersion,
                         IString** errMsg)
{
    uint32_t linkedMajor{};
    uint32_t linkedMinor{};
    uint32_t linkedPatch{};

    getDependencyVersion(&linkedMajor, &linkedMinor, &linkedPatch);
    if (compiledVersion.major == linkedMajor)
    {
        return true;
    }

    if (errMsg != nullptr)
    {
        *errMsg = String(fmt::format("Incompatible {0} v{1}.{2}.{3} (expected compatible with v{4}.{5}.{6})",
                                     dependency,
                                     compiledVersion.major,
                                     compiledVersion.minor,
                                     compiledVersion.patch,
                                     linkedMajor,
                                     linkedMinor,
                                     linkedPatch))
                      .addRefAndReturn();
    }

    return false;
}

END_NAMESPACE_OPENDAQ
