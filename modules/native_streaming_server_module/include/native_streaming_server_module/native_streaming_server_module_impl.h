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
#include <native_streaming_server_module/common.h>
#include <opendaq/module_impl.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_SERVER_MODULE

class NativeStreamingServerModule final : public Module
{
public:
    explicit NativeStreamingServerModule(const ContextPtr & ctx);

    DictPtr<IString, IServerType> onGetAvailableServerTypes() override;
    ServerPtr onCreateServer(const StringPtr& serverType, const PropertyObjectPtr& serverConfig, const DevicePtr& rootDevice) override;

private:
    std::mutex sync;
};

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_SERVER_MODULE
