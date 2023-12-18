#include <opendaq/component_factory.h>
#include <opendaq/context_factory.h>
#include <coreobjects/property_factory.h>
#include <gtest/gtest.h>

using namespace daq;

using CoreEventTest = testing::Test;

TEST_F(CoreEventTest, PropertyChanged)
{
    const auto context = NullContext();
    const auto component = Component(context, nullptr, "comp");
    component.addProperty(StringProperty("string", "foo"));
    int callCount = 0;

    context.getOnCoreEvent() += [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
    {
        ASSERT_EQ(args.getEventId(), core_event_ids::PropertyValueChanged);
        ASSERT_EQ(args.getEventName(), "PropertyValueChanged");
        ASSERT_EQ(comp, component);
        ASSERT_TRUE(args.getParameters().hasKey("Name"));
        ASSERT_TRUE(args.getParameters().hasKey("Value"));

        callCount++;
    };

    component.setPropertyValue("string", "bar");
    component.setPropertyValue("string", "foo");
    component.clearPropertyValue("string");

    ASSERT_EQ(callCount, 3);
}

TEST_F(CoreEventTest, PropertyChangedWithInternalEvent)
{
    const auto context = NullContext();
    const auto component = Component(context, nullptr, "comp");
    component.addProperty(StringProperty("string", "foo"));

    int callCount = 0;
    component.getOnPropertyValueWrite("string") +=
        [&](const PropertyObjectPtr& obj, const PropertyValueEventArgsPtr& args)
        {
            args.setValue("override_" + std::to_string(callCount));
        };

    context.getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventId(), core_event_ids::PropertyValueChanged);
            ASSERT_EQ(args.getEventName(), "PropertyValueChanged");
            ASSERT_EQ(comp, component);
            ASSERT_TRUE(args.getParameters().hasKey("Name"));
            ASSERT_TRUE(args.getParameters().hasKey("Value"));
            ASSERT_EQ(args.getParameters().get("Value"), "override_" + std::to_string(callCount));

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

    int propChangeCount = 0;
    int updateCount = 0;
    int otherCount = 0;

    context.getOnCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& args)
        {
            DictPtr<IString, IBaseObject> updated;
            switch (args.getEventId())
            {
                case core_event_ids::PropertyValueChanged:
                    propChangeCount++;
                    break;
                case core_event_ids::UpdateEnd:
                    updateCount++;
                    updated = args.getParameters().get("UpdatedProperties");
                    ASSERT_EQ(updated.getCount(), 2);
                    ASSERT_EQ(args.getEventName(), "UpdateEnd");
                    break;
                default:
                    otherCount++;
                    break;
            }            
        };

    const auto deserializer = JsonDeserializer();
    deserializer.update(component, serializer.getOutput());

    ASSERT_EQ(propChangeCount, 0);
    ASSERT_EQ(updateCount, 1);
    ASSERT_EQ(otherCount, 0);
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

    context.getOnCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& args)
        {
            DictPtr<IString, IBaseObject> updated;
            switch (args.getEventId())
            {
                case core_event_ids::PropertyValueChanged:
                    propChangeCount++;
                    break;
                case core_event_ids::UpdateEnd:
                    updateCount++;
                    updated = args.getParameters().get("UpdatedProperties");
                    ASSERT_EQ(updated.getCount(), 2);
                    ASSERT_EQ(args.getEventName(), "UpdateEnd");
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

    context.getOnCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventId(), core_event_ids::PropertyAdded);
            ASSERT_EQ(args.getEventName(), "PropertyAdded");
            ASSERT_TRUE(args.getParameters().hasKey("Property"));
            addCount++;
        };

    component.addProperty(StringProperty("string1", "foo"));
    component.addProperty(StringProperty("string2", "bar"));
    component.addProperty(FloatProperty("float", 1.123));
    ASSERT_EQ(addCount, 3);
}

TEST_F(CoreEventTest, PropertyRemovedEvent)
{
    const auto context = NullContext();
    const auto component = Component(context, nullptr, "comp");
    
    component.addProperty(StringProperty("string1", "foo"));
    component.addProperty(StringProperty("string2", "bar"));
    component.addProperty(FloatProperty("float", 1.123));

    int removeCount = 0;

    context.getOnCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventId(), core_event_ids::PropertyRemoved);
            ASSERT_EQ(args.getEventName(), "PropertyRemoved");
            ASSERT_TRUE(args.getParameters().hasKey("Name"));
            removeCount++;
        };

    component.removeProperty("string1");
    component.removeProperty("string2");
    component.removeProperty("float");
    ASSERT_EQ(removeCount, 3);
}
