#include <gtest/gtest.h>
#include <coreobjects/permission_mask_builder_factory.h>

using namespace daq;

using PermissionMaskBuilderTest = testing::Test;

TEST_F(PermissionMaskBuilderTest, Create)
{
    auto builder = PermissionMaskBuilder();
    ASSERT_TRUE(builder.assigned());
}

TEST_F(PermissionMaskBuilderTest, Default)
{
    auto mask = PermissionMaskBuilder().build();
    ASSERT_EQ(mask, 0);
}

TEST_F(PermissionMaskBuilderTest, Setters)
{
    auto r = PermissionMaskBuilder().read().build();
    ASSERT_EQ(r, (Int) Permission::Read);

    auto w = PermissionMaskBuilder().write().build();
    ASSERT_EQ(w, (Int) Permission::Write);

    auto e = PermissionMaskBuilder().execute().build();
    ASSERT_EQ(e, (Int) Permission::Execute);

    auto all = PermissionMaskBuilder().read().write().execute().build();
    ASSERT_EQ(all, (Int) (Permission::Read | Permission::Write | Permission::Execute));

    auto rw = PermissionMaskBuilder().read().write().build();
    ASSERT_EQ(rw, (Int) (Permission::Read | Permission::Write));

    auto wr = PermissionMaskBuilder().write().read().build();
    ASSERT_EQ(wr, (Int) (Permission::Read | Permission::Write));
}

TEST_F(PermissionMaskBuilderTest, Clear)
{
    auto mask1 = PermissionMaskBuilder().clear().build();
    ASSERT_EQ(mask1, 0);

    auto mask2 = PermissionMaskBuilder().read().write().clear().build();
    ASSERT_EQ(mask2, 0);

    auto mask3 = PermissionMaskBuilder().read().write().clear().write().execute().build();
    ASSERT_EQ(mask3, (Int) (Permission::Write | Permission::Execute));
}

TEST_F(PermissionMaskBuilderTest, FromIntMask)
{
    Int rw = (Int) (Permission::Read | Permission::Write);

    auto rwCopy = PermissionMaskBuilder(rw).build();
    ASSERT_EQ(rw, rwCopy);

    auto rwe = PermissionMaskBuilder(rw).execute().build();
    ASSERT_EQ(rwe, (Int) (Permission::Read | Permission::Write | Permission::Execute));
}
