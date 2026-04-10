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

#include <fmt/format.h>
#include <opendaq/opendaq_config.h>
#include <opendaq/version.h>
#include <coretypes/validation.h>
#include <coretypes/stringobject_factory.h>

BEGIN_NAMESPACE_OPENDAQ

inline daq::ErrCode checkModuleCoreVersionMetadata(
    unsigned int major,
    unsigned int minor,
    unsigned int patch,
    daq::IString* branch,
    daq::IString* sha,
    [[maybe_unused]] daq::IString* fork,
    daq::IString** logMessage)
{
    OPENDAQ_PARAM_NOT_NULL(logMessage);
    OPENDAQ_PARAM_NOT_NULL(branch);
    OPENDAQ_PARAM_NOT_NULL(sha);

    daq::ErrCode errCode = OPENDAQ_SUCCESS;

    auto runningSdkMetadataAsString =
        fmt::format(R"([[ 'major': '{}'; 'minor': '{}'; 'patch': '{}'; 'branch': '{}'; 'sha': '{}'; ]])",
                    OPENDAQ_OPENDAQ_MAJOR_VERSION,
                    OPENDAQ_OPENDAQ_MINOR_VERSION,
                    OPENDAQ_OPENDAQ_PATCH_VERSION,
                    OPENDAQ_OPENDAQ_BRANCH_NAME,
                    OPENDAQ_OPENDAQ_REVISION_HASH);
    std::string inModuleMetadataAsString =
        fmt::format(R"([[ 'major': '{}'; 'minor': '{}'; 'patch': '{}'; 'branch': '{}'; 'sha': '{}'; ]])",
                    major,
                    minor,
                    patch,
                    daq::StringPtr::Borrow(branch).toStdString(),
                    daq::StringPtr::Borrow(sha).toStdString());

    auto message = fmt::format(R"("The core libraries version metadata of loading module "{}".)", inModuleMetadataAsString);

    if (major != OPENDAQ_OPENDAQ_MAJOR_VERSION)
    {
        return DAQ_MAKE_ERROR_INFO(
            OPENDAQ_ERR_NO_COMPATIBLE_VERSION,
            fmt::format(R"(The running version of openDAQ is: "{}"; the core SDK libraries version has been used to build module is incompatible: "{}" - the major number mismatches.)",
                        runningSdkMetadataAsString,
                        inModuleMetadataAsString
            )
        );
    }

    if (minor != OPENDAQ_OPENDAQ_MINOR_VERSION)
    {
        return DAQ_MAKE_ERROR_INFO(
            OPENDAQ_ERR_NO_COMPATIBLE_VERSION,
            fmt::format(R"(The running version of openDAQ is: "{}"; the core SDK libraries version has been used to build module is incompatible: "{}" - the minor number mismatches.)",
                        runningSdkMetadataAsString,
                        inModuleMetadataAsString
            )
        );
    }

    if (patch != OPENDAQ_OPENDAQ_PATCH_VERSION)
    {
        message +=
            fmt::format("\nThe running version of openDAQ is: \"{}\";\nthe core SDK libraries version has been used to build module differs: \"{}\" - the patch number mismatches.",
                        runningSdkMetadataAsString,
                        inModuleMetadataAsString
            );
        errCode = OPENDAQ_PARTIAL_SUCCESS;
    }

    if (daq::StringPtr::Borrow(branch) != OPENDAQ_OPENDAQ_BRANCH_NAME)
    {
        message +=
            fmt::format("\nThe running version of openDAQ is: \"{}\";\nthe core SDK libraries version has been used to build module differs: \"{}\" - the git branch name mismatches.",
                        runningSdkMetadataAsString,
                        inModuleMetadataAsString
            );
        errCode = OPENDAQ_PARTIAL_SUCCESS;
    }

    if (daq::StringPtr::Borrow(sha) != OPENDAQ_OPENDAQ_REVISION_HASH)
    {
        message +=
            fmt::format("\nThe running version of openDAQ is: \"{}\";\nthe core SDK libraries version has been used to build module differs: \"{}\" - the git commit sha mismatches.",
                        runningSdkMetadataAsString,
                        inModuleMetadataAsString
            );
        errCode = OPENDAQ_PARTIAL_SUCCESS;
    }

    *logMessage = daq::String(message).detach();
    return errCode;
}

END_NAMESPACE_OPENDAQ
