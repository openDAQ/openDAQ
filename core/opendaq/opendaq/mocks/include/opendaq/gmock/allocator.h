/*
 * Copyright 2022-2024 openDAQ d. o. o.
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
#include <opendaq/allocator_ptr.h>
#include <coretypes/intfs.h>
#include <gmock/gmock.h>
#include <coretypes/gmock/mock_ptr.h>

#include <cstdlib>

struct MockAllocator : daq::ImplementationOf<daq::IAllocator>
{
    typedef MockPtr<
        daq::IAllocator,
        daq::AllocatorPtr,
        MockAllocator
    > Strict;

    MOCK_METHOD(daq::ErrCode, allocate,
        (
            const daq::IDataDescriptor* descriptor,
            daq::SizeT bytes,
            daq::SizeT align,
            daq::VoidPtr* address
        ),
        (override MOCK_CALL));

    MOCK_METHOD(daq::ErrCode, free, (daq::VoidPtr address), (override MOCK_CALL));

    MockAllocator()
    {
        using namespace testing;

        ON_CALL(*this, allocate)
            .WillByDefault(DoAll(
                Invoke([&](const daq::IDataDescriptor *descriptor, daq::SizeT bytes, daq::SizeT align, daq::VoidPtr* address) {
                    *address = std::malloc(bytes);
                }),
                Return(OPENDAQ_SUCCESS)
            ));

        ON_CALL(*this, free)
            .WillByDefault(DoAll(
                Invoke([&](daq::VoidPtr address) {
                    std::free(address);
                }),
                Return(OPENDAQ_SUCCESS)
            ));
    }
};
