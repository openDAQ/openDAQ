#include "gtest/gtest.h"
#include <opcuaserver/opcuaserverlock.h>
#include <opcuaserver/opcuasession.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA


using OpcUaSessionTest = testing::Test;

TEST_F(OpcUaSessionTest, Create)
{
    OpcUaServerLock serverLock;
    OpcUaSession session(OpcUaNodeId(1, 1), &serverLock);
}

TEST_F(OpcUaSessionTest, LockConfigurationControl)
{
    OpcUaServerLock serverLock;
    OpcUaSession session(OpcUaNodeId(1, 1), &serverLock);
    ASSERT_TRUE(session.lockConfigurationControl(std::chrono::seconds(10)));
}

TEST_F(OpcUaSessionTest, LockConfigurationControlExtendLock)
{
    OpcUaServerLock serverLock;
    OpcUaSession session(OpcUaNodeId(1, 1), &serverLock);
    session.lockConfigurationControl(std::chrono::seconds(10));
    ASSERT_TRUE(session.lockConfigurationControl(std::chrono::seconds(10)));
}

TEST_F(OpcUaSessionTest, LockConfigurationControlRejectAccess)
{
    OpcUaServerLock serverLock;

    OpcUaSession session1(OpcUaNodeId(1, 1), &serverLock);
    session1.lockConfigurationControl(std::chrono::seconds(10));

    OpcUaSession session2(OpcUaNodeId(1, 2), &serverLock);
    ASSERT_FALSE(session2.lockConfigurationControl(std::chrono::seconds(10)));
}

TEST_F(OpcUaSessionTest, LockConfigurationControlNewSessionAfterTimeout)
{
    OpcUaServerLock serverLock;
    OpcUaSession session1(OpcUaNodeId(1, 1), &serverLock);
    session1.lockConfigurationControl(std::chrono::seconds(0));

    OpcUaSession session2(OpcUaNodeId(1, 2), &serverLock);
    ASSERT_TRUE(session2.lockConfigurationControl(std::chrono::seconds(10)));
}

TEST_F(OpcUaSessionTest, LockConfigurationControlAfterPasswordLock)
{
    OpcUaServerLock serverLock;
    serverLock.passwordLock("test");

    OpcUaSession session(OpcUaNodeId(1, 1), &serverLock);
    ASSERT_TRUE(session.lockConfigurationControl(std::chrono::seconds(10)));
}

TEST_F(OpcUaSessionTest, RefuseConfigurationControlLock)
{
    OpcUaServerLock serverLock;
    serverLock.passwordLock("test");

    OpcUaSession session(OpcUaNodeId(1, 1), &serverLock);
    session.lockConfigurationControl(std::chrono::seconds(10));

    ASSERT_NO_THROW(session.refuseConfigurationControlLock());
}

TEST_F(OpcUaSessionTest, PasswordLock)
{
    OpcUaServerLock serverLock;

    OpcUaSession session(OpcUaNodeId(1, 1), &serverLock);
    ASSERT_TRUE(session.passwordLock("test"));
}

TEST_F(OpcUaSessionTest, PasswordLockConfigurationLock)
{
    OpcUaServerLock serverLock;

    OpcUaSession session(OpcUaNodeId(1, 1), &serverLock);
    session.lockConfigurationControl(std::chrono::seconds(10));
    ASSERT_TRUE(session.passwordLock("test"));
}

TEST_F(OpcUaSessionTest, PasswordLockConfigurationLockByAnotherSession)
{
    OpcUaServerLock serverLock;

    OpcUaSession session1(OpcUaNodeId(1, 1), &serverLock);
    session1.lockConfigurationControl(std::chrono::seconds(10));

    OpcUaSession session2(OpcUaNodeId(1, 2), &serverLock);
    ASSERT_FALSE(session2.passwordLock("test"));
}

TEST_F(OpcUaSessionTest, PasswordUnlock)
{
    OpcUaServerLock serverLock;

    OpcUaSession session(OpcUaNodeId(1, 1), &serverLock);
    session.passwordLock("test");
    ASSERT_TRUE(session.passwordUnlock("test"));
}

TEST_F(OpcUaSessionTest, PasswordUnlockConfigurationLock)
{
    OpcUaServerLock serverLock;

    OpcUaSession session(OpcUaNodeId(1, 1), &serverLock);
    session.lockConfigurationControl(std::chrono::seconds(10));

    session.passwordLock("test");
    ASSERT_TRUE(session.passwordUnlock("test"));
}

TEST_F(OpcUaSessionTest, PasswordUnlockConfigurationLockByAnotherSession)
{
    OpcUaServerLock serverLock;

    OpcUaSession session1(OpcUaNodeId(1, 1), &serverLock);
    session1.lockConfigurationControl(std::chrono::seconds(10));
    session1.passwordLock("test");

    OpcUaSession session2(OpcUaNodeId(1, 2), &serverLock);
    ASSERT_FALSE(session2.passwordUnlock("test"));
}

