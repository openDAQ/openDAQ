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

#pragma once
#include <coretypes/version_info.h>
#include <coretypes/intfs.h>

BEGIN_NAMESPACE_OPENDAQ

class VersionInfoImpl : public ImplementationOf<IVersionInfo>
{
public:
    VersionInfoImpl(SizeT major, SizeT minor, SizeT patch);

    ErrCode INTERFACE_FUNC getMajor(SizeT* major) override;
    ErrCode INTERFACE_FUNC getMinor(SizeT* minor) override;
    ErrCode INTERFACE_FUNC getPatch(SizeT* patch) override;

private:
    SizeT major;
    SizeT minor;
    SizeT patch;
};

END_NAMESPACE_OPENDAQ
