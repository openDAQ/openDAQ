#include <gtest/gtest.h>
#include <coreobjects/permissions_builder_factory.h>
#include <coreobjects/permission_manager.h>

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
    auto config =
        builder.set("admin", List<Permission>(Permission::Read, Permission::Write)).set("user", List<Permission>(Permission::Read)).build();
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
    auto config = builder.set("admin", List<Permission>(Permission::Read, Permission::Write))
                      .set("admin", List<Permission>(Permission::Read))
                      .build();

    Int mask = config.getAllowed().get("admin");
    ASSERT_EQ(mask, (Int) Permission::Read);
}

TEST_F(PermissionsBuilderTest, Allow)
{
    auto builder = PermissionsBuilder();
    auto config = builder.set("user", List<Permission>(Permission::Read)).allow("user", List<Permission>(Permission::Write)).build();
    Int mask;

    mask = config.getAllowed().get("user");
    ASSERT_EQ(mask, (Int) (Permission::Read | Permission::Write));

    mask = config.getDenied().get("user");
    ASSERT_EQ(mask, 0);
}

TEST_F(PermissionsBuilderTest, Deny)
{
    auto builder = PermissionsBuilder();
    auto config = builder.set("user", List<Permission>(Permission::Read)).deny("user", List<Permission>(Permission::Read)).build();
    Int mask;

    mask = config.getAllowed().get("user");
    ASSERT_EQ(mask, 0);

    mask = config.getDenied().get("user");
    ASSERT_EQ(mask, (Int) Permission::Read);
}

TEST_F(PermissionsBuilderTest, AllowDeny)
{
    auto builder = PermissionsBuilder();
    auto config = builder.allow("user", List<Permission>(Permission::Read)).deny("user", List<Permission>(Permission::Read)).build();
    Int mask;

    mask = config.getAllowed().get("user");
    ASSERT_EQ(mask, 0);

    mask = config.getDenied().get("user");
    ASSERT_EQ(mask, (Int) Permission::Read);
}

TEST_F(PermissionsBuilderTest, DenyAllow)
{
    auto builder = PermissionsBuilder();
    auto config = builder.deny("user", List<Permission>(Permission::Read)).allow("user", List<Permission>(Permission::Read)).build();
    Int mask;

    mask = config.getAllowed().get("user");
    ASSERT_EQ(mask, (Int) Permission::Read);

    mask = config.getDenied().get("user");
    ASSERT_EQ(mask, 0);
}

TEST_F(PermissionsBuilderTest, ExtendSimple)
{
    auto additionalConfig = PermissionsBuilder().allow("manager", List<Permission>(Permission::Execute)).build();

    auto builder = PermissionsBuilder();
    auto config = builder.set("admin", List<Permission>(Permission::Read, Permission::Write)).extend(additionalConfig).build();
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
                                .deny("admin", List<Permission>(Permission::Write))
                                .allow("admin", List<Permission>(Permission::Execute))
                                .allow("user", List<Permission>(Permission::Write))
                                .allow("manager", List<Permission>(Permission::Write))
                                .set("guest", List<Permission>(Permission::Read))
                                .build();

    auto builder = PermissionsBuilder();
    auto config = builder.set("admin", List<Permission>(Permission::Read, Permission::Write))
                      .set("user", List<Permission>(Permission::Read))
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
