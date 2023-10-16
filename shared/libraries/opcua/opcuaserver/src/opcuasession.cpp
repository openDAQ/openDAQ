#include "opcuaserver/opcuasession.h"
#include <future>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

OpcUaSession::OpcUaSession(const OpcUaNodeId& sessionId, OpcUaServerLock* serverLock)
    : sessionId(sessionId)
    , serverLock(serverLock)
{
}

OpcUaSession::~OpcUaSession()
{
}

bool OpcUaSession::hasConfigurationControlLock() const
{
    return serverLock->hasConfigurationControlLock(getConfigurationLockTokenId());
}

void OpcUaSession::refuseConfigurationControlLock()
{
    serverLock->refuseConfigurationControlLock(getConfigurationLockTokenId());
}

bool OpcUaSession::lockConfigurationControl(const std::chrono::seconds timeout)
{
    return serverLock->lockConfigurationControl(getConfigurationLockTokenId(), timeout);
}

bool OpcUaSession::canControlAcq()
{
    return serverLock->canControlAcq(getConfigurationLockTokenId());
}

bool OpcUaSession::passwordLock(const std::string& password)
{
    return serverLock->passwordLock(password, getConfigurationLockTokenId());
}

bool OpcUaSession::passwordUnlock(const std::string& password)
{
    return serverLock->passwordUnlock(password, getConfigurationLockTokenId());
}

void OpcUaSession::setConfigurationLockTokenId(const OpcUaNodeId& configurationLockTokenId)
{
    this->configurationLockTokenId = configurationLockTokenId;
}

const OpcUaNodeId& OpcUaSession::getConfigurationLockTokenId() const
{
    if (!configurationLockTokenId.isNull())
        return configurationLockTokenId;

    return sessionId;
}

END_NAMESPACE_OPENDAQ_OPCUA
