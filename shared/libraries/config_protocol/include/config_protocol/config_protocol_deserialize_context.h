/*
 * Copyright 2022-2024 Blueberry d.o.o.
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
#include <opendaq/component_deserialize_context.h>
#include <config_protocol/config_protocol_client.h>

namespace daq::config_protocol
{

DECLARE_OPENDAQ_INTERFACE(IConfigProtocolDeserializeContext, IComponentDeserializeContext)
{
    virtual ConfigProtocolClientCommPtr getClientComm() = 0;
    virtual std::string getRemoteGlobalId() = 0;
    virtual void setRemoteGlobalId(const std::string& remoteGlobalId) = 0;
    virtual TypeManagerPtr getTypeManager() = 0;
};

}
