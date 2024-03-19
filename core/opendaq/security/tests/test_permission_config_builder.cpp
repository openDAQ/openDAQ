#include <gtest/gtest.h>
#include <opendaq/permission_config_builder_factory.h>
#include <opendaq/permission_manager.h>

using namespace daq;

using PermissionConfigBuilderTest = testing::Test;

TEST_F(PermissionConfigBuilderTest, Create)
{
    auto builder = PermissionConfigBuilder();
    ASSERT_TRUE(builder.assigned());
}

TEST_F(PermissionConfigBuilderTest, Inherited)
{
    auto builder = PermissionConfigBuilder();

    auto config1 = builder.inherit(false).build();
    ASSERT_EQ(config1.getInherited(), false);

    auto config2 = builder.inherit(true).build();
    ASSERT_EQ(config2.getInherited(), true);
}

TEST_F(PermissionConfigBuilderTest, Set)
{
    auto builder = PermissionConfigBuilder();
    auto config = builder.set("admin", Permission::Read | Permission::Write).set("user", Permission::Read).build();
    Int mask;

    ASSERT_EQ(config.getAllowed().getCount(), 2);
    mask = config.getAllowed().get("admin");
    ASSERT_EQ(mask, Permission::Read | Permission::Write);
    mask = config.getAllowed().get("user");
    ASSERT_EQ(mask, Permission::Read);

    ASSERT_EQ(config.getDenied().getCount(), 2);
    mask = config.getDenied().get("admin");
    ASSERT_EQ(mask, 0);
    mask = config.getDenied().get("user");
    ASSERT_EQ(mask, 0);
}

TEST_F(PermissionConfigBuilderTest, SetOverwrite)
{
    auto builder = PermissionConfigBuilder();
    auto config = builder.set("admin", Permission::Read | Permission::Write).set("admin", Permission::Read).build();

    Int mask = config.getAllowed().get("admin");
    ASSERT_EQ(mask, Permission::Read);
}

TEST_F(PermissionConfigBuilderTest, Allow)
{
    auto builder = PermissionConfigBuilder();
    auto config = builder.set("user", Permission::Read).allow("user", Permission::Write).build();
    Int mask;

    mask = config.getAllowed().get("user");
    ASSERT_EQ(mask, Permission::Read | Permission::Write);

    mask = config.getDenied().get("user");
    ASSERT_EQ(mask, 0);
}

TEST_F(PermissionConfigBuilderTest, Deny)
{
    auto builder = PermissionConfigBuilder();
    auto config = builder.set("user", Permission::Read).deny("user", Permission::Read).build();
    Int mask;

    mask = config.getAllowed().get("user");
    ASSERT_EQ(mask, 0);

    mask = config.getDenied().get("user");
    ASSERT_EQ(mask, Permission::Read);
}

TEST_F(PermissionConfigBuilderTest, AllowDeny)
{
    auto builder = PermissionConfigBuilder();
    auto config = builder.allow("user", Permission::Read).deny("user", Permission::Read).build();
    Int mask;

    mask = config.getAllowed().get("user");
    ASSERT_EQ(mask, 0);

    mask = config.getDenied().get("user");
    ASSERT_EQ(mask, Permission::Read);
}

TEST_F(PermissionConfigBuilderTest, DenyAllow)
{
    auto builder = PermissionConfigBuilder();
    auto config = builder.deny("user", Permission::Read).allow("user", Permission::Read).build();
    Int mask;

    mask = config.getAllowed().get("user");
    ASSERT_EQ(mask, Permission::Read);

    mask = config.getDenied().get("user");
    ASSERT_EQ(mask, 0);
}

TEST_F(PermissionConfigBuilderTest, ExtendSimple)
{
    auto additionalConfig = PermissionConfigBuilder().allow("manager", Permission::Execute).build();

    auto builder = PermissionConfigBuilder();
    auto config = builder.set("admin", Permission::Read | Permission::Write).extend(additionalConfig).build();
    Int mask;

    mask = config.getAllowed().get("admin");
    ASSERT_EQ(mask, Permission::Read | Permission::Write);
    mask = config.getDenied().get("admin");
    ASSERT_EQ(mask, 0);

    mask = config.getAllowed().get("manager");
    ASSERT_EQ(mask, Permission::Execute);
    mask = config.getDenied().get("manager");
    ASSERT_EQ(mask, 0);
}

TEST_F(PermissionConfigBuilderTest, Extend)
{
    auto additionalConfig = PermissionConfigBuilder()
                                .deny("admin", Permission::Write)
                                .allow("admin", Permission::Execute)
                                .allow("user", Permission::Write)
                                .allow("manager", Permission::Write)
                                .set("guest", Permission::Read)
                                .build();

    auto builder = PermissionConfigBuilder();
    auto config = builder.set("admin", Permission::Read | Permission::Write).set("user", Permission::Read).extend(additionalConfig).build();
    Int mask;

    mask = config.getAllowed().get("admin");
    ASSERT_EQ(mask, Permission::Read | Permission::Execute);
    mask = config.getDenied().get("admin");
    ASSERT_EQ(mask, Permission::Write);

    mask = config.getAllowed().get("user");
    ASSERT_EQ(mask, Permission::Read | Permission::Write);
    mask = config.getDenied().get("user");
    ASSERT_EQ(mask, 0);

    mask = config.getAllowed().get("manager");
    ASSERT_EQ(mask, Permission::Write);
    mask = config.getDenied().get("manager");
    ASSERT_EQ(mask, 0);

    mask = config.getAllowed().get("guest");
    ASSERT_EQ(mask, Permission::Read);
    mask = config.getDenied().get("guest");
    ASSERT_EQ(mask, 0);
}
