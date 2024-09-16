#include <gtest/gtest.h>
#include <coreobjects/permission_manager_factory.h>
#include <coreobjects/permission_manager_internal_ptr.h>
#include <coreobjects/user_factory.h>
#include <coreobjects/permissions_builder_factory.h>
#include <coreobjects/permission_mask_builder_factory.h>

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
    manager.setPermissions(
        PermissionsBuilder().assign("admin", PermissionMaskBuilder().read().write()).assign("user", PermissionMaskBuilder().read()).build());

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
    managerRoot.setPermissions(PermissionsBuilder().assign("admin", PermissionMaskBuilder().read().write()).build());

    auto manager = PermissionManager(managerRoot);
    manager.setPermissions(PermissionsBuilder().inherit(true).assign("user", PermissionMaskBuilder().read()).build());

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
    managerRoot.setPermissions(PermissionsBuilder().assign("admin", PermissionMaskBuilder().read().write()).build());

    auto manager = PermissionManager(managerRoot);
    manager.setPermissions(PermissionsBuilder().inherit(false).assign("user", PermissionMaskBuilder().read().execute()).build());

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
    managerRoot.setPermissions(PermissionsBuilder().assign("admin", PermissionMaskBuilder().read().write()).build());

    auto manager = PermissionManager(managerRoot);
    manager.setPermissions(PermissionsBuilder().inherit(true).build());

    ASSERT_TRUE(manager.isAuthorized(admin, Permission::Read));
    ASSERT_TRUE(manager.isAuthorized(admin, Permission::Write));
    ASSERT_FALSE(manager.isAuthorized(admin, Permission::Execute));

    managerRoot.setPermissions(PermissionsBuilder().assign("admin", PermissionMaskBuilder().read()).build());

    ASSERT_TRUE(manager.isAuthorized(admin, Permission::Read));
    ASSERT_FALSE(manager.isAuthorized(admin, Permission::Write));
    ASSERT_FALSE(manager.isAuthorized(admin, Permission::Execute));
}

TEST_F(PermissionManagerTest, Allow)
{
    auto admin = User("admin", "password", List<IString>("admin", "guest"));

    auto managerRoot = PermissionManager();
    managerRoot.setPermissions(PermissionsBuilder().assign("admin", PermissionMaskBuilder().read().write()).build());

    auto manager = PermissionManager(managerRoot);
    manager.setPermissions(PermissionsBuilder().inherit(true).allow("admin", PermissionMaskBuilder().execute()).build());

    ASSERT_TRUE(manager.isAuthorized(admin, Permission::Read));
    ASSERT_TRUE(manager.isAuthorized(admin, Permission::Write));
    ASSERT_TRUE(manager.isAuthorized(admin, Permission::Execute));
}

TEST_F(PermissionManagerTest, Deny)
{
    auto admin = User("admin", "password", List<IString>("admin", "guest"));

    auto managerRoot = PermissionManager();
    managerRoot.setPermissions(PermissionsBuilder().assign("admin", PermissionMaskBuilder().read().write()).build());

    auto manager = PermissionManager(managerRoot);
    manager.setPermissions(PermissionsBuilder().inherit(true).deny("admin", PermissionMaskBuilder().write()).build());

    ASSERT_TRUE(manager.isAuthorized(admin, Permission::Read));
    ASSERT_FALSE(manager.isAuthorized(admin, Permission::Write));
    ASSERT_FALSE(manager.isAuthorized(admin, Permission::Execute));
}

TEST_F(PermissionManagerTest, UpdateAllowRoot)
{
    auto admin = User("admin", "password", List<IString>("admin", "guest"));

    auto managerRoot = PermissionManager();
    managerRoot.setPermissions(PermissionsBuilder().assign("admin", PermissionMaskBuilder().read().write()).build());

    auto manager = PermissionManager(managerRoot);
    manager.setPermissions(PermissionsBuilder().inherit(true).build());

    ASSERT_TRUE(manager.isAuthorized(admin, Permission::Read));
    ASSERT_TRUE(manager.isAuthorized(admin, Permission::Write));
    ASSERT_FALSE(manager.isAuthorized(admin, Permission::Execute));

    managerRoot.setPermissions(PermissionsBuilder().allow("admin", PermissionMaskBuilder().execute()).build());

    ASSERT_FALSE(manager.isAuthorized(admin, Permission::Read));
    ASSERT_FALSE(manager.isAuthorized(admin, Permission::Write));
    ASSERT_TRUE(manager.isAuthorized(admin, Permission::Execute));
}

