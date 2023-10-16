#include "gtest/gtest.h"
#include <opcuaserver/opcuaserverlock.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

using OpcUaServerLockTest = testing::Test;

TEST_F(OpcUaServerLockTest, Create)
{
    OpcUaServerLock serverLock;
}

TEST_F(OpcUaServerLockTest, PasswordLock)
{
    OpcUaServerLock serverLock;
    ASSERT_TRUE(serverLock.passwordLock("test"));
}

TEST_F(OpcUaServerLockTest, PasswordLockTwoTimes)
{
    OpcUaServerLock serverLock;
    serverLock.passwordLock("test");
    ASSERT_FALSE(serverLock.passwordLock("test1"));
}

TEST_F(OpcUaServerLockTest, PasswordUnlock)
{
    OpcUaServerLock serverLock;
    serverLock.passwordLock("test");
    ASSERT_TRUE(serverLock.passwordUnlock("test"));
}

TEST_F(OpcUaServerLockTest, PasswordUnlockWrongPassword)
{
    OpcUaServerLock serverLock;
    serverLock.passwordLock("test");
    ASSERT_FALSE(serverLock.passwordUnlock("wrong password"));
}

TEST_F(OpcUaServerLockTest, PasswordUnlockWithoutLock)
{
    OpcUaServerLock serverLock;
    ASSERT_TRUE(serverLock.passwordUnlock("test3"));
}

TEST_F(OpcUaServerLockTest, IsPasswordLocked)
{
    OpcUaServerLock serverLock;
    ASSERT_FALSE(serverLock.isPasswordLocked());
}

TEST_F(OpcUaServerLockTest, IsPasswordLockedAfterLock)
{
    OpcUaServerLock serverLock;
    serverLock.passwordLock("test");
    ASSERT_TRUE(serverLock.isPasswordLocked());
}

TEST_F(OpcUaServerLockTest, IsPasswordLockedAfterUnlock)
{
    OpcUaServerLock serverLock;
    serverLock.passwordLock("test");
    serverLock.passwordUnlock("test");
    ASSERT_FALSE(serverLock.isPasswordLocked());
}

TEST_F(OpcUaServerLockTest, IsPasswordLockedAfterWrongPasswordUnlock)
{
    OpcUaServerLock serverLock;
    serverLock.passwordLock("test");
    serverLock.passwordUnlock("wrong password");
    ASSERT_TRUE(serverLock.isPasswordLocked());
}

TEST_F(OpcUaServerLockTest, LockConfigurationControl)
{
    OpcUaServerLock serverLock;
    OpcUaNodeId sessionId(1, 1);
    ASSERT_TRUE(serverLock.lockConfigurationControl(sessionId, std::chrono::seconds(10)));
}

TEST_F(OpcUaServerLockTest, LockConfigurationControlExtendLock)
{
    OpcUaServerLock serverLock;
    OpcUaNodeId sessionId(1, 1);
    serverLock.lockConfigurationControl(sessionId, std::chrono::seconds(10));
    ASSERT_TRUE(serverLock.lockConfigurationControl(sessionId, std::chrono::seconds(10)));
}

TEST_F(OpcUaServerLockTest, LockConfigurationControlRejectAccess)
{
    OpcUaServerLock serverLock;

    OpcUaNodeId sessionId1(1, 1);
    serverLock.lockConfigurationControl(sessionId1, std::chrono::seconds(10));

    OpcUaNodeId sessionId2(1, 2);
    ASSERT_FALSE(serverLock.lockConfigurationControl(sessionId2, std::chrono::seconds(10)));
}

TEST_F(OpcUaServerLockTest, LockConfigurationControlNewSessionAfterTimeout)
{
    OpcUaServerLock serverLock;

    OpcUaNodeId sessionId1(1, 1);
    serverLock.lockConfigurationControl(sessionId1, std::chrono::seconds(0));

    OpcUaNodeId sessionId2(1, 2);
    ASSERT_TRUE(serverLock.lockConfigurationControl(sessionId2, std::chrono::seconds(10)));
}

TEST_F(OpcUaServerLockTest, LockConfigurationControlAfterPasswordLock)
{
    OpcUaServerLock serverLock;
    serverLock.passwordLock("test");

    OpcUaNodeId sessionId(1, 1);
    ASSERT_TRUE(serverLock.lockConfigurationControl(sessionId, std::chrono::seconds(10)));
}

