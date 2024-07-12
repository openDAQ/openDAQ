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
#include <opendaq/allocator.h>
#include <opendaq/signal_descriptor.h>
#include <coretypes/common.h>
#include <coretypes/intfs.h>

BEGIN_NAMESPACE_OPENDAQ

class MiMallocAllocatorImpl : public ImplementationOf<IAllocator>
{
public:
    ErrCode INTERFACE_FUNC allocate(
        const ISignalDescriptor *descriptor,
        daq::SizeT bytes,
        daq::SizeT align,
        VoidPtr* address) override;

    ErrCode INTERFACE_FUNC free(VoidPtr address) override;
};

END_NAMESPACE_OPENDAQ
