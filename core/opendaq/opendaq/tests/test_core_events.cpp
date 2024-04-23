#include <opendaq/component_factory.h>
#include <opendaq/context_factory.h>
#include <coreobjects/property_factory.h>
#include <gtest/gtest.h>
#include <coreobjects/property_object_internal_ptr.h>
#include <opendaq/mock/mock_fb_module.h>
#include <opendaq/mock/mock_device_module.h>
#include <opendaq/instance_factory.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/signal_config_ptr.h>
#include <opendaq/folder_config_ptr.h>
#include <opendaq/input_port_factory.h>
#include <opendaq/signal_factory.h>
#include <opendaq/tags_private_ptr.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_object_protected_ptr.h>
#include <opendaq/component_status_container_private_ptr.h>
#include <opendaq/mock/mock_physical_device.h>
#include <opendaq/device_domain_factory.h>
#include <coreobjects/authentication_provider_factory.h>

using namespace daq;

class CoreEventTest : public testing::Test
{
public:
    void SetUp() override
    {
        const auto logger = Logger();
        const auto moduleManager = ModuleManager("[[none]]");
        const auto typeManager = TypeManager();
        const auto authenticationProvider = AuthenticationProvider();
        const auto context = Context(nullptr, logger, typeManager, moduleManager, authenticationProvider);

        const ModulePtr deviceModule(MockDeviceModule_Create(context));
        moduleManager.addModule(deviceModule);

        const ModulePtr fbModule(MockFunctionBlockModule_Create(context));
        moduleManager.addModule(fbModule);

        instance = InstanceCustom(context, "test");
        instance.addDevice("mock_phys_device");
        instance.addFunctionBlock("mock_fb_uid");
    }

    Event<ComponentPtr, CoreEventArgsPtr> getOnCoreEvent() const
    {
        return instance.getContext().getOnCoreEvent();
    }

    InstancePtr instance;
};

TEST_F(CoreEventTest, PropertyChanged)
{
    const auto context = NullContext();
    const auto component = Component(context, nullptr, "comp");
    component.addProperty(StringProperty("string", "foo"));
    int callCount = 0;

    component.asPtrOrNull<IPropertyObjectInternal>().enableCoreEventTrigger();
    context.getOnCoreEvent() += [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
    {
        ASSERT_EQ(args.getEventId(), static_cast<Int>(CoreEventId::PropertyValueChanged));
        ASSERT_EQ(args.getEventName(), "PropertyValueChanged");
        ASSERT_EQ(comp, component);
        ASSERT_TRUE(args.getParameters().hasKey("Name"));
        ASSERT_TRUE(args.getParameters().hasKey("Value"));
        ASSERT_EQ(comp, args.getParameters().get("Owner"));

        callCount++;
    };

    component.setPropertyValue("string", "bar");
    component.setPropertyValue("string", "foo");
    component.clearPropertyValue("string");

    ASSERT_EQ(callCount, 3);
}

TEST_F(CoreEventTest, PropertyChangedNested)
{
    const auto context = NullContext();
    const auto component = Component(context, nullptr, "comp");

    const auto defaultObj = PropertyObject();
    const auto obj2 = PropertyObject();
    defaultObj.addProperty(StringProperty("string", "foo"));
    obj2.addProperty(StringProperty("string", "foo"));

    component.addProperty(ObjectProperty("obj1", defaultObj));
    const PropertyObjectPtr obj1 = component.getPropertyValue("obj1");

    component.addProperty(ObjectProperty("obj2"));
    component.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue("obj2", obj2);

    int callCount = 0;

    component.asPtrOrNull<IPropertyObjectInternal>().enableCoreEventTrigger();
    context.getOnCoreEvent() += [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
    {
        ASSERT_EQ(args.getEventId(), static_cast<int>(CoreEventId::PropertyValueChanged));
        ASSERT_EQ(args.getEventName(), "PropertyValueChanged");
        ASSERT_EQ(comp, component);
        ASSERT_TRUE(args.getParameters().hasKey("Name"));
        ASSERT_TRUE(args.getParameters().hasKey("Value"));

        if (callCount == 0)
        {
            ASSERT_EQ(obj1, args.getParameters().get("Owner"));
        }
        else
        {
            ASSERT_EQ(obj2, args.getParameters().get("Owner"));
            ASSERT_EQ("obj2", args.getParameters().get("Path"));
        }

        callCount++;
    };

    obj1.setPropertyValue("string", "bar");
    obj2.setPropertyValue("string", "bar");

    ASSERT_EQ(callCount, 2);
}

TEST_F(CoreEventTest, MultipleLevelNestedObjectPath)
{
    const auto context = NullContext();
    const auto component = Component(context, nullptr, "comp");

    const auto defaultObj = PropertyObject();
    const auto defaultObj1 = PropertyObject();
    const auto defaultObj2 = PropertyObject();
    defaultObj2.addProperty(IntProperty("IntVal", 5));
    
    defaultObj1.addProperty(ObjectProperty("obj3", defaultObj2));
    defaultObj.addProperty(ObjectProperty("obj2", defaultObj1));
    component.addProperty(ObjectProperty("obj1", defaultObj));

    int callCount = 0;
    component.asPtrOrNull<IPropertyObjectInternal>().enableCoreEventTrigger();
    context.getOnCoreEvent() += [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& args)
    {
        ASSERT_EQ("obj1.obj2.obj3", args.getParameters().get("Path"));
        callCount++;
    };

    const PropertyObjectPtr obj = component.getPropertyValue("obj1.obj2.obj3");
    component.setPropertyValue("obj1.obj2.obj3.IntVal", 10);
    obj.addProperty(StringProperty("test", "test"));
    obj.removeProperty("test");
    obj.beginUpdate();
    obj.endUpdate();

    ASSERT_EQ(callCount, 3);
}

TEST_F(CoreEventTest, NestedAddAfterCoreEventEnable)
{
    const auto context = NullContext();
    const auto component = Component(context, nullptr, "comp");

    const auto defaultObj = PropertyObject();
    const auto defaultObj1 = PropertyObject();
    const auto defaultObj2 = PropertyObject();

    defaultObj2.addProperty(IntProperty("IntVal", 5));
    defaultObj1.addProperty(ObjectProperty("obj3", defaultObj2));

    component.asPtrOrNull<IPropertyObjectInternal>().enableCoreEventTrigger();

    defaultObj.addProperty(ObjectProperty("obj2", defaultObj1));
    component.addProperty(ObjectProperty("obj1", defaultObj));

    int callCount = 0;
    context.getOnCoreEvent() += [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& args)
    {
        ASSERT_EQ("obj1.obj2.obj3", args.getParameters().get("Path"));
        callCount++;
    };

    const PropertyObjectPtr obj = component.getPropertyValue("obj1.obj2.obj3");
    component.setPropertyValue("obj1.obj2.obj3.IntVal", 10);
    obj.addProperty(StringProperty("test", "test"));
    obj.removeProperty("test");
    obj.beginUpdate();
    obj.endUpdate();

    ASSERT_EQ(callCount, 3);
}

TEST_F(CoreEventTest, NestedObjDisableTrigger)
{
    const auto context = NullContext();
    const auto component = Component(context, nullptr, "comp");

    const auto defaultObj = PropertyObject();
    const auto obj2 = PropertyObject();
    defaultObj.addProperty(StringProperty("string", "foo"));
    obj2.addProperty(StringProperty("string", "foo"));

    component.addProperty(ObjectProperty("obj1", defaultObj));
    const PropertyObjectPtr obj1 = component.getPropertyValue("obj1");

    component.addProperty(ObjectProperty("obj2"));
    component.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue("obj2", obj2);

    int callCount = 0;

    context.getOnCoreEvent() += [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& /*args*/)
    {
        callCount++;
    };

    obj1.setPropertyValue("string", "bar");
    obj2.setPropertyValue("string", "foo");
    
    component.asPtrOrNull<IPropertyObjectInternal>().enableCoreEventTrigger();
    obj1.setPropertyValue("string", "foo");
    obj2.setPropertyValue("string", "bar");
    
    component.asPtrOrNull<IPropertyObjectInternal>().disableCoreEventTrigger();
    obj1.setPropertyValue("string", "bar");
    obj2.setPropertyValue("string", "foo");

    ASSERT_EQ(callCount, 2);
}

TEST_F(CoreEventTest, PropertyChangedWithInternalEvent)
{
    const auto context = NullContext();
    const auto component = Component(context, nullptr, "comp");
    component.addProperty(StringProperty("string", "foo"));

    int callCount = 0;
    component.asPtrOrNull<IPropertyObjectInternal>().enableCoreEventTrigger();
    component.getOnPropertyValueWrite("string") +=
        [&](const PropertyObjectPtr& /*obj*/, const PropertyValueEventArgsPtr& args)
        {
            args.setValue("override_" + std::to_string(callCount));
        };

    context.getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventId(), static_cast<int>(CoreEventId::PropertyValueChanged));
            ASSERT_EQ(args.getEventName(), "PropertyValueChanged");
            ASSERT_EQ(comp, component);
            ASSERT_TRUE(args.getParameters().hasKey("Name"));
            ASSERT_TRUE(args.getParameters().hasKey("Value"));
            ASSERT_EQ(args.getParameters().get("Value"), "override_" + std::to_string(callCount));
            ASSERT_EQ(comp, args.getParameters().get("Owner"));

            callCount++;
        };

    component.setPropertyValue("string", "bar");
    component.setPropertyValue("string", "foo");
    component.clearPropertyValue("string");

    ASSERT_EQ(callCount, 3);
}

