#include <gtest/gtest.h>
#include <opendaq/permission_manager_factory.h>
#include <opendaq/permission_manager_private_ptr.h>
#include <opendaq/user_factory.h>
#include <opendaq/permission_config_builder_factory.h>

using namespace daq;

using PermissionManagerTest = testing::Test;

TEST_F(PermissionManagerTest, Create)
{
    auto manager = PermissionManager();
    ASSERT_TRUE(manager.assigned());
}

TEST_F(PermissionManagerTest, CreateNested)
{
    auto managerRoot = PermissionManager();
    auto manager = PermissionManager(managerRoot);

    ASSERT_TRUE(managerRoot.assigned());
    ASSERT_TRUE(manager.assigned());
}

TEST_F(PermissionManagerTest, IsAuthorizedSimple)
{
    auto admin = User("admin", "password", List<IString>("admin", "guest"));
    auto user = User("user", "password", List<IString>("user"));
    auto guest = User("guest", "password", List<IString>("guest"));

    auto manager = PermissionManager();
    manager.setPermissionConfig(
        PermissionConfigBuilder().set("admin", Permission::Read | Permission::Write).set("user", Permission::Read).build());

    ASSERT_TRUE(manager.isAuthorized(admin, Permission::Read));
    ASSERT_TRUE(manager.isAuthorized(admin, Permission::Write));
    ASSERT_FALSE(manager.isAuthorized(admin, Permission::Execute));

    ASSERT_TRUE(manager.isAuthorized(user, Permission::Read));
    ASSERT_FALSE(manager.isAuthorized(user, Permission::Write));
    ASSERT_FALSE(manager.isAuthorized(user, Permission::Execute));

    ASSERT_FALSE(manager.isAuthorized(guest, Permission::Read));
    ASSERT_FALSE(manager.isAuthorized(guest, Permission::Write));
    ASSERT_FALSE(manager.isAuthorized(guest, Permission::Execute));
}

TEST_F(PermissionManagerTest, IsAuthorizedInherited)
{
    auto admin = User("admin", "password", List<IString>("admin", "guest"));
    auto user = User("user", "password", List<IString>("user"));

    auto managerRoot = PermissionManager();
    managerRoot.setPermissionConfig(PermissionConfigBuilder().set("admin", Permission::Read | Permission::Write).build());

    auto manager = PermissionManager(managerRoot);
    manager.setPermissionConfig(PermissionConfigBuilder().inherit(true).set("user", Permission::Read).build());

    ASSERT_TRUE(managerRoot.isAuthorized(admin, Permission::Read));
    ASSERT_TRUE(managerRoot.isAuthorized(admin, Permission::Write));
    ASSERT_FALSE(managerRoot.isAuthorized(admin, Permission::Execute));

    ASSERT_TRUE(manager.isAuthorized(admin, Permission::Read));
    ASSERT_TRUE(manager.isAuthorized(admin, Permission::Write));
    ASSERT_FALSE(manager.isAuthorized(admin, Permission::Execute));

    ASSERT_TRUE(manager.isAuthorized(user, Permission::Read));
    ASSERT_FALSE(manager.isAuthorized(user, Permission::Write));
    ASSERT_FALSE(manager.isAuthorized(user, Permission::Execute));
}

TEST_F(PermissionManagerTest, IsAuthorizedNotInherited)
{
    auto admin = User("admin", "password", List<IString>("admin", "guest"));
    auto user = User("user", "password", List<IString>("user"));

    auto managerRoot = PermissionManager();
    managerRoot.setPermissionConfig(PermissionConfigBuilder().set("admin", Permission::Read | Permission::Write).build());

    auto manager = PermissionManager(managerRoot);
    manager.setPermissionConfig(PermissionConfigBuilder().inherit(false).set("user", Permission::Read | Permission::Execute).build());

    ASSERT_TRUE(managerRoot.isAuthorized(admin, Permission::Read));
    ASSERT_TRUE(managerRoot.isAuthorized(admin, Permission::Write));
    ASSERT_FALSE(managerRoot.isAuthorized(admin, Permission::Execute));

    ASSERT_FALSE(manager.isAuthorized(admin, Permission::Read));
    ASSERT_FALSE(manager.isAuthorized(admin, Permission::Write));
    ASSERT_FALSE(manager.isAuthorized(admin, Permission::Execute));

    ASSERT_TRUE(manager.isAuthorized(user, Permission::Read));
    ASSERT_FALSE(manager.isAuthorized(user, Permission::Write));
    ASSERT_TRUE(manager.isAuthorized(user, Permission::Execute));
}

TEST_F(PermissionManagerTest, UpdateInherited)
{
    auto admin = User("admin", "password", List<IString>("admin", "guest"));

    auto managerRoot = PermissionManager();
    managerRoot.setPermissionConfig(PermissionConfigBuilder().set("admin", Permission::Read | Permission::Write).build());

    auto manager = PermissionManager(managerRoot);
    manager.setPermissionConfig(PermissionConfigBuilder().inherit(true).build());

    ASSERT_TRUE(manager.isAuthorized(admin, Permission::Read));
    ASSERT_TRUE(manager.isAuthorized(admin, Permission::Write));
    ASSERT_FALSE(manager.isAuthorized(admin, Permission::Execute));

    managerRoot.setPermissionConfig(PermissionConfigBuilder().set("admin", Permission::Read).build());

    ASSERT_TRUE(manager.isAuthorized(admin, Permission::Read));
    ASSERT_FALSE(manager.isAuthorized(admin, Permission::Write));
    ASSERT_FALSE(manager.isAuthorized(admin, Permission::Execute));
}

