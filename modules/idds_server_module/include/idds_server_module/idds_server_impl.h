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
#include <idds_server_module/common.h>
#include <opendaq/device_ptr.h>
#include <opendaq/server.h>
#include <opendaq/server_impl.h>
#include <coretypes/intfs.h>
#include <idds/idds_server.h>

BEGIN_NAMESPACE_OPENDAQ_IDDS_SERVER_MODULE

class iDDSServerImpl : public daq::Server
{
public:
    explicit iDDSServerImpl(daq::DevicePtr rootDevice, const ContextPtr& context);
    static ServerTypePtr createType();

protected:
    void onStopServer() override;

    daq::idds::iDDSServer iDDSServer;
};

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY, iDDSServer, daq::IServer,
    DevicePtr, rootDevice,
    const ContextPtr&, context
)

END_NAMESPACE_OPENDAQ_IDDS_SERVER_MODULE