TEST_F(CoreEventTest, EndUpdateEventSerilizer)
{
    const auto context = NullContext();
    const auto component = Component(context, nullptr, "comp");
    component.addProperty(StringProperty("string", "foo"));
    component.addProperty(IntProperty("int", 0));
    component.addProperty(FloatProperty("float", 1.123));
    
    component.setPropertyValue("string", "bar");
    component.setPropertyValue("int", 1);

    const auto serializer = JsonSerializer();
    component.serialize(serializer);
    const auto out = serializer.getOutput();
    
    component.clearPropertyValue("string");
    component.clearPropertyValue("int");

    int updateCount = 0;
    
    component.asPtrOrNull<IPropertyObjectInternal>().enableCoreEventTrigger();
    context.getOnCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventName(), "ComponentUpdateEnd");
            updateCount++;
        };

    const auto deserializer = JsonDeserializer();
    deserializer.update(component, serializer.getOutput());

    ASSERT_EQ(updateCount, 1);
}

TEST_F(CoreEventTest, EndUpdateEventBeginEnd)
{
    const auto context = NullContext();
    const auto component = Component(context, nullptr, "comp");
    component.addProperty(StringProperty("string", "foo"));
    component.addProperty(IntProperty("int", 0));
    component.addProperty(FloatProperty("float", 1.123));

    int propChangeCount = 0;
    int updateCount = 0;
    int otherCount = 0;
    
    component.asPtrOrNull<IPropertyObjectInternal>().enableCoreEventTrigger();
    context.getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
        {
            DictPtr<IString, IBaseObject> updated;
            switch (static_cast<CoreEventId>(args.getEventId()))
            {
                case CoreEventId::PropertyValueChanged:
                    propChangeCount++;
                    break;
                case CoreEventId::PropertyObjectUpdateEnd:
                    updateCount++;
                    updated = args.getParameters().get("UpdatedProperties");
                    ASSERT_EQ(updated.getCount(), 2u);
                    ASSERT_EQ(args.getEventName(), "PropertyObjectUpdateEnd");
                    ASSERT_EQ(comp, args.getParameters().get("Owner"));
                    break;
                default:
                    otherCount++;
                    break;
            }            
        };

    component.beginUpdate();
    component.setPropertyValue("string", "bar");
    component.setPropertyValue("int", 1);
    component.endUpdate();
    
    component.beginUpdate();
    component.clearPropertyValue("string");
    component.clearPropertyValue("int");
    component.endUpdate();

    ASSERT_EQ(propChangeCount, 0);
    ASSERT_EQ(updateCount, 2);
    ASSERT_EQ(otherCount, 0);
}

TEST_F(CoreEventTest, PropertyAddedEvent)
{
    const auto context = NullContext();
    const auto component = Component(context, nullptr, "comp");

    int addCount = 0;
    
    component.asPtrOrNull<IPropertyObjectInternal>().enableCoreEventTrigger();
    context.getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventId(), static_cast<int>(CoreEventId::PropertyAdded));
            ASSERT_EQ(args.getEventName(), "PropertyAdded");
            ASSERT_TRUE(args.getParameters().hasKey("Property"));
            ASSERT_EQ(comp, args.getParameters().get("Owner"));
            addCount++;
        };

    component.addProperty(StringProperty("string1", "foo"));
    component.addProperty(StringProperty("string2", "bar"));
    component.addProperty(FloatProperty("float", 1.123));
    ASSERT_EQ(addCount, 3);
}

