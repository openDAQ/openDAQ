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
#include "opcuashared/opcua.h"
#include "opcuashared/opcuanodeid.h"
#include "opcuashared/opcuacommon.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA

class OpcUaServerLock
{
public:
    OpcUaServerLock();
    ~OpcUaServerLock();

    bool canControlAcq(const OpcUaNodeId& sessionId);

    bool passwordLock(const std::string& password, const OpcUaNodeId& sessionId = OpcUaNodeId());
    bool passwordUnlock(const std::string& password, const OpcUaNodeId& sessionId = OpcUaNodeId());
    bool isPasswordLocked();

    bool hasConfigurationControlLock(const OpcUaNodeId& sessionId) const;
    void refuseConfigurationControlLock(const OpcUaNodeId& sessionId);
    bool lockConfigurationControl(const OpcUaNodeId& sessionId, const std::chrono::seconds timeout);

    bool hasActiveConfigurationControlLock() const;

private:
    bool hasConfigurationControlAccess(const OpcUaNodeId& sessionId) const;
    bool canEditPasswordLock(const OpcUaNodeId& sessionId);

    std::string password;

    OpcUaNodeId configurationControlLockSessionId;
    utils::DurationTimeStamp configurationControlLockValidTo;
};

END_NAMESPACE_OPENDAQ_OPCUA
