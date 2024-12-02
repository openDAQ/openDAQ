#include "test_helpers.h"
#include <gtest/gtest.h>
#include <opendaq/module_impl.h>
#include <opendaq/instance_factory.h>
#include <opendaq/device_type_factory.h>
#include <opendaq/function_block_type_factory.h>
#include <opendaq/server_type_factory.h>
#include <opendaq/streaming_type_factory.h>

using ModuleCallbacksTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

class TestModuleImpl final : public Module
{
public:

    static PropertyObjectPtr CreatePartialConfigObject()
    {
        PropertyObjectPtr partial = PropertyObject();
        auto childObj1 = PropertyObject();
        childObj1.addProperty(StringProperty("foo", "bar"));
        partial.addProperty(ObjectProperty("child", childObj1));
        return partial;
    }

    static PropertyObjectPtr CreateInvalidConfigObject()
    {
        PropertyObjectPtr invalid = PropertyObject();
        auto childObj1 = PropertyObject();
        invalid.addProperty(BoolProperty("foo", false));
        childObj1.addProperty(IntProperty("foo", 1));
        childObj1.addProperty(StringProperty("invalid", "bar"));
        invalid.addProperty(ObjectProperty("invalidchild", childObj1));
        return invalid;
    }

    TestModuleImpl()
        : Module("name", VersionInfo(1, 0, 0), NullContext(), "id")
    {
        obj = PropertyObject();
        obj.addProperty(StringProperty("foo", "bar"));

        auto childObj1 = PropertyObject();
        childObj1.addProperty(StringProperty("foo", "bar"));

        auto childObj2 = PropertyObject();
        childObj2.addProperty(StringProperty("foo", "bar"));

        childObj1.addProperty(ObjectProperty("child", childObj2));
        obj.addProperty(ObjectProperty("child", childObj1));
    }

    void configValid(const PropertyObjectPtr& config, const PropertyObjectPtr& reference) 
    {
        ASSERT_EQ(config.getAllProperties().getCount(), reference.getAllProperties().getCount());
        for (const auto& prop : reference.getAllProperties())
        {
            const auto name = prop.getName();
            ASSERT_TRUE(config.hasProperty(name));
            if (prop.getValueType() == ctObject)
                configValid(config.getPropertyValue(name), reference.getPropertyValue(name));
        }
    }

    DictPtr<IString, IDeviceType> onGetAvailableDeviceTypes() override
    {
        return Dict<IString, IDeviceType>({{"id", DeviceType("id", "name", "desc", "daqtest", obj)}});
    }

    DevicePtr onCreateDevice(const StringPtr& connectionString, const ComponentPtr& parent, const PropertyObjectPtr& config) override
    {
        configValid(config, obj);
        return {};
    }

    DictPtr<IString, IFunctionBlockType> onGetAvailableFunctionBlockTypes() override
    {
        return Dict<IString, IServerType>({{"id", ServerType("id", "name", "desc", obj)}});
    }

    FunctionBlockPtr onCreateFunctionBlock(const StringPtr& id, const ComponentPtr& parent, const StringPtr& localId, const PropertyObjectPtr& config) override
    {
        configValid(config, obj);
        return {};
    }

    DictPtr<IString, IServerType> onGetAvailableServerTypes() override
    {
        return Dict<IString, IServerType>({{"id", ServerType("id", "name", "desc", obj)}});
    }

    ServerPtr onCreateServer(const StringPtr& serverType, const PropertyObjectPtr& serverConfig, const DevicePtr& rootDevice) override
    {
        configValid(serverConfig, obj);
        return {};
    }

    DictPtr<IString, IStreamingType> onGetAvailableStreamingTypes() override
    {
        return Dict<IString, IStreamingType>({{"id", StreamingType("id", "name", "desc", "daqtest", obj)}});
    }

    StreamingPtr onCreateStreaming(const StringPtr& connectionString, const PropertyObjectPtr& config) override
    {
        configValid(config, obj);
        return {};
    }

    PropertyObjectPtr obj;
};


TEST_F(ModuleCallbacksTest, TestDeviceCallback)
{
    const auto _module = createWithImplementation<IModule, TestModuleImpl>();

    ASSERT_FALSE(_module.createDevice("daqtest://", nullptr, nullptr).assigned());
    ASSERT_FALSE(_module.createDevice("daqtest://", nullptr, PropertyObject()).assigned());
    ASSERT_FALSE(_module.createDevice("daqtest://", nullptr, TestModuleImpl::CreatePartialConfigObject()).assigned());
    ASSERT_FALSE(_module.createDevice("daqtest://", nullptr, TestModuleImpl::CreateInvalidConfigObject()).assigned());
}

TEST_F(ModuleCallbacksTest, TestFBCallback)
{
    const auto _module = createWithImplementation<IModule, TestModuleImpl>();
    ASSERT_FALSE(_module.createFunctionBlock("id", nullptr, "", nullptr).assigned());
    ASSERT_FALSE(_module.createFunctionBlock("id", nullptr, "", PropertyObject()).assigned());
    ASSERT_FALSE(_module.createFunctionBlock("id", nullptr, "", TestModuleImpl::CreatePartialConfigObject()).assigned());
    ASSERT_FALSE(_module.createFunctionBlock("id", nullptr, "", TestModuleImpl::CreateInvalidConfigObject()).assigned());
}

TEST_F(ModuleCallbacksTest, TestServerCallback)
{
    const auto _module = createWithImplementation<IModule, TestModuleImpl>();
    ASSERT_FALSE(_module.createServer("id", nullptr, nullptr).assigned());
    ASSERT_FALSE(_module.createServer("id", nullptr, PropertyObject()).assigned());
    ASSERT_FALSE(_module.createServer("id", nullptr, TestModuleImpl::CreatePartialConfigObject()).assigned());
    ASSERT_FALSE(_module.createServer("id", nullptr, TestModuleImpl::CreateInvalidConfigObject()).assigned());
}

TEST_F(ModuleCallbacksTest, TestStreamingCallback)
{
    const auto _module = createWithImplementation<IModule, TestModuleImpl>();
    ASSERT_FALSE(_module.createStreaming("daqtest://", nullptr).assigned());
    ASSERT_FALSE(_module.createStreaming("daqtest://", PropertyObject()).assigned());
    ASSERT_FALSE(_module.createStreaming("daqtest://", TestModuleImpl::CreatePartialConfigObject()).assigned());
    ASSERT_FALSE(_module.createStreaming("daqtest://", TestModuleImpl::CreateInvalidConfigObject()).assigned());
}
END_NAMESPACE_OPENDAQ
