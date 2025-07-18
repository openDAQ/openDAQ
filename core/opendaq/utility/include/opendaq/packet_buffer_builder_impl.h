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
#include <opendaq/packet_buffer_builder.h>
#include <opendaq/context_ptr.h>
#include <opendaq/data_descriptor_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class PacketBufferBuilderImpl : public ImplementationOf<IPacketBufferBuilder>
{
public:

    PacketBufferBuilderImpl();

    ErrCode INTERFACE_FUNC getContext(IContext** context) override;
    ErrCode INTERFACE_FUNC setContext(IContext* context) override;

    ErrCode INTERFACE_FUNC getSizeInBytes(SizeT* sizeInBytes) override;
    ErrCode INTERFACE_FUNC setSizeInBytes(SizeT sizeInBytes) override;

    ErrCode INTERFACE_FUNC build(IPacketBuffer** buffer) override;

private:

    SizeT sizeInBytes;
    ContextPtr context;
};

END_NAMESPACE_OPENDAQ
