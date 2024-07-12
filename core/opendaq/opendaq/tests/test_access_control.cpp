#include <gtest/gtest.h>
#include <opendaq/instance_factory.h>
#include <opendaq/logger_factory.h>
#include <opendaq/mock/mock_device_module.h>
#include <opendaq/mock/mock_fb_module.h>
#include <opendaq/module_manager_factory.h>
#include <opendaq/scheduler_factory.h>
#include <coreobjects/user_factory.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>
#include <coreobjects/permissions_builder_factory.h>
#include <opendaq/search_filter_factory.h>
#include <coreobjects/permission_manager_factory.h>
#include <coreobjects/permission_mask_builder_factory.h>
#include <coreobjects/authentication_provider_factory.h>
#include <coreobjects/property_object_class_factory.h>
#include <coreobjects/permission_manager_impl.h>


using namespace daq;

class AccessControlTest : public testing::Test
{
public:
    InstancePtr createDevice()
    {
        const auto moduleManager = ModuleManager("[[none]]");
        auto logger = Logger();
        auto authenticationProvider = AuthenticationProvider();
        auto context = Context(Scheduler(logger, 1), logger, TypeManager(), moduleManager, authenticationProvider);

        const ModulePtr deviceModule(MockDeviceModule_Create(context));
        moduleManager.addModule(deviceModule);

        const ModulePtr fbModule(MockFunctionBlockModule_Create(context));
        moduleManager.addModule(fbModule);

        auto instance = InstanceCustom(context, "localInstance");
        instance.addDevice("daqmock://phys_device");
        instance.addFunctionBlock("mock_fb_uid");

        return instance;
    }
};


TEST_F(AccessControlTest, DefaultPermissions)
{
    const auto user = User("user", "psswordHash");

    auto device = createDevice();
    auto fb = device.findComponent("FB/mock_fb_uid_1");

    auto deviceManager = device.getPermissionManager();
    auto fbManager = fb.getPermissionManager();

    ASSERT_TRUE(deviceManager.isAuthorized(user, Permission::Read));
    ASSERT_TRUE(deviceManager.isAuthorized(user, Permission::Write));
    ASSERT_TRUE(deviceManager.isAuthorized(user, Permission::Execute));

    ASSERT_TRUE(fbManager.isAuthorized(user, Permission::Read));
    ASSERT_TRUE(fbManager.isAuthorized(user, Permission::Write));
    ASSERT_TRUE(fbManager.isAuthorized(user, Permission::Execute));
}

TEST_F(AccessControlTest, ComponentInherit)
{
    const auto user = User("user", "psswordHash", List<IString>("user", "guest"));

    auto device = createDevice();
    auto fb = device.findComponent("FB/mock_fb_uid_1");

    auto deviceManager = device.getPermissionManager();
    auto fbManager = fb.getPermissionManager();

    deviceManager.setPermissions(PermissionsBuilder().assign("guest", PermissionMaskBuilder().read().write()).build());
    fbManager.setPermissions(PermissionsBuilder()
                                 .inherit(true)
                                 .allow("guest", PermissionMaskBuilder().execute())
                                 .deny("guest", PermissionMaskBuilder().write())
                                 .build());

    ASSERT_TRUE(deviceManager.isAuthorized(user, Permission::Read));
    ASSERT_TRUE(deviceManager.isAuthorized(user, Permission::Write));
    ASSERT_FALSE(deviceManager.isAuthorized(user, Permission::Execute));

    ASSERT_TRUE(fbManager.isAuthorized(user, Permission::Read));
    ASSERT_FALSE(fbManager.isAuthorized(user, Permission::Write));
    ASSERT_TRUE(fbManager.isAuthorized(user, Permission::Execute));
}

TEST_F(AccessControlTest, ObjectPropertyInherit)
{
    const auto user = User("user", "psswordHash", List<IString>("user", "guest"));

    auto device = createDevice();

    auto object = PropertyObject();
    object.addProperty(IntProperty("TestInt", 10));
    device.addProperty(ObjectProperty("TestObject", object));

    auto deviceManager = device.getPermissionManager();
    auto objectManager = object.getPermissionManager();

    deviceManager.setPermissions(PermissionsBuilder().assign("guest", PermissionMaskBuilder().read().write()).build());
    objectManager.setPermissions(PermissionsBuilder()
                                     .inherit(true)
                                     .allow("guest", PermissionMaskBuilder().execute())
                                     .deny("guest", PermissionMaskBuilder().write())
                                     .build());

    ASSERT_TRUE(deviceManager.isAuthorized(user, Permission::Read));
    ASSERT_TRUE(deviceManager.isAuthorized(user, Permission::Write));
    ASSERT_FALSE(deviceManager.isAuthorized(user, Permission::Execute));

    ASSERT_TRUE(objectManager.isAuthorized(user, Permission::Read));
    ASSERT_FALSE(objectManager.isAuthorized(user, Permission::Write));
    ASSERT_TRUE(objectManager.isAuthorized(user, Permission::Execute));
}