TEST_F(PermissionManagerTest, Allow)
{
    auto admin = User("admin", "password", List<IString>("admin", "guest"));

    auto managerRoot = PermissionManager();
    managerRoot.setPermissionConfig(PermissionConfigBuilder().set("admin", Permission::Read | Permission::Write).build());

    auto manager = PermissionManager(managerRoot);
    manager.setPermissionConfig(PermissionConfigBuilder().inherit(true).allow("admin", Permission::Execute).build());

    ASSERT_TRUE(manager.isAuthorized(admin, Permission::Read));
    ASSERT_TRUE(manager.isAuthorized(admin, Permission::Write));
    ASSERT_TRUE(manager.isAuthorized(admin, Permission::Execute));
}

TEST_F(PermissionManagerTest, Deny)
{
    auto admin = User("admin", "password", List<IString>("admin", "guest"));

    auto managerRoot = PermissionManager();
    managerRoot.setPermissionConfig(PermissionConfigBuilder().set("admin", Permission::Read | Permission::Write).build());

    auto manager = PermissionManager(managerRoot);
    manager.setPermissionConfig(PermissionConfigBuilder().inherit(true).deny("admin", Permission::Write).build());

    ASSERT_TRUE(manager.isAuthorized(admin, Permission::Read));
    ASSERT_FALSE(manager.isAuthorized(admin, Permission::Write));
    ASSERT_FALSE(manager.isAuthorized(admin, Permission::Execute));
}

TEST_F(PermissionManagerTest, UpdateAllowRoot)
{
    auto admin = User("admin", "password", List<IString>("admin", "guest"));

    auto managerRoot = PermissionManager();
    managerRoot.setPermissionConfig(PermissionConfigBuilder().set("admin", Permission::Read | Permission::Write).build());

    auto manager = PermissionManager(managerRoot);
    manager.setPermissionConfig(PermissionConfigBuilder().inherit(true).build());

    ASSERT_TRUE(manager.isAuthorized(admin, Permission::Read));
    ASSERT_TRUE(manager.isAuthorized(admin, Permission::Write));
    ASSERT_FALSE(manager.isAuthorized(admin, Permission::Execute));

    managerRoot.setPermissionConfig(PermissionConfigBuilder().allow("admin", Permission::Execute).build());

    ASSERT_FALSE(manager.isAuthorized(admin, Permission::Read));
    ASSERT_FALSE(manager.isAuthorized(admin, Permission::Write));
    ASSERT_TRUE(manager.isAuthorized(admin, Permission::Execute));
}

TEST_F(PermissionManagerTest, SetParent)
{
    auto admin = User("admin", "password", List<IString>("admin", "guest"));

    auto manager = PermissionManager();
    manager.setPermissionConfig(PermissionConfigBuilder().inherit(true).allow("admin", Permission::Read).build());

    ASSERT_TRUE(manager.isAuthorized(admin, Permission::Read));
    ASSERT_FALSE(manager.isAuthorized(admin, Permission::Write));
    ASSERT_FALSE(manager.isAuthorized(admin, Permission::Execute));

    auto managerRoot = PermissionManager();
    managerRoot.setPermissionConfig(PermissionConfigBuilder().allow("admin", Permission::Read | Permission::Write).build());
    manager.asPtr<IPermissionManagerPrivate>().setParent(managerRoot);

    ASSERT_TRUE(manager.isAuthorized(admin, Permission::Read));
    ASSERT_TRUE(manager.isAuthorized(admin, Permission::Write));
    ASSERT_FALSE(manager.isAuthorized(admin, Permission::Execute));
}

TEST_F(PermissionManagerTest, ChangeParent)
{
    auto admin = User("admin", "password", List<IString>("admin", "guest"));

    auto managerRoot1 = PermissionManager();
    managerRoot1.setPermissionConfig(PermissionConfigBuilder().set("admin", Permission::Read | Permission::Write).build());

    auto managerRoot2 = PermissionManager();
    managerRoot2.setPermissionConfig(PermissionConfigBuilder().set("admin", Permission::Read).build());

    auto manager = PermissionManager(managerRoot1);
    manager.setPermissionConfig(PermissionConfigBuilder().inherit(true).build());

    ASSERT_TRUE(manager.isAuthorized(admin, Permission::Read));
    ASSERT_TRUE(manager.isAuthorized(admin, Permission::Write));
    ASSERT_FALSE(manager.isAuthorized(admin, Permission::Execute));

    manager.asPtr<IPermissionManagerPrivate>().setParent(managerRoot2);

    ASSERT_TRUE(manager.isAuthorized(admin, Permission::Read));
    ASSERT_FALSE(manager.isAuthorized(admin, Permission::Write));
    ASSERT_FALSE(manager.isAuthorized(admin, Permission::Execute));
}

TEST_F(PermissionManagerTest, SetNullParent)
{
    auto admin = User("admin", "password", List<IString>("admin", "guest"));

    auto managerRoot = PermissionManager();
    managerRoot.setPermissionConfig(PermissionConfigBuilder().set("admin", Permission::Read | Permission::Write).build());

    auto manager = PermissionManager(managerRoot);
    manager.setPermissionConfig(PermissionConfigBuilder().inherit(true).build());

    ASSERT_TRUE(manager.isAuthorized(admin, Permission::Read));
    ASSERT_TRUE(manager.isAuthorized(admin, Permission::Write));
    ASSERT_FALSE(manager.isAuthorized(admin, Permission::Execute));

    manager.asPtr<IPermissionManagerPrivate>().setParent(nullptr);

    ASSERT_FALSE(manager.isAuthorized(admin, Permission::Read));
    ASSERT_FALSE(manager.isAuthorized(admin, Permission::Write));
    ASSERT_FALSE(manager.isAuthorized(admin, Permission::Execute));
}