TEST_F(CoreEventTest, PropertyAddedNested)
{
    const auto context = NullContext();
    const auto component = Component(context, nullptr, "comp");

    const auto defaultObj = PropertyObject();
    const auto obj2 = PropertyObject();

    component.addProperty(ObjectProperty("obj1", defaultObj));
    const PropertyObjectPtr obj1 = component.getPropertyValue("obj1");

    component.addProperty(ObjectProperty("obj2"));
    component.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue("obj2", obj2);

    int addCount = 0;
    
    component.asPtrOrNull<IPropertyObjectInternal>().enableCoreEventTrigger();
    context.getOnCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventId(), static_cast<int>(CoreEventId::PropertyAdded));
            ASSERT_EQ(args.getEventName(), "PropertyAdded");
            ASSERT_TRUE(args.getParameters().hasKey("Property"));

            if (addCount == 0)
                ASSERT_EQ(obj1, args.getParameters().get("Owner"));
            else
                ASSERT_EQ(obj2, args.getParameters().get("Owner"));

            addCount++;
        };
    
    obj1.addProperty(StringProperty("string", "foo"));
    obj2.addProperty(StringProperty("string", "foo"));
    ASSERT_EQ(addCount, 2);
}

TEST_F(CoreEventTest, PropertyRemovedEvent)
{
    const auto context = NullContext();
    const auto component = Component(context, nullptr, "comp");
    
    component.addProperty(StringProperty("string1", "foo"));
    component.addProperty(StringProperty("string2", "bar"));
    component.addProperty(FloatProperty("float", 1.123));

    int removeCount = 0;
    
    component.asPtrOrNull<IPropertyObjectInternal>().enableCoreEventTrigger();
    context.getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventId(), static_cast<int>(CoreEventId::PropertyRemoved));
            ASSERT_EQ(args.getEventName(), "PropertyRemoved");
            ASSERT_TRUE(args.getParameters().hasKey("Name"));
            ASSERT_EQ(comp, args.getParameters().get("Owner"));
            ASSERT_EQ(comp, component);
            removeCount++;
        };

    component.removeProperty("string1");
    component.removeProperty("string2");
    component.removeProperty("float");
    ASSERT_EQ(removeCount, 3);
}

TEST_F(CoreEventTest, PropertyRemovedNested)
{
    const auto context = NullContext();
    const auto component = Component(context, nullptr, "comp");

    const auto defaultObj = PropertyObject();
    const auto obj2 = PropertyObject();
    defaultObj.addProperty(StringProperty("string", "foo"));
    obj2.addProperty(StringProperty("string", "foo"));

    component.addProperty(ObjectProperty("obj1", defaultObj));
    const PropertyObjectPtr obj1 = component.getPropertyValue("obj1");

    component.addProperty(ObjectProperty("obj2"));
    component.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue("obj2", obj2);

    int removeCount = 0;
    
    component.asPtrOrNull<IPropertyObjectInternal>().enableCoreEventTrigger();
    context.getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventId(), static_cast<int>(CoreEventId::PropertyRemoved));
            ASSERT_EQ(args.getEventName(), "PropertyRemoved");
            ASSERT_TRUE(args.getParameters().hasKey("Name"));
            if (removeCount == 0)
                ASSERT_EQ(obj1, args.getParameters().get("Owner"));
            else
                ASSERT_EQ(obj2, args.getParameters().get("Owner"));
            ASSERT_EQ(comp, component);
            removeCount++;
        };

    obj1.removeProperty("string");
    obj2.removeProperty("string");

    ASSERT_EQ(removeCount, 2);
}

TEST_F(CoreEventTest, PropertyAddedMuted)
{
    const auto context = NullContext();
    const auto component = Component(context, nullptr, "comp");
    int addCount = 0;
    context.getOnCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& /*args*/)
        {
            addCount++;
        };

    component.addProperty(StringProperty("string1", "foo"));
    component.addProperty(StringProperty("string2", "bar"));
    component.addProperty(FloatProperty("float", 1.123));

    ASSERT_EQ(addCount, 0);
}

TEST_F(CoreEventTest, PropertyRemovedMuted)
{
    const auto context = NullContext();
    const auto component = Component(context, nullptr, "comp");

    component.addProperty(StringProperty("string1", "foo"));
    component.addProperty(StringProperty("string2", "bar"));
    component.addProperty(FloatProperty("float", 1.123));

    int removeCount = 0;
    context.getOnCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& /*args*/)
        {
            removeCount++;
        };

    component.removeProperty("string1");
    component.removeProperty("string2");
    component.removeProperty("float");

    ASSERT_EQ(removeCount, 0);
}

TEST_F(CoreEventTest, PropertyValueChangedMuted)
{
    const auto context = NullContext();
    const auto component = Component(context, nullptr, "comp");

    component.addProperty(StringProperty("string1", "foo"));

    int changeCount = 0;
    context.getOnCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& /*args*/)
        {
            changeCount++;
        };

    component.setPropertyValue("string1", "bar");
    component.setPropertyValue("string1", "foo");
    component.setPropertyValue("string1", "bar");

    ASSERT_EQ(changeCount, 0);   
}

TEST_F(CoreEventTest, PropertyObjectUpdatedMuted)
{
    const auto context = NullContext();
    const auto component = Component(context, nullptr, "comp");
    int updateCount = 0;

    context.getOnCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& /*args*/)
        {
            updateCount++;
        };

    component.beginUpdate();
    component.endUpdate();
    
    component.beginUpdate();
    component.endUpdate();

    ASSERT_EQ(updateCount, 0);
}

TEST_F(CoreEventTest, recursiveMute)
{
    const auto device = instance.getRootDevice();
    device.asPtr<IPropertyObjectInternal>().disableCoreEventTrigger();

    int callCount = 0;
    getOnCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& /*args*/)
        {
            callCount++;
        };

    device.addProperty(StringProperty("string1", "string1"));
    device.addProperty(StringProperty("string2", "string2"));
    device.setPropertyValue("string1", "foo");
    device.setPropertyValue("string2", "bar");

    ASSERT_EQ(callCount, 0);
}