TEST_F(AccessControlTest, ComponentAndObjectInherit)
{
    const auto user = User("user", "psswordHash", List<IString>("user", "guest"));

    auto device = createDevice();
    auto fb = device.findComponent("FB/mock_fb_uid_1");

    auto object = PropertyObject();
    object.addProperty(IntProperty("TestInt", 10));
    fb.addProperty(ObjectProperty("TestObject", object));

    auto deviceManager = device.getPermissionManager();
    auto fbManager = fb.getPermissionManager();
    auto objectManager = object.getPermissionManager();

    deviceManager.setPermissions(PermissionsBuilder().assign("guest", PermissionMaskBuilder().read()).build());
    fbManager.setPermissions(PermissionsBuilder().inherit(true).allow("guest", PermissionMaskBuilder().write()).build());
    objectManager.setPermissions(PermissionsBuilder().inherit(true).allow("guest", PermissionMaskBuilder().execute()).build());

    ASSERT_TRUE(deviceManager.isAuthorized(user, Permission::Read));
    ASSERT_FALSE(deviceManager.isAuthorized(user, Permission::Write));
    ASSERT_FALSE(deviceManager.isAuthorized(user, Permission::Execute));

    ASSERT_TRUE(fbManager.isAuthorized(user, Permission::Read));
    ASSERT_TRUE(fbManager.isAuthorized(user, Permission::Write));
    ASSERT_FALSE(fbManager.isAuthorized(user, Permission::Execute));

    ASSERT_TRUE(objectManager.isAuthorized(user, Permission::Read));
    ASSERT_TRUE(objectManager.isAuthorized(user, Permission::Write));
    ASSERT_TRUE(objectManager.isAuthorized(user, Permission::Execute));
}

TEST_F(AccessControlTest, DefaultValueClone)
{
    const auto user = User("user", "psswordHash", List<IString>("user", "guest"));

    auto childObject = PropertyObject();
    childObject.getPermissionManager().setPermissions(
        PermissionsBuilder().inherit(true).allow("guest", PermissionMaskBuilder().write()).build());

    auto object = PropertyObject();
    object.addProperty(ObjectProperty("TestObject", childObject));
    object.getPermissionManager().setPermissions(PermissionsBuilder().allow("guest", PermissionMaskBuilder().read()).build());

    auto defObj = object.getProperty("TestObject").getDefaultValue().asPtr<IPropertyObject>();
    auto valueObj = object.getPropertyValue("TestObject").asPtr<IPropertyObject>();

    ASSERT_NE(defObj.getPermissionManager(), valueObj.getPermissionManager());

    ASSERT_TRUE(defObj.getPermissionManager().isAuthorized(user, Permission::Read));
    ASSERT_TRUE(defObj.getPermissionManager().isAuthorized(user, Permission::Write));
    ASSERT_FALSE(defObj.getPermissionManager().isAuthorized(user, Permission::Execute));

    ASSERT_TRUE(valueObj.getPermissionManager().isAuthorized(user, Permission::Read));
    ASSERT_TRUE(valueObj.getPermissionManager().isAuthorized(user, Permission::Write));
    ASSERT_FALSE(valueObj.getPermissionManager().isAuthorized(user, Permission::Execute));
}

TEST_F(AccessControlTest, ClassDefaultValueClone)
{
    const auto user = User("user", "psswordHash", List<IString>("user", "guest"));

    auto typeManager = TypeManager();

    auto childObject = PropertyObject();
    childObject.getPermissionManager().setPermissions(
        PermissionsBuilder().inherit(true).allow("guest", PermissionMaskBuilder().write()).build());

    auto cls = PropertyObjectClassBuilder("TestClass").addProperty(ObjectProperty("TestObject", childObject)).build();
    typeManager.addType(cls);

    auto object = PropertyObject(typeManager, "TestClass");
    object.getPermissionManager().setPermissions(PermissionsBuilder().allow("guest", PermissionMaskBuilder().read()).build());

    auto defObj = object.getProperty("TestObject").getDefaultValue().asPtr<IPropertyObject>();
    auto valueObj = object.getPropertyValue("TestObject").asPtr<IPropertyObject>();

    ASSERT_NE(childObject.getPermissionManager(), defObj.getPermissionManager());
    ASSERT_NE(childObject.getPermissionManager(), valueObj.getPermissionManager());
    ASSERT_NE(defObj.getPermissionManager(), valueObj.getPermissionManager());

    ASSERT_TRUE(defObj.getPermissionManager().isAuthorized(user, Permission::Read));
    ASSERT_TRUE(defObj.getPermissionManager().isAuthorized(user, Permission::Write));
    ASSERT_FALSE(defObj.getPermissionManager().isAuthorized(user, Permission::Execute));

    ASSERT_TRUE(valueObj.getPermissionManager().isAuthorized(user, Permission::Read));
    ASSERT_TRUE(valueObj.getPermissionManager().isAuthorized(user, Permission::Write));
    ASSERT_FALSE(valueObj.getPermissionManager().isAuthorized(user, Permission::Execute));
}

TEST_F(AccessControlTest, RemoveChildPermissionManager)
{
    auto typeManager = TypeManager();

    auto childObject = PropertyObject();
    auto cls = PropertyObjectClassBuilder("TestClass").addProperty(ObjectProperty("TestObject", childObject)).build();
    typeManager.addType(cls);

    auto object = PropertyObject(typeManager, "TestClass");

    WeakRefPtr<IPermissionManager> permissionManager;

    {
        auto prop = object.getProperty("TestObject");
        permissionManager = prop.getDefaultValue().asPtr<IPropertyObject>().getPermissionManager();
        ASSERT_TRUE(permissionManager.getRef().assigned());
    }

    ASSERT_FALSE(permissionManager.getRef().assigned());
}