TEST_F(PermissionManagerTest, SetParent)
{
    auto admin = User("admin", "password", List<IString>("admin", "guest"));

    auto manager = PermissionManager();
    manager.setPermissions(PermissionsBuilder().inherit(true).allow("admin", PermissionMaskBuilder().read()).build());

    ASSERT_TRUE(manager.isAuthorized(admin, Permission::Read));
    ASSERT_FALSE(manager.isAuthorized(admin, Permission::Write));
    ASSERT_FALSE(manager.isAuthorized(admin, Permission::Execute));

    auto managerRoot = PermissionManager();
    managerRoot.setPermissions(PermissionsBuilder().allow("admin", PermissionMaskBuilder().read().write()).build());
    manager.asPtr<IPermissionManagerInternal>().setParent(managerRoot);

    ASSERT_TRUE(manager.isAuthorized(admin, Permission::Read));
    ASSERT_TRUE(manager.isAuthorized(admin, Permission::Write));
    ASSERT_FALSE(manager.isAuthorized(admin, Permission::Execute));
}

TEST_F(PermissionManagerTest, ChangeParent)
{
    auto admin = User("admin", "password", List<IString>("admin", "guest"));

    auto managerRoot1 = PermissionManager();
    managerRoot1.setPermissions(PermissionsBuilder().assign("admin", PermissionMaskBuilder().read().write()).build());

    auto managerRoot2 = PermissionManager();
    managerRoot2.setPermissions(PermissionsBuilder().assign("admin", PermissionMaskBuilder().read()).build());

    auto manager = PermissionManager(managerRoot1);
    manager.setPermissions(PermissionsBuilder().inherit(true).build());

    ASSERT_TRUE(manager.isAuthorized(admin, Permission::Read));
    ASSERT_TRUE(manager.isAuthorized(admin, Permission::Write));
    ASSERT_FALSE(manager.isAuthorized(admin, Permission::Execute));

    manager.asPtr<IPermissionManagerInternal>().setParent(managerRoot2);

    ASSERT_TRUE(manager.isAuthorized(admin, Permission::Read));
    ASSERT_FALSE(manager.isAuthorized(admin, Permission::Write));
    ASSERT_FALSE(manager.isAuthorized(admin, Permission::Execute));
}

TEST_F(PermissionManagerTest, SetNullParent)
{
    auto admin = User("admin", "password", List<IString>("admin", "guest"));

    auto managerRoot = PermissionManager();
    managerRoot.setPermissions(PermissionsBuilder().assign("admin", PermissionMaskBuilder().read().write()).build());

    auto manager = PermissionManager(managerRoot);
    manager.setPermissions(PermissionsBuilder().inherit(true).build());

    ASSERT_TRUE(manager.isAuthorized(admin, Permission::Read));
    ASSERT_TRUE(manager.isAuthorized(admin, Permission::Write));
    ASSERT_FALSE(manager.isAuthorized(admin, Permission::Execute));

    manager.asPtr<IPermissionManagerInternal>().setParent(nullptr);

    ASSERT_FALSE(manager.isAuthorized(admin, Permission::Read));
    ASSERT_FALSE(manager.isAuthorized(admin, Permission::Write));
    ASSERT_FALSE(manager.isAuthorized(admin, Permission::Execute));
}


TEST_F(PermissionManagerTest, AssignRemovePermission)
{
    auto admin = User("admin", "password", List<IString>("admin"));
    auto guest = User("guest", "password", List<IString>("guest"));

    auto managerRoot = PermissionManager();
    managerRoot.setPermissions(PermissionsBuilder()
                                   .assign("guest", PermissionMaskBuilder().read().write())
                                   .assign("admin", PermissionMaskBuilder().read().write())
                                   .build());

    auto manager = PermissionManager(managerRoot);
    manager.setPermissions(PermissionsBuilder().inherit(true).assign("admin", PermissionMaskBuilder().read()).build());

    ASSERT_TRUE(manager.isAuthorized(guest, Permission::Read));
    ASSERT_TRUE(manager.isAuthorized(guest, Permission::Write));
    ASSERT_FALSE(manager.isAuthorized(guest, Permission::Execute));

    ASSERT_TRUE(manager.isAuthorized(admin, Permission::Read));
    ASSERT_FALSE(manager.isAuthorized(admin, Permission::Write));
    ASSERT_FALSE(manager.isAuthorized(admin, Permission::Execute));
}

TEST_F(PermissionManagerTest, AssignAddPermission)
{
    auto admin = User("admin", "password", List<IString>("admin"));
    auto guest = User("guest", "password", List<IString>("guest"));

    auto managerRoot = PermissionManager();
    managerRoot.setPermissions(PermissionsBuilder()
                                   .assign("guest", PermissionMaskBuilder().read().write())
                                   .assign("admin", PermissionMaskBuilder().read().write())
                                   .build());

    auto manager = PermissionManager(managerRoot);
    manager.setPermissions(PermissionsBuilder().inherit(true).assign("admin", PermissionMaskBuilder().read().write().execute()).build());

    ASSERT_TRUE(manager.isAuthorized(guest, Permission::Read));
    ASSERT_TRUE(manager.isAuthorized(guest, Permission::Write));
    ASSERT_FALSE(manager.isAuthorized(guest, Permission::Execute));

    ASSERT_TRUE(manager.isAuthorized(admin, Permission::Read));
    ASSERT_TRUE(manager.isAuthorized(admin, Permission::Write));
    ASSERT_TRUE(manager.isAuthorized(admin, Permission::Execute));
}