TEST_F(CoreEventTest, recursiveUnmute)
{
    const auto device = instance.getRootDevice();
    device.asPtr<IPropertyObjectInternal>().disableCoreEventTrigger();
    device.asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();

    int callCount = 0;
    getOnCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& /*args*/)
        {
            callCount++;
        };

    device.addProperty(StringProperty("string1", "string1"));
    device.addProperty(StringProperty("string2", "string2"));
    device.setPropertyValue("string1", "foo");
    device.setPropertyValue("string2", "bar");

    ASSERT_EQ(callCount, 4);
}

TEST_F(CoreEventTest, SignalConnected)
{
    int connectCount = 0;
    const auto sig = instance.getSignalsRecursive()[0];
    const auto ip = instance.getFunctionBlocks()[0].getInputPorts()[0];

    getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventId(), static_cast<int>(CoreEventId::SignalConnected));
            ASSERT_EQ(args.getEventName(), "SignalConnected");
            ASSERT_TRUE(args.getParameters().hasKey("Signal"));
            ASSERT_EQ(args.getParameters().get("Signal"), sig);
            ASSERT_EQ(comp, instance.getFunctionBlocks()[0].getInputPorts()[0]);
            connectCount++;
        };


    ip.connect(sig);
    ip.connect(sig);
    ip.connect(sig);
    
    ASSERT_EQ(connectCount, 3);
}

TEST_F(CoreEventTest, SignalConnectedMuted)
{
    int connectCount = 0;
    getOnCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& /*args*/)
        {
            connectCount++;
        };

    const auto sig = instance.getSignalsRecursive()[0];
    const auto ip = instance.getFunctionBlocks()[0].getInputPorts()[0];

    ip.asPtr<IPropertyObjectInternal>().disableCoreEventTrigger();
    ip.connect(sig);

    ip.asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();
    ip.connect(sig);

    instance.getRootDevice().asPtr<IPropertyObjectInternal>().disableCoreEventTrigger();
    ip.connect(sig);
    
    ASSERT_EQ(connectCount, 1);
}

TEST_F(CoreEventTest, SignalDisconnected)
{
    const auto sig = instance.getSignalsRecursive()[0];
    const auto ip = instance.getFunctionBlocks()[0].getInputPorts()[0];

    ip.connect(sig);
    ip.connect(sig);
    ip.connect(sig);

    int disconnectCount = 0;
    getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventId(), static_cast<int>(CoreEventId::SignalDisconnected));
            ASSERT_EQ(args.getEventName(), "SignalDisconnected");
            ASSERT_EQ(comp, instance.getFunctionBlocks()[0].getInputPorts()[0]);
            disconnectCount++;
        };

    ip.disconnect();
    ip.disconnect();
    
    ASSERT_EQ(disconnectCount, 1);
}

TEST_F(CoreEventTest, SignalDisconnectedMuted)
{
    const auto sig = instance.getSignalsRecursive()[0];
    const auto ip = instance.getFunctionBlocks()[0].getInputPorts()[0];

    ip.connect(sig);
    ip.connect(sig);
    ip.connect(sig);

    int disconnectCount = 0;
    getOnCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& /*args*/)
        {
            disconnectCount++;
        };
    
    ip.asPtr<IPropertyObjectInternal>().disableCoreEventTrigger();

    ip.disconnect();
    ip.disconnect();
    
    ASSERT_EQ(disconnectCount, 0);
}

TEST_F(CoreEventTest, DescriptorChanged)
{
    const auto sig = instance.getSignalsRecursive()[0].asPtr<ISignalConfig>();
    const auto desc = DataDescriptorBuilder().setSampleType(SampleType::Float32).setRule(ExplicitDataRule()).build();

    int changeCount = 0;
    getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventId(), static_cast<int>(CoreEventId::DataDescriptorChanged));
            ASSERT_EQ(args.getEventName(), "DataDescriptorChanged");
            ASSERT_TRUE(args.getParameters().hasKey("DataDescriptor"));
            ASSERT_EQ(comp, instance.getSignalsRecursive()[0]);
            ASSERT_EQ(args.getParameters().get("DataDescriptor"), desc);
            changeCount++;
        };

    sig.setDescriptor(desc);
    sig.setDescriptor(desc);
    sig.setDescriptor(desc);

    ASSERT_EQ(changeCount, 3);
}

TEST_F(CoreEventTest, DescriptorChangedMuted)
{
    const auto sig = instance.getSignalsRecursive()[0].asPtr<ISignalConfig>();
    const auto desc = DataDescriptorBuilder().setSampleType(SampleType::Float32).setRule(ExplicitDataRule()).build();

    int changeCount = 0;
    getOnCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& /*args*/)
        {
            changeCount++;
        };

    sig.asPtr<IPropertyObjectInternal>().disableCoreEventTrigger();
    sig.setDescriptor(desc);
    
    sig.asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();
    sig.setDescriptor(desc);

    instance.getRootDevice().asPtr<IPropertyObjectInternal>().disableCoreEventTrigger();
    sig.setDescriptor(desc);

    ASSERT_EQ(changeCount, 1);
}

TEST_F(CoreEventTest, ComponentUpdated)
{
    const auto device = instance.getRootDevice();
    instance.removeFunctionBlock(instance.getFunctionBlocks()[0]); // Workaround due to save/load fb update  bug

    const auto config = device.saveConfiguration();

    int updateCount = 0;
    getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventId(), static_cast<int>(CoreEventId::ComponentUpdateEnd));
            ASSERT_EQ(args.getEventName(), "ComponentUpdateEnd");
            ASSERT_EQ(comp, instance.getRootDevice());
            updateCount++;
        };

    device.loadConfiguration(config);
    device.loadConfiguration(config);
    device.loadConfiguration(config);

    ASSERT_EQ(updateCount, 3);
}

TEST_F(CoreEventTest, ComponentUpdatedMuted)
{
    const auto device = instance.getRootDevice();
    instance.removeFunctionBlock(instance.getFunctionBlocks()[0]); // Workaround due to save/load fb update bug

    const auto config = device.saveConfiguration();

    int updateCount = 0;
    getOnCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& /*args*/)
        {
            updateCount++;
        };
    
    device.asPtr<IPropertyObjectInternal>().disableCoreEventTrigger();

    device.loadConfiguration(config);
    device.loadConfiguration(config);
    device.loadConfiguration(config);

    ASSERT_EQ(updateCount, 0);
}

