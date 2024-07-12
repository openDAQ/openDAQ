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

#include <config_protocol/config_protocol_client.h>

namespace daq::config_protocol
{

class ConfigClientObjectImpl
{
public:
    ConfigClientObjectImpl(ConfigProtocolClientCommPtr clientComm, std::string remoteGlobalId);
    virtual ~ConfigClientObjectImpl() = default;

protected:
    ConfigProtocolClientCommPtr clientComm;
    std::string remoteGlobalId;
};

}
