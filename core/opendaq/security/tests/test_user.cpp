#include <gtest/gtest.h>
#include <opendaq/user_factory.h>
#include <opendaq/authentication_provider_factory.h>
#include <opendaq/permission_manager_factory.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>

using namespace daq;

using UserTest = testing::Test;

TEST_F(UserTest, Create)
{
    auto object = PropertyObject();
    object.addProperty(StringProperty("name", "device"));

    auto amplObject = PropertyObject();
    amplObject.addProperty(StringProperty("name", "StgAmpl"));
    object.addProperty(ObjectProperty("ampl", amplObject));

    auto user = User("janm", "pass", List<IString>("admin", "user"));
    ASSERT_TRUE(user.assigned());

    auto provider = AuthenticationProvider();
    ASSERT_TRUE(provider.assigned());

    auto manager = PermissionManager();
    manager.setPermissions("admin", AccessPermission::Read | AccessPermission::Write);
    auto a1 = manager.isAuthorized(user, AccessPermission::Read);
    auto a2 = manager.isAuthorized(user, AccessPermission::Write);
    auto a3 = manager.isAuthorized(user, AccessPermission::Execute);
    ASSERT_TRUE(manager.assigned());
}