TEST_F(CoreEventTest, DeviceAdded)
{
    int addCount = 0;
    getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventId(), static_cast<int>(CoreEventId::ComponentAdded));
            ASSERT_EQ(args.getEventName(), "ComponentAdded");
            ASSERT_TRUE(args.getParameters().hasKey("Component"));
            ASSERT_EQ(args.getParameters().get("Component"), instance.getDevices()[addCount + 1]);
            ASSERT_EQ(comp, instance.getRootDevice().getItem("Dev"));
            addCount++;
        };

    instance.addDevice("mock_phys_device");
    instance.addDevice("mock_phys_device");
    instance.addDevice("mock_phys_device");

    ASSERT_EQ(addCount, 3);
}

TEST_F(CoreEventTest, DeviceRemoved)
{
    const auto dev1 = instance.addDevice("mock_phys_device");
    const auto dev2 = instance.addDevice("mock_phys_device");
    const auto dev3 = instance.addDevice("mock_phys_device");

    int removeCount = 0;
    getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventId(), static_cast<int>(CoreEventId::ComponentRemoved));
            ASSERT_EQ(args.getEventName(), "ComponentRemoved");
            ASSERT_TRUE(args.getParameters().hasKey("Id"));
            ASSERT_EQ(comp, instance.getRootDevice().getItem("Dev"));
            removeCount++;
        };
    
    instance.removeDevice(dev1);
    instance.removeDevice(dev2);
    instance.removeDevice(dev3);

    ASSERT_EQ(removeCount, 3);
}

TEST_F(CoreEventTest, DeviceAddedRemovedMuted)
{
    instance.getRootDevice().asPtr<IPropertyObjectInternal>().disableCoreEventTrigger();

    int callCount = 0;
    getOnCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& /*args*/)
        {
            callCount++;
        };

    const auto dev1 = instance.addDevice("mock_phys_device");
    const auto dev2 = instance.addDevice("mock_phys_device");
    
    instance.getRootDevice().asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();
    const auto dev3 = instance.addDevice("mock_phys_device");
    instance.getRootDevice().asPtr<IPropertyObjectInternal>().disableCoreEventTrigger();

    instance.removeDevice(dev1);
    instance.removeDevice(dev2);

    instance.getRootDevice().asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();
    instance.removeDevice(dev3);
    instance.getRootDevice().asPtr<IPropertyObjectInternal>().disableCoreEventTrigger();

    ASSERT_EQ(callCount, 2);
}

TEST_F(CoreEventTest, FBAdded)
{
    int addCount = 0;
    getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventId(), static_cast<int>(CoreEventId::ComponentAdded));
            ASSERT_EQ(args.getEventName(), "ComponentAdded");
            ASSERT_TRUE(args.getParameters().hasKey("Component"));
            ASSERT_EQ(args.getParameters().get("Component"), instance.getFunctionBlocks()[addCount + 1]);
            ASSERT_EQ(comp, instance.getRootDevice().getItem("FB"));
            addCount++;
        };

    instance.addFunctionBlock("mock_fb_uid");
    instance.addFunctionBlock("mock_fb_uid");
    instance.addFunctionBlock("mock_fb_uid");

    ASSERT_EQ(addCount, 3);
}

TEST_F(CoreEventTest, FBRemoved)
{
    const auto fb1 = instance.addFunctionBlock("mock_fb_uid");
    const auto fb2 = instance.addFunctionBlock("mock_fb_uid");
    const auto fb3 = instance.addFunctionBlock("mock_fb_uid");

    ListPtr<IString> ids = List<IString>(fb1.getLocalId(), fb2.getLocalId(), fb3.getLocalId());

    int removeCount = 0;
    getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventId(), static_cast<int>(CoreEventId::ComponentRemoved));
            ASSERT_EQ(args.getEventName(), "ComponentRemoved");
            ASSERT_TRUE(args.getParameters().hasKey("Id"));
            ASSERT_EQ(args.getParameters().get("Id"), ids[removeCount]);
            ASSERT_EQ(comp, instance.getRootDevice().getItem("FB"));
            removeCount++;
        };

    instance.removeFunctionBlock(fb1);
    instance.removeFunctionBlock(fb2);
    instance.removeFunctionBlock(fb3);

    ASSERT_EQ(removeCount, 3);
}

TEST_F(CoreEventTest, FBAddedRemovedMuted)
{
    instance.getRootDevice().asPtr<IPropertyObjectInternal>().disableCoreEventTrigger();

    int callCount = 0;
    getOnCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& /*args*/)
        {
            callCount++;
        };
    
    const auto fb1 = instance.addFunctionBlock("mock_fb_uid");
    const auto fb2 = instance.addFunctionBlock("mock_fb_uid");
    
    instance.getRootDevice().asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();
    const auto fb3 = instance.addFunctionBlock("mock_fb_uid");
    instance.getRootDevice().asPtr<IPropertyObjectInternal>().disableCoreEventTrigger();

    instance.removeFunctionBlock(fb1);
    instance.removeFunctionBlock(fb2);

    instance.getRootDevice().asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();
    instance.removeFunctionBlock(fb3);
    instance.getRootDevice().asPtr<IPropertyObjectInternal>().disableCoreEventTrigger();

    ASSERT_EQ(callCount, 2);
}

TEST_F(CoreEventTest, SignalAdded)
{
    const FolderConfigPtr sigs = instance.getItem("Sig");
    int addCount = 0;
    getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventId(), static_cast<int>(CoreEventId::ComponentAdded));
            ASSERT_EQ(args.getEventName(), "ComponentAdded");
            ASSERT_TRUE(args.getParameters().hasKey("Component"));
            ASSERT_EQ(args.getParameters().get("Component"), sigs.getItems()[addCount]);
            ASSERT_EQ(comp, sigs);
            addCount++;
        };

    sigs.addItem(Signal(instance.getContext(), sigs, "sig1"));
    sigs.addItem(Signal(instance.getContext(), sigs, "sig2"));
    sigs.addItem(Signal(instance.getContext(), sigs, "sig3"));

    ASSERT_EQ(addCount, 3);
}

