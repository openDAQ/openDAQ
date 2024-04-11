#include <gtest/gtest.h>
#include <coreobjects/permissions_builder_factory.h>
#include <coreobjects/permission_manager.h>
#include <coreobjects/permission_mask_builder_factory.h>

using namespace daq;

using PermissionsBuilderTest = testing::Test;

TEST_F(PermissionsBuilderTest, Create)
{
    auto builder = PermissionsBuilder();
    ASSERT_TRUE(builder.assigned());
}

TEST_F(PermissionsBuilderTest, Inherited)
{
    auto builder = PermissionsBuilder();

    auto config1 = builder.inherit(false).build();
    ASSERT_EQ(config1.getInherited(), false);

    auto config2 = builder.inherit(true).build();
    ASSERT_EQ(config2.getInherited(), true);
}

TEST_F(PermissionsBuilderTest, Set)
{
    auto builder = PermissionsBuilder();
    auto config = builder.set("admin", PermissionMaskBuilder().read().write()).set("user", PermissionMaskBuilder().read()).build();
    Int mask;

    ASSERT_EQ(config.getAllowed().getCount(), 2);
    mask = config.getAllowed().get("admin");
    ASSERT_EQ(mask, (Int) (Permission::Read | Permission::Write));
    mask = config.getAllowed().get("user");
    ASSERT_EQ(mask, (Int) Permission::Read);

    ASSERT_EQ(config.getDenied().getCount(), 2);
    mask = config.getDenied().get("admin");
    ASSERT_EQ(mask, 0);
    mask = config.getDenied().get("user");
    ASSERT_EQ(mask, 0);
}

TEST_F(PermissionsBuilderTest, SetOverwrite)
{
    auto builder = PermissionsBuilder();
    auto config = builder.set("admin", PermissionMaskBuilder().read().write()).set("admin", PermissionMaskBuilder().read()).build();

    Int mask = config.getAllowed().get("admin");
    ASSERT_EQ(mask, (Int) Permission::Read);
}

TEST_F(PermissionsBuilderTest, Allow)
{
    auto builder = PermissionsBuilder();
    auto config = builder.set("user", PermissionMaskBuilder().read()).allow("user", PermissionMaskBuilder().write()).build();
    Int mask;

    mask = config.getAllowed().get("user");
    ASSERT_EQ(mask, (Int) (Permission::Read | Permission::Write));

    mask = config.getDenied().get("user");
    ASSERT_EQ(mask, 0);
}

TEST_F(PermissionsBuilderTest, Deny)
{
    auto builder = PermissionsBuilder();
    auto config = builder.set("user", PermissionMaskBuilder().read()).deny("user", PermissionMaskBuilder().read()).build();
    Int mask;

    mask = config.getAllowed().get("user");
    ASSERT_EQ(mask, 0);

    mask = config.getDenied().get("user");
    ASSERT_EQ(mask, (Int) Permission::Read);
}

TEST_F(PermissionsBuilderTest, AllowDeny)
{
    auto builder = PermissionsBuilder();
    auto config = builder.allow("user", PermissionMaskBuilder().read()).deny("user", PermissionMaskBuilder().read()).build();
    Int mask;

    mask = config.getAllowed().get("user");
    ASSERT_EQ(mask, 0);

    mask = config.getDenied().get("user");
    ASSERT_EQ(mask, (Int) Permission::Read);
}

TEST_F(PermissionsBuilderTest, DenyAllow)
{
    auto builder = PermissionsBuilder();
    auto config = builder.deny("user", PermissionMaskBuilder().read()).allow("user", PermissionMaskBuilder().read()).build();
    Int mask;

    mask = config.getAllowed().get("user");
    ASSERT_EQ(mask, (Int) Permission::Read);

    mask = config.getDenied().get("user");
    ASSERT_EQ(mask, 0);
}

TEST_F(PermissionsBuilderTest, ExtendSimple)
{
    auto additionalConfig = PermissionsBuilder().allow("manager", PermissionMaskBuilder().execute()).build();

    auto builder = PermissionsBuilder();
    auto config = builder.set("admin", PermissionMaskBuilder().read().write()).extend(additionalConfig).build();
    Int mask;

    mask = config.getAllowed().get("admin");
    ASSERT_EQ(mask, (Int) (Permission::Read | Permission::Write));
    mask = config.getDenied().get("admin");
    ASSERT_EQ(mask, 0);

    mask = config.getAllowed().get("manager");
    ASSERT_EQ(mask, (Int) Permission::Execute);
    mask = config.getDenied().get("manager");
    ASSERT_EQ(mask, 0);
}

TEST_F(PermissionsBuilderTest, Extend)
{
    auto additionalConfig = PermissionsBuilder()
                                .deny("admin", PermissionMaskBuilder().write())
                                .allow("admin", PermissionMaskBuilder().execute())
                                .allow("user", PermissionMaskBuilder().write())
                                .allow("manager", PermissionMaskBuilder().write())
                                .set("guest", PermissionMaskBuilder().read())
                                .build();

    auto builder = PermissionsBuilder();
    auto config = builder.set("admin", PermissionMaskBuilder().read().write())
                      .set("user", PermissionMaskBuilder().read())
                      .extend(additionalConfig)
                      .build();
    Int mask;

    mask = config.getAllowed().get("admin");
    ASSERT_EQ(mask, (Int) (Permission::Read | Permission::Execute));
    mask = config.getDenied().get("admin");
    ASSERT_EQ(mask, (Int) Permission::Write);

    mask = config.getAllowed().get("user");
    ASSERT_EQ(mask, (Int) (Permission::Read | Permission::Write));
    mask = config.getDenied().get("user");
    ASSERT_EQ(mask, 0);

    mask = config.getAllowed().get("manager");
    ASSERT_EQ(mask, (Int) Permission::Write);
    mask = config.getDenied().get("manager");
    ASSERT_EQ(mask, 0);

    mask = config.getAllowed().get("guest");
    ASSERT_EQ(mask, (Int) Permission::Read);
    mask = config.getDenied().get("guest");
    ASSERT_EQ(mask, 0);
}