TEST_F(OpcUaSessionTest, RefuseConfigurationControlLockWithoutLock)
{
    OpcUaServerLock serverLock;
    serverLock.passwordLock("test");

    OpcUaSession session(OpcUaNodeId(1, 1), &serverLock);
    ASSERT_NO_THROW(session.refuseConfigurationControlLock());
}

TEST_F(OpcUaSessionTest, RefuseConfigurationControlLockLockedByAnotherSession)
{
    OpcUaServerLock serverLock;

    OpcUaSession session1(OpcUaNodeId(1, 1), &serverLock);
    session1.lockConfigurationControl(std::chrono::seconds(10));

    OpcUaSession session2(OpcUaNodeId(1, 2), &serverLock);
    ASSERT_NO_THROW(session2.refuseConfigurationControlLock());
}

TEST_F(OpcUaSessionTest, HasConfigurationControlLock)
{
    OpcUaServerLock serverLock;
    OpcUaSession session(OpcUaNodeId(1, 1), &serverLock);
    ASSERT_FALSE(session.hasConfigurationControlLock());
}

TEST_F(OpcUaSessionTest, HasConfigurationControlLockAfterLock)
{
    OpcUaServerLock serverLock;
    OpcUaSession session(OpcUaNodeId(1, 1), &serverLock);
    session.lockConfigurationControl(std::chrono::seconds(10));
    ASSERT_TRUE(session.hasConfigurationControlLock());
}

TEST_F(OpcUaSessionTest, HasConfigurationControlLockAfterLockTimeout)
{
    OpcUaServerLock serverLock;
    OpcUaSession session(OpcUaNodeId(1, 1), &serverLock);
    session.lockConfigurationControl(std::chrono::seconds(0)); //expires immediately
    ASSERT_FALSE(session.hasConfigurationControlLock());
}

TEST_F(OpcUaSessionTest, HasConfigurationControlLockAfterAnotherSessionLock)
{
    OpcUaServerLock serverLock;
    OpcUaSession session1(OpcUaNodeId(1, 1), &serverLock);
    session1.lockConfigurationControl(std::chrono::seconds(10));

    OpcUaSession session2(OpcUaNodeId(1, 2), &serverLock);
    ASSERT_FALSE(session2.hasConfigurationControlLock());
}

TEST_F(OpcUaSessionTest, CanControlAcq)
{
    OpcUaServerLock serverLock;
    OpcUaSession session(OpcUaNodeId(1, 1), &serverLock);
    ASSERT_TRUE(session.canControlAcq());
}

TEST_F(OpcUaSessionTest, CanControlAcqPasswordLock)
{
    OpcUaServerLock serverLock;
    serverLock.passwordLock("Test");
    OpcUaSession session(OpcUaNodeId(1, 1), &serverLock);
    ASSERT_FALSE(session.canControlAcq());
}

TEST_F(OpcUaSessionTest, CanControlAcqPasswordUnlock)
{
    OpcUaServerLock serverLock;
    serverLock.passwordLock("Test");
    serverLock.passwordUnlock("Test");
    OpcUaSession session(OpcUaNodeId(1, 1), &serverLock);
    ASSERT_TRUE(session.canControlAcq());
}

TEST_F(OpcUaSessionTest, CanControlAcqConfigurationControlLock)
{
    OpcUaServerLock serverLock;
    OpcUaSession session(OpcUaNodeId(1, 1), &serverLock);

    session.lockConfigurationControl(std::chrono::seconds(10));
    ASSERT_TRUE(session.canControlAcq());
}

TEST_F(OpcUaSessionTest, CanControlAcqConfigurationControlLockByOtherSession)
{
    OpcUaServerLock serverLock;
    OpcUaSession session1(OpcUaNodeId(1, 1), &serverLock);
    session1.lockConfigurationControl(std::chrono::seconds(10));

    OpcUaSession session2(OpcUaNodeId(1, 2), &serverLock);
    ASSERT_FALSE(session2.canControlAcq());
}

TEST_F(OpcUaSessionTest, CanControlAcqConfigurationControlLockByOtherSessionAfterTimeout)
{
    OpcUaServerLock serverLock;
    OpcUaSession session1(OpcUaNodeId(1, 1), &serverLock);
    session1.lockConfigurationControl(std::chrono::seconds(0));

    OpcUaSession session2(OpcUaNodeId(1, 2), &serverLock);
    ASSERT_TRUE(session2.canControlAcq());
}

END_NAMESPACE_OPENDAQ_OPCUA
