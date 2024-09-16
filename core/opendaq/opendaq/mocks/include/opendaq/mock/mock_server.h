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
#include <opendaq/server_impl.h>
#include <opendaq/server.h>
#include <opendaq/server_type_ptr.h>

class MockServerImpl : public daq::Server
{
public:
    explicit MockServerImpl(const daq::StringPtr& id, const daq::DevicePtr& rootDevice, const daq::ContextPtr& context);

protected:
};

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY, MockServer, daq::IServer,
    const daq::StringPtr&, id,
    const daq::DevicePtr&, rootDevice,
    const daq::ContextPtr&, context
)