TEST_F(CoreEventTest, SignalRemoved)
{
    const FolderConfigPtr sigs = instance.getItem("Sig");
    sigs.addItem(Signal(instance.getContext(), sigs, "sig1"));
    sigs.addItem(Signal(instance.getContext(), sigs, "sig2"));
    sigs.addItem(Signal(instance.getContext(), sigs, "sig3"));

    ListPtr<IString> ids = List<IString>("sig1", "sig2", "sig3");

    int removeCount = 0;
    getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventId(), static_cast<int>(CoreEventId::ComponentRemoved));
            ASSERT_EQ(args.getEventName(), "ComponentRemoved");
            ASSERT_TRUE(args.getParameters().hasKey("Id"));
            ASSERT_EQ(args.getParameters().get("Id"), ids[removeCount]);
            ASSERT_EQ(comp, sigs);
            removeCount++;
        };

    sigs.removeItemWithLocalId("sig1");
    sigs.removeItemWithLocalId("sig2");
    sigs.removeItemWithLocalId("sig3");

    ASSERT_EQ(removeCount, 3);
}

TEST_F(CoreEventTest, SignalAddedRemovedMuted)
{
    instance.getRootDevice().asPtr<IPropertyObjectInternal>().disableCoreEventTrigger();
    const FolderConfigPtr sigs = instance.getItem("Sig");

    int callCount = 0;
    getOnCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& /*args*/)
        {
            callCount++;
        };
    
    sigs.addItem(Signal(instance.getContext(), sigs, "sig1"));
    sigs.addItem(Signal(instance.getContext(), sigs, "sig2"));
    
    instance.getRootDevice().asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();
    sigs.addItem(Signal(instance.getContext(), sigs, "sig3"));
    instance.getRootDevice().asPtr<IPropertyObjectInternal>().disableCoreEventTrigger();
    
    sigs.removeItemWithLocalId("sig1");
    sigs.removeItemWithLocalId("sig2");

    instance.getRootDevice().asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();
    sigs.removeItemWithLocalId("sig3");
    instance.getRootDevice().asPtr<IPropertyObjectInternal>().disableCoreEventTrigger();

    ASSERT_EQ(callCount, 2);
}

TEST_F(CoreEventTest, InputPortAdded)
{
    const FolderConfigPtr ips = instance.getFunctionBlocks()[0].getItem("IP");
    int addCount = 0;
    getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventId(), static_cast<int>(CoreEventId::ComponentAdded));
            ASSERT_EQ(args.getEventName(), "ComponentAdded");
            ASSERT_TRUE(args.getParameters().hasKey("Component"));
            const auto ipps = ips.getItems();
            ASSERT_EQ(args.getParameters().get("Component"), ips.getItems()[addCount + 2]);
            ASSERT_EQ(comp, ips);
            addCount++;
        };

    ips.addItem(InputPort(instance.getContext(), ips, "ip1"));
    ips.addItem(InputPort(instance.getContext(), ips, "ip2"));
    ips.addItem(InputPort(instance.getContext(), ips, "ip3"));

    ASSERT_EQ(addCount, 3);
}

TEST_F(CoreEventTest, InputPortRemoved)
{
    const FolderConfigPtr ips = instance.getFunctionBlocks()[0].getItem("IP");
    ips.addItem(InputPort(instance.getContext(), ips, "ip1"));
    ips.addItem(InputPort(instance.getContext(), ips, "ip2"));
    ips.addItem(InputPort(instance.getContext(), ips, "ip3"));

    ListPtr<IString> ids = List<IString>("ip1", "ip2", "ip3");

    int removeCount = 0;
    getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventId(), static_cast<int>(CoreEventId::ComponentRemoved));
            ASSERT_EQ(args.getEventName(), "ComponentRemoved");
            ASSERT_TRUE(args.getParameters().hasKey("Id"));
            ASSERT_EQ(args.getParameters().get("Id"), ids[removeCount]);
            ASSERT_EQ(comp, ips);
            removeCount++;
        };

    ips.removeItemWithLocalId("ip1");
    ips.removeItemWithLocalId("ip2");
    ips.removeItemWithLocalId("ip3");

    ASSERT_EQ(removeCount, 3);
}

TEST_F(CoreEventTest, InputPortAddedRemovedMuted)
{
    instance.getRootDevice().asPtr<IPropertyObjectInternal>().disableCoreEventTrigger();
    const FolderConfigPtr ips = instance.getFunctionBlocks()[0].getItem("IP");

    int callCount = 0;
    getOnCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& /*args*/)
        {
            callCount++;
        };
    
    ips.addItem(InputPort(instance.getContext(), ips, "ip1"));
    ips.addItem(InputPort(instance.getContext(), ips, "ip2"));
    
    instance.getRootDevice().asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();
    ips.addItem(InputPort(instance.getContext(), ips, "ip3"));
    instance.getRootDevice().asPtr<IPropertyObjectInternal>().disableCoreEventTrigger();
    
    ips.removeItemWithLocalId("ip1");
    ips.removeItemWithLocalId("ip2");

    instance.getRootDevice().asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();
    ips.removeItemWithLocalId("ip3");
    instance.getRootDevice().asPtr<IPropertyObjectInternal>().disableCoreEventTrigger();

    ASSERT_EQ(callCount, 2);
}

TEST_F(CoreEventTest, CoreEventException)
{
    getOnCoreEvent() += [](const ComponentPtr&, const CoreEventArgsPtr&) { throw GeneralErrorException{}; };
    ASSERT_NO_THROW(instance.addProperty(StringProperty("test", "test")));
    ASSERT_NO_THROW(instance.setPropertyValue("test", "foo"));
    instance.beginUpdate();
    ASSERT_NO_THROW(instance.endUpdate());
    ASSERT_NO_THROW(instance.removeProperty("test"));

    const FolderConfigPtr sigs = instance.getItem("Sig");
    ASSERT_NO_THROW(sigs.addItem(Signal(instance.getContext(), sigs, "sig1")));
    ASSERT_NO_THROW(sigs.removeItemWithLocalId("sig1"));

    const auto sig = instance.getSignalsRecursive()[0];
    const auto ip = instance.getFunctionBlocks()[0].getInputPorts()[0];

    ASSERT_NO_THROW(ip.connect(sig));
    ASSERT_NO_THROW(ip.disconnect());
    ASSERT_NO_THROW(sig.asPtr<ISignalConfig>().setDescriptor(DataDescriptorBuilder().setSampleType(SampleType::Float32).setRule(ExplicitDataRule()).build()));

    const auto str = instance.saveConfiguration();

    ASSERT_NO_THROW(instance.loadConfiguration(str));
}

