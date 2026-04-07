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
#include <coretypes/common.h>
#include <coretypes/stringobject.h>

extern "C"
void PUBLIC_EXPORT daqOpenDaqGetVersion(unsigned int* major, unsigned int* minor, unsigned int* revision);

extern "C"
daq::ErrCode PUBLIC_EXPORT daqCoreValidateVersionMetadata(
    unsigned int major,
    unsigned int minor,
    unsigned int patch,
    daq::IString* branch,
    daq::IString* sha,
    [[maybe_unused]] daq::IString* fork,
    daq::IString** warningMessage);

extern "C"
daq::ErrCode PUBLIC_EXPORT getSdkCoreVersionMetadata(unsigned int* major, unsigned int* minor, unsigned int* patch, daq::IString** branch, daq::IString** sha, daq::IString** fork);