TEST_F(OpcUaServerLockTest, RefuseConfigurationControlLock)
{
    OpcUaServerLock serverLock;
    serverLock.passwordLock("test");

    OpcUaNodeId sessionId(1, 1);
    serverLock.lockConfigurationControl(sessionId, std::chrono::seconds(10));

    ASSERT_NO_THROW(serverLock.refuseConfigurationControlLock(sessionId));
}

TEST_F(OpcUaServerLockTest, SessionPasswordLock)
{
    OpcUaServerLock serverLock;

    OpcUaNodeId sessionId(1, 1);
    ASSERT_TRUE(serverLock.passwordLock("test", sessionId));
}

TEST_F(OpcUaServerLockTest, SessionPasswordLockConfigurationLock)
{
    OpcUaServerLock serverLock;

    OpcUaNodeId sessionId1(1, 1);
    serverLock.lockConfigurationControl(sessionId1, std::chrono::seconds(10));
    ASSERT_TRUE(serverLock.passwordLock("test", sessionId1));
}

TEST_F(OpcUaServerLockTest, SessionPasswordLockConfigurationLockByAnotherSession)
{
    OpcUaServerLock serverLock;

    OpcUaNodeId sessionId1(1, 1);
    serverLock.lockConfigurationControl(sessionId1, std::chrono::seconds(10));

    OpcUaNodeId sessionId2(1, 2);
    ASSERT_FALSE(serverLock.passwordLock("test", sessionId2));
}

TEST_F(OpcUaServerLockTest, SessionPasswordUnlock)
{
    OpcUaServerLock serverLock;

    OpcUaNodeId sessionId(1, 1);
    serverLock.passwordLock("test", sessionId);
    ASSERT_TRUE(serverLock.passwordUnlock("test", sessionId));
}

TEST_F(OpcUaServerLockTest, SessionPasswordUnlockConfigurationLock)
{
    OpcUaServerLock serverLock;

    OpcUaNodeId sessionId(1, 1);
    serverLock.lockConfigurationControl(sessionId, std::chrono::seconds(10));

    serverLock.passwordLock("test", sessionId);
    ASSERT_TRUE(serverLock.passwordUnlock("test", sessionId));
}

TEST_F(OpcUaServerLockTest, SessionPasswordUnlockConfigurationLockByAnotherSession)
{
    OpcUaServerLock serverLock;

    OpcUaNodeId sessionId1(1, 1);
    serverLock.lockConfigurationControl(sessionId1, std::chrono::seconds(10));
    serverLock.passwordLock("test", sessionId1);

    OpcUaNodeId sessionId2(1, 2);
    ASSERT_FALSE(serverLock.passwordUnlock("test", sessionId2));
}

TEST_F(OpcUaServerLockTest, RefuseConfigurationControlLockWithoutLock)
{
    OpcUaServerLock serverLock;
    serverLock.passwordLock("test");

    OpcUaNodeId sessionId(1, 1);
    ASSERT_NO_THROW(serverLock.refuseConfigurationControlLock(sessionId));
}

TEST_F(OpcUaServerLockTest, RefuseConfigurationControlLockLockedByAnotherSession)
{
    OpcUaServerLock serverLock;

    OpcUaNodeId sessionId1(1, 1);
    serverLock.lockConfigurationControl(sessionId1, std::chrono::seconds(10));

    OpcUaNodeId sessionId2(1, 2);
    ASSERT_NO_THROW(serverLock.refuseConfigurationControlLock(sessionId2));
}

TEST_F(OpcUaServerLockTest, HasConfigurationControlLock)
{
    OpcUaServerLock serverLock;
    OpcUaNodeId sessionId(1, 1);
    ASSERT_FALSE(serverLock.hasConfigurationControlLock(sessionId));
}

TEST_F(OpcUaServerLockTest, HasConfigurationControlLockAfterLock)
{
    OpcUaServerLock serverLock;
    OpcUaNodeId sessionId(1, 1);
    serverLock.lockConfigurationControl(sessionId, std::chrono::seconds(10));
    ASSERT_TRUE(serverLock.hasConfigurationControlLock(sessionId));
}