TEST_F(CoreEventTest, ActiveChanged)
{
    int changeCount = 0;
    getOnCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventId(), static_cast<int>(CoreEventId::AttributeChanged));
            ASSERT_EQ(args.getEventName(), "AttributeChanged");
            ASSERT_TRUE(args.getParameters().hasKey("Active"));
            changeCount++;
        };

    instance.setActive(false);
    instance.setActive(false);
    instance.setActive(true);

    instance.getRootDevice().asPtr<IPropertyObjectInternal>().disableCoreEventTrigger();
    instance.setActive(false);
    instance.setActive(true);
    instance.getRootDevice().asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();

    const auto sig = instance.getSignalsRecursive()[0];
    sig.setActive(false);
    sig.setActive(false);
    sig.setActive(true);

    ASSERT_EQ(changeCount, 4);
}

TEST_F(CoreEventTest, DomainSignalChanged)
{
    const auto domainSig1 = Signal(instance.getContext(), instance.getDevices()[0], "testDomainSig1");
    const auto domainSig2 = Signal(instance.getContext(), instance.getDevices()[0], "testDomainSig2");
    int changeCount = 0;
    getOnCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventId(), static_cast<int>(CoreEventId::AttributeChanged));
            ASSERT_EQ(args.getEventName(), "AttributeChanged");
            ASSERT_TRUE(args.getParameters().hasKey("DomainSignal"));
            if (changeCount % 2 == 0)
                ASSERT_EQ(args.getParameters().get("DomainSignal"), domainSig1);
            else
                ASSERT_EQ(args.getParameters().get("DomainSignal"), domainSig2);
            changeCount++;
        };

    const SignalConfigPtr sig = instance.getSignalsRecursive()[0];
    sig.setDomainSignal(domainSig1);
    sig.setDomainSignal(domainSig2);
    sig.setDomainSignal(domainSig2);

    instance.getRootDevice().asPtr<IPropertyObjectInternal>().disableCoreEventTrigger();
    sig.setDomainSignal(domainSig1);
    sig.setDomainSignal(domainSig2);
    instance.getRootDevice().asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();
    
    sig.setDomainSignal(domainSig1);
    sig.setDomainSignal(domainSig1);
    sig.setDomainSignal(domainSig2);

    ASSERT_EQ(changeCount, 4);
}

TEST_F(CoreEventTest, RelatedSignalsChanged)
{
    const auto relatedSig1 = Signal(instance.getContext(), instance.getDevices()[0], "relatedSig1");
    const auto relatedSig2 = Signal(instance.getContext(), instance.getDevices()[0], "relatedSig2");
    const auto relatedSig3 = Signal(instance.getContext(), instance.getDevices()[0], "relatedSig3");

    int changeCount = 0;
    getOnCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventId(), static_cast<int>(CoreEventId::AttributeChanged));
            ASSERT_EQ(args.getEventName(), "AttributeChanged");
            ASSERT_TRUE(args.getParameters().hasKey("RelatedSignals"));
            ASSERT_TRUE(args.getParameters().get("RelatedSignals").asPtrOrNull<IList>().assigned());
            changeCount++;
        };

    const SignalConfigPtr sig = instance.getSignalsRecursive()[0];
    sig.addRelatedSignal(relatedSig1);
    sig.addRelatedSignal(relatedSig2);
    sig.removeRelatedSignal(relatedSig2);
    sig.setRelatedSignals(List<ISignal>(relatedSig1, relatedSig2, relatedSig3));

    instance.getRootDevice().asPtr<IPropertyObjectInternal>().disableCoreEventTrigger();
    sig.setRelatedSignals(List<ISignal>(relatedSig1, relatedSig2, relatedSig3));
    sig.removeRelatedSignal(relatedSig1);
    sig.removeRelatedSignal(relatedSig2);
    sig.removeRelatedSignal(relatedSig3);

    ASSERT_EQ(changeCount, 4);
}

TEST_F(CoreEventTest, NameDescriptionChanged)
{
    int changeCount = 0;
    getOnCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventId(), static_cast<int>(CoreEventId::AttributeChanged));
            ASSERT_EQ(args.getEventName(), "AttributeChanged");
            changeCount++;
        };

    instance.setName("name1");
    instance.setName("name1");
    instance.setName("name2");
    
    instance.setDescription("desc1");
    instance.setDescription("desc1");
    instance.setDescription("desc2");

    instance.getRootDevice().asPtr<IPropertyObjectInternal>().disableCoreEventTrigger();

    instance.setName("name1");
    instance.setName("name2");
    
    instance.setDescription("desc1");
    instance.setDescription("desc2");

    instance.getRootDevice().asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();

    instance.setName("name1");
    instance.setName("name1");
    instance.setName("name2");
    
    instance.setDescription("desc1");
    instance.setDescription("desc1");
    instance.setDescription("desc2");

    ASSERT_EQ(changeCount, 8);
}
TEST_F(CoreEventTest, TagsChanged)
{
    int changeCount = 0;
    getOnCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventId(), static_cast<int>(CoreEventId::TagsChanged));
            ASSERT_EQ(args.getEventName(), "TagsChanged");
            changeCount++;
        };

    const auto tagsPrivate = instance.getTags().asPtr<ITagsPrivate>();

    tagsPrivate.add("Tag1");
    tagsPrivate.add("Tag1");
    tagsPrivate.add("Tag2");
    tagsPrivate.add("Tag2");

    ASSERT_EQ(changeCount, 2);
}

TEST_F(CoreEventTest, TagsChangedMuted)
{
    instance.getRootDevice().asPtr<IPropertyObjectInternal>().disableCoreEventTrigger();

    int changeCount = 0;
    getOnCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventId(), static_cast<int>(CoreEventId::TagsChanged));
            ASSERT_EQ(args.getEventName(), "TagsChanged");
            changeCount++;
        };

    const auto tagsPrivate = instance.getTags().asPtr<ITagsPrivate>();

    tagsPrivate.add("Tag1");
    tagsPrivate.add("Tag1");
    tagsPrivate.add("Tag2");
    tagsPrivate.add("Tag2");

    ASSERT_EQ(changeCount, 0);
}

