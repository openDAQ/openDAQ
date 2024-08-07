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
#include <coretypes/intfs.h>
#include <opendaq/server.h>
#include <coretypes/stringobject.h>

class MockServerImpl : public daq::ImplementationOf<daq::IServer>
{
public:
    explicit MockServerImpl();

    daq::ErrCode INTERFACE_FUNC enableDiscovery() override;
    daq::ErrCode INTERFACE_FUNC stop() override;
    daq::ErrCode INTERFACE_FUNC getId(daq::IString** serverId) override;
};

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(INTERNAL_FACTORY, MockServer, daq::IServer)
