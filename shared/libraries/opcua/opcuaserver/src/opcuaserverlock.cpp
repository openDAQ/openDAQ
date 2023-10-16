#include "opcuaserver/opcuaserverlock.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA

OpcUaServerLock::OpcUaServerLock()
{
    configurationControlLockSessionId = OpcUaNodeId();
    refuseConfigurationControlLock(configurationControlLockSessionId);
}

OpcUaServerLock::~OpcUaServerLock()
{
}

bool OpcUaServerLock::passwordLock(const std::string& password, const OpcUaNodeId& sessionId)
{
    if (!canEditPasswordLock(sessionId))
        return false;

    if (isPasswordLocked())
        return false;

    this->password = password;
    return true;
}

bool OpcUaServerLock::passwordUnlock(const std::string& password, const OpcUaNodeId& sessionId)
{
    if (!canEditPasswordLock(sessionId))
        return false;

    if (isPasswordLocked() && this->password != password)
        return false;

    this->password = "";
    return true;
}

bool OpcUaServerLock::isPasswordLocked()
{
    return password != "";
}

bool OpcUaServerLock::hasConfigurationControlLock(const OpcUaNodeId& sessionId) const
{
    return sessionId == configurationControlLockSessionId && hasActiveConfigurationControlLock();
}

void OpcUaServerLock::refuseConfigurationControlLock(const OpcUaNodeId& sessionId)
{
    if (sessionId == configurationControlLockSessionId)
        lockConfigurationControl(sessionId, std::chrono::seconds(-1));
}

bool OpcUaServerLock::lockConfigurationControl(const OpcUaNodeId& sessionId, const std::chrono::seconds timeout)
{
    if (hasConfigurationControlAccess(sessionId))
    {
        configurationControlLockSessionId = sessionId;
        configurationControlLockValidTo = utils::GetDurationTimeStamp() + timeout;
        return true;
    }
    return false;
}

bool OpcUaServerLock::hasConfigurationControlAccess(const OpcUaNodeId& sessionId) const
{
    return hasConfigurationControlLock(sessionId) || !hasActiveConfigurationControlLock();
}

bool OpcUaServerLock::hasActiveConfigurationControlLock() const
{
    return utils::GetDurationTimeStamp() < configurationControlLockValidTo;
}

bool OpcUaServerLock::canControlAcq(const OpcUaNodeId& sessionId)
{
    return !isPasswordLocked() && hasConfigurationControlAccess(sessionId);
}

bool OpcUaServerLock::canEditPasswordLock(const OpcUaNodeId& sessionId)
{
    if (this->hasConfigurationControlLock(sessionId))
        return true;

    return !hasActiveConfigurationControlLock();
}



END_NAMESPACE_OPENDAQ_OPCUA
