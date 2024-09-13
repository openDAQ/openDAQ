#include <gtest/gtest.h>
#include <opendaq/opendaq.h>

using HowToAccessControl = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

// Corresponding document: Antora/modules/howto_guides/pages/howto_access_control.adoc

TEST_F(HowToAccessControl, CreateServer)
{
    auto users = List<IUser>();
    users.pushBack(User("opendaq", "opendaq123"));
    users.pushBack(User("root", "root123", {"admin"}));
    const AuthenticationProviderPtr authenticationProvider = StaticAuthenticationProvider(true, users);

    const InstanceBuilderPtr instanceBuilder = InstanceBuilder().setAuthenticationProvider(authenticationProvider);

    const InstancePtr instance = InstanceFromBuilder(instanceBuilder);
    instance.addStandardServers();

    //std::cin.get();
}

TEST_F(HowToAccessControl, Hashing)
{
    auto users = List<IUser>();
    users.pushBack(User("opendaq", "$2a$12$MmSt1b9YEHB5SpLNyikiD.37NvN23UA7zLH6Y98ob5HF0OsKH0IuO"));
    users.pushBack(User("root", "$2a$12$ceV7Q2j.vZcuz05hy1EkC.GHH8PIrv0D5wz7iLH9twsyumgZ4tGI2", {"admin"}));
    const AuthenticationProviderPtr authenticationProvider = StaticAuthenticationProvider(true, users);

    const InstanceBuilderPtr instanceBuilder = InstanceBuilder().setAuthenticationProvider(authenticationProvider);

    const InstancePtr instance = InstanceFromBuilder(instanceBuilder);
    instance.addStandardServers();

    // std::cin.get();
}

TEST_F(HowToAccessControl, ConnectWithUsername)
{
    auto createAndStartServer = []()
    {
        auto users = List<IUser>();
        users.pushBack(User("opendaq", "$2a$12$MmSt1b9YEHB5SpLNyikiD.37NvN23UA7zLH6Y98ob5HF0OsKH0IuO"));
        users.pushBack(User("root", "$2a$12$ceV7Q2j.vZcuz05hy1EkC.GHH8PIrv0D5wz7iLH9twsyumgZ4tGI2", {"admin"}));
        const AuthenticationProviderPtr authenticationProvider = StaticAuthenticationProvider(true, users);

        const InstanceBuilderPtr instanceBuilder = InstanceBuilder().setAuthenticationProvider(authenticationProvider);
        const InstancePtr instance = InstanceFromBuilder(instanceBuilder);
        instance.addStandardServers();
        return instance;
    };

    auto serverInstance = createAndStartServer();

    // Start of the example

    auto instance = Instance();

    auto config = instance.createDefaultAddDeviceConfig();
    PropertyObjectPtr generalConfig = config.getPropertyValue("General");

    generalConfig.setPropertyValue("Username", "opendaq");
    generalConfig.setPropertyValue("Password", "opendaq123");

    auto device = instance.addDevice("daq.nd://127.0.0.1", config);
    std::cout << "Connected to: " << device.getName() << std::endl;
}

TEST_F(HowToAccessControl, AddingProtectedObject)
{
    auto users = List<IUser>();
    users.pushBack(User("opendaq", "$2a$12$MmSt1b9YEHB5SpLNyikiD.37NvN23UA7zLH6Y98ob5HF0OsKH0IuO"));
    users.pushBack(User("root", "$2a$12$ceV7Q2j.vZcuz05hy1EkC.GHH8PIrv0D5wz7iLH9twsyumgZ4tGI2", {"admin"}));
    const AuthenticationProviderPtr authenticationProvider = StaticAuthenticationProvider(true, users);

    const InstanceBuilderPtr instanceBuilder = InstanceBuilder().setAuthenticationProvider(authenticationProvider);
    const InstancePtr instance = InstanceFromBuilder(instanceBuilder);

    instance.addStandardServers();
    instance.addDevice("daqref://device0");

    //std::cin.get();
}

TEST_F(HowToAccessControl, Allow)
{
    auto targetObject = PropertyObject();
    auto parentObject = PropertyObject();
    parentObject.addProperty(ObjectProperty("TargetObject", targetObject));

    auto parentPermissions = PermissionsBuilder().assign("everyone", PermissionMaskBuilder().read().write()).build();
    parentObject.getPermissionManager().setPermissions(parentPermissions);

    auto permissions = PermissionsBuilder().inherit(true).allow("everyone", PermissionMaskBuilder().execute()).build();
    targetObject.getPermissionManager().setPermissions(permissions);

    // target object permissions:
    // everyone: rwx

    auto user = User("", "");
    ASSERT_TRUE(targetObject.getPermissionManager().isAuthorized(user, Permission::Read));
    ASSERT_TRUE(targetObject.getPermissionManager().isAuthorized(user, Permission::Write));
    ASSERT_TRUE(targetObject.getPermissionManager().isAuthorized(user, Permission::Execute));
}

TEST_F(HowToAccessControl, Deny)
{
    auto targetObject = PropertyObject();
    auto parentObject = PropertyObject();
    parentObject.addProperty(ObjectProperty("TargetObject", targetObject));

    auto parentPermissions = PermissionsBuilder().allow("everyone", PermissionMaskBuilder().read().write().execute()).build();
    parentObject.getPermissionManager().setPermissions(parentPermissions);

    auto permissions = PermissionsBuilder().inherit(true).deny("everyone", PermissionMaskBuilder().execute()).build();
    targetObject.getPermissionManager().setPermissions(permissions);

    // target object permisisons:
    // everyone: rw

    auto user = User("", "");
    ASSERT_TRUE(targetObject.getPermissionManager().isAuthorized(user, Permission::Read));
    ASSERT_TRUE(targetObject.getPermissionManager().isAuthorized(user, Permission::Write));
    ASSERT_FALSE(targetObject.getPermissionManager().isAuthorized(user, Permission::Execute));
}

TEST_F(HowToAccessControl, Assign)
{
    auto targetObject = PropertyObject();
    auto parentObject = PropertyObject();
    parentObject.addProperty(ObjectProperty("TargetObject", targetObject));

    auto parentPermissions = PermissionsBuilder()
                                 .assign("everyone", PermissionMaskBuilder().read().write().execute())
                                 .assign("guest", PermissionMaskBuilder().read())
                                 .build();
    parentObject.getPermissionManager().setPermissions(parentPermissions);

    auto permissions = PermissionsBuilder().inherit(true).assign("everyone", PermissionMaskBuilder().read()).build();
    targetObject.getPermissionManager().setPermissions(permissions);

    // target object permisisons:
    // everyone: r
    // guest: r

    auto user = User("", "");
    ASSERT_TRUE(targetObject.getPermissionManager().isAuthorized(user, Permission::Read));
    ASSERT_FALSE(targetObject.getPermissionManager().isAuthorized(user, Permission::Write));
    ASSERT_FALSE(targetObject.getPermissionManager().isAuthorized(user, Permission::Execute));

    auto guest = User("guest", "guest", {"guest"});
    ASSERT_TRUE(targetObject.getPermissionManager().isAuthorized(guest, Permission::Read));
    ASSERT_FALSE(targetObject.getPermissionManager().isAuthorized(guest, Permission::Write));
    ASSERT_FALSE(targetObject.getPermissionManager().isAuthorized(guest, Permission::Execute));
}

END_NAMESPACE_OPENDAQ
