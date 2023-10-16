/*
 * Copyright 2022-2023 Blueberry d.o.o.
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
#include <map>
#include <atomic>
#include "opcuashared/opcuanodeid.h"
#include "opcuaserver/opcuaserverlock.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA

class OpcUaSession;
using OpcUaSessionPtr = std::shared_ptr<OpcUaSession>;

class OpcUaSession
{
public:
    explicit OpcUaSession(const OpcUaNodeId& sessionId, OpcUaServerLock* serverLock);
    ~OpcUaSession();

    bool canControlAcq();

    bool hasConfigurationControlLock() const;
    bool lockConfigurationControl(std::chrono::seconds timeout);
    void refuseConfigurationControlLock();

    bool passwordLock(const std::string& password);
    bool passwordUnlock(const std::string& password);

    void setConfigurationLockTokenId(const OpcUaNodeId& configurationLockTokenId);
    const OpcUaNodeId& getConfigurationLockTokenId() const;

private:
    OpcUaNodeId sessionId;
    OpcUaServerLock* serverLock;
    OpcUaNodeId configurationLockTokenId;
};

END_NAMESPACE_OPENDAQ_OPCUA