TEST_F(OpcUaServerLockTest, HasConfigurationControlLockAfterLockTimeout)
{
    OpcUaServerLock serverLock;
    OpcUaNodeId sessionId(1, 1);
    serverLock.lockConfigurationControl(sessionId, std::chrono::seconds(0)); //expires immediately
    ASSERT_FALSE(serverLock.hasConfigurationControlLock(sessionId));
}

TEST_F(OpcUaServerLockTest, HasConfigurationControlLockAfterAnotherSessionLock)
{
    OpcUaServerLock serverLock;
    OpcUaNodeId sessionId1(1, 1);
    serverLock.lockConfigurationControl(sessionId1, std::chrono::seconds(10));

    OpcUaNodeId sessionId2(1, 2);
    ASSERT_FALSE(serverLock.hasConfigurationControlLock(sessionId2));
}

TEST_F(OpcUaServerLockTest, HasActiveConfigurationControlLock)
{
    OpcUaServerLock serverLock;
    ASSERT_FALSE(serverLock.hasActiveConfigurationControlLock());
}

TEST_F(OpcUaServerLockTest, HasActiveConfigurationControlLockAfterLock)
{
    OpcUaServerLock serverLock;
    OpcUaNodeId sessionId(1, 1);
    serverLock.lockConfigurationControl(sessionId, std::chrono::seconds(10));

    ASSERT_TRUE(serverLock.hasActiveConfigurationControlLock());
}

TEST_F(OpcUaServerLockTest, HasActiveConfigurationControlLockAfterTimeout)
{
    OpcUaServerLock serverLock;
    OpcUaNodeId sessionId(1, 1);
    serverLock.lockConfigurationControl(sessionId, std::chrono::seconds(0));

    ASSERT_FALSE(serverLock.hasActiveConfigurationControlLock());
}

TEST_F(OpcUaServerLockTest, HasActiveConfigurationControlLockAfterRefuseLock)
{
    OpcUaServerLock serverLock;
    OpcUaNodeId sessionId(1, 1);
    serverLock.lockConfigurationControl(sessionId, std::chrono::seconds(10));
    serverLock.refuseConfigurationControlLock(sessionId);

    ASSERT_FALSE(serverLock.hasActiveConfigurationControlLock());
}

TEST_F(OpcUaServerLockTest, CanControlAcq)
{
    OpcUaServerLock serverLock;
    OpcUaNodeId sessionId(1, 1);
    ASSERT_TRUE(serverLock.canControlAcq(sessionId));
}

TEST_F(OpcUaServerLockTest, CanControlAcqPasswordLock)
{
    OpcUaServerLock serverLock;
    serverLock.passwordLock("Test");
    OpcUaNodeId sessionId(1, 1);
    ASSERT_FALSE(serverLock.canControlAcq(sessionId));
}

TEST_F(OpcUaServerLockTest, CanControlAcqPasswordUnlock)
{
    OpcUaServerLock serverLock;
    serverLock.passwordLock("Test");
    serverLock.passwordUnlock("Test");
    OpcUaNodeId sessionId(1, 1);
    ASSERT_TRUE(serverLock.canControlAcq(sessionId));
}

TEST_F(OpcUaServerLockTest, CanControlAcqConfigurationControlLock)
{
    OpcUaServerLock serverLock;
    OpcUaNodeId sessionId(1, 1);

    serverLock.lockConfigurationControl(sessionId, std::chrono::seconds(10));
    ASSERT_TRUE(serverLock.canControlAcq(sessionId));
}

TEST_F(OpcUaServerLockTest, CanControlAcqConfigurationControlLockByOtherSession)
{
    OpcUaServerLock serverLock;
    OpcUaNodeId sessionId1(1, 1);
    serverLock.lockConfigurationControl(sessionId1, std::chrono::seconds(10));

    OpcUaNodeId sessionId2(1, 2);
    ASSERT_FALSE(serverLock.canControlAcq(sessionId2));
}

TEST_F(OpcUaServerLockTest, CanControlAcqConfigurationControlLockByOtherSessionAfterTimeout)
{
    OpcUaServerLock serverLock;
    OpcUaNodeId sessionId1(1, 1);
    serverLock.lockConfigurationControl(sessionId1, std::chrono::seconds(0));

    OpcUaNodeId sessionId2(1, 2);
    ASSERT_TRUE(serverLock.canControlAcq(sessionId2));
}

END_NAMESPACE_OPENDAQ_OPCUA