TEST_F(CoreEventTest, StatusChanged)
{
    const auto typeManager = instance.getContext().getTypeManager();
    const auto statusType = EnumerationType("StatusType", List<IString>("Status0", "Status1"));
    typeManager.addType(statusType);

    const auto statusInitValue = Enumeration("StatusType", "Status0", typeManager);
    const auto statusValue = Enumeration("StatusType", "Status1", typeManager);

    const auto device = instance.getRootDevice();
    const auto statusContainer = device.getStatusContainer().asPtr<IComponentStatusContainerPrivate>();
    statusContainer.addStatus("TestStatus", statusInitValue);

    int changeCount = 0;
    getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
    {
        ASSERT_EQ(args.getEventId(), static_cast<int>(CoreEventId::StatusChanged));
        ASSERT_EQ(args.getEventName(), "StatusChanged");
        ASSERT_TRUE(args.getParameters().hasKey("TestStatus"));
        ASSERT_EQ(comp, instance.getRootDevice());
        if (changeCount % 2 == 0)
        {
            ASSERT_EQ(args.getParameters().get("TestStatus"), statusValue);
            ASSERT_EQ(args.getParameters().get("TestStatus"), "Status1");
        }
        else
        {
            ASSERT_EQ(args.getParameters().get("TestStatus"), statusInitValue);
            ASSERT_EQ(args.getParameters().get("TestStatus"), "Status0");
        }
        changeCount++;
    };

    statusContainer.setStatus("TestStatus", statusValue);
    statusContainer.setStatus("TestStatus", statusValue);
    statusContainer.setStatus("TestStatus", statusInitValue);
    statusContainer.setStatus("TestStatus", statusValue);

    ASSERT_EQ(changeCount, 3);
}

TEST_F(CoreEventTest, StatusChangedMuted)
{
    const auto typeManager = instance.getContext().getTypeManager();
    const auto statusType = EnumerationType("StatusType", List<IString>("Status0", "Status1"));
    typeManager.addType(statusType);

    const auto statusInitValue = Enumeration("StatusType", "Status0", typeManager);
    const auto statusValue = Enumeration("StatusType", "Status1", typeManager);

    const auto device = instance.getRootDevice();
    const auto statusContainer = device.getStatusContainer().asPtr<IComponentStatusContainerPrivate>();
    statusContainer.addStatus("TestStatus", statusInitValue);

    int changeCount = 0;
    getOnCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& /*args*/)
    {
        changeCount++;
    };

    device.asPtr<IPropertyObjectInternal>().disableCoreEventTrigger();
    statusContainer.setStatus("TestStatus", statusValue);
    statusContainer.setStatus("TestStatus", statusValue);

    device.asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();
    statusContainer.setStatus("TestStatus", statusValue);
    statusContainer.setStatus("TestStatus", statusInitValue);

    instance.getRootDevice().asPtr<IPropertyObjectInternal>().disableCoreEventTrigger();
    statusContainer.setStatus("TestStatus", statusValue);
    statusContainer.setStatus("TestStatus", statusInitValue);

    ASSERT_EQ(changeCount, 1);
}

TEST_F(CoreEventTest, TypeAdded)
{
    const auto typeManager = instance.getContext().getTypeManager();

    int addCount = 0;
    getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
    {
        ASSERT_EQ(args.getEventId(), static_cast<int>(CoreEventId::TypeAdded));
        ASSERT_TRUE(args.getParameters().hasKey("Type"));
        ASSERT_FALSE(comp.assigned());
        addCount++;
    };

    const auto statusType = EnumerationType("StatusType", List<IString>("Status0", "Status1"));
    typeManager.addType(statusType);
    
    const auto structType1 = StructType("StructType1", List<IString>("Field0"), List<IType>(SimpleType(ctString)));
    typeManager.addType(structType1);

    const auto structType2 = StructType("StructType2", List<IString>("Field0"), List<IType>(SimpleType(ctString)));
    typeManager.addType(structType2);

    ASSERT_EQ(addCount, 3);
}

TEST_F(CoreEventTest, TypeRemoved)
{
    const auto typeManager = instance.getContext().getTypeManager();
    
    const auto statusType = EnumerationType("StatusType", List<IString>("Status0", "Status1"));
    typeManager.addType(statusType);
    
    const auto structType1 = StructType("StructType1", List<IString>("Field0"), List<IType>(SimpleType(ctString)));
    typeManager.addType(structType1);

    const auto structType2 = StructType("StructType2", List<IString>("Field0"), List<IType>(SimpleType(ctString)));
    typeManager.addType(structType2);

    int removeCount = 0;
    getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
    {
        ASSERT_EQ(args.getEventId(), static_cast<int>(CoreEventId::TypeRemoved));
        ASSERT_TRUE(args.getParameters().hasKey("TypeName"));
        ASSERT_FALSE(comp.assigned());
        removeCount++;
    };

    typeManager.removeType("StatusType");
    typeManager.removeType("StructType1");
    typeManager.removeType("StructType2");

    ASSERT_EQ(removeCount, 3);
}

TEST_F(CoreEventTest, DomainChanged)
{
    const auto device = instance.getDevices()[0];
    const auto mock = dynamic_cast<MockPhysicalDeviceImpl*>(device.getObject());

    int changeCount = 0;
    getOnCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& args)
    {
        ASSERT_EQ(args.getEventId(), static_cast<int>(CoreEventId::DeviceDomainChanged));
        ASSERT_EQ(args.getEventName(), "DeviceDomainChanged");
        ASSERT_TRUE(args.getParameters().hasKey("DeviceDomain"));
        changeCount++;
    };

    const DeviceDomainPtr domain1 = DeviceDomain(Ratio(1, 1), "foo", Unit("test"));
    const DeviceDomainPtr domain2 = DeviceDomain(Ratio(1, 1), "bar", Unit("test1"));

    mock->setDeviceDomainHelper(domain1);
    ASSERT_EQ(device.getDomain().getOrigin(), "foo");

    mock->setDeviceDomainHelper(domain2);
    ASSERT_EQ(device.getDomain().getOrigin(), "bar");

    mock->setDeviceDomainHelper(domain1);
    ASSERT_EQ(device.getDomain().getOrigin(), "foo");

    ASSERT_EQ(changeCount, 3);
}
