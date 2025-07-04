#include <opendaq/function_block_impl.h>
#include <opendaq/function_block_ptr.h>
#include <gtest/gtest.h>
#include <opendaq/context_factory.h>
#include <opendaq/function_block_type_factory.h>
#include <opendaq/component_deserialize_context_factory.h>
#include <opendaq/input_port_config_ptr.h>
#include <opendaq/tags_private_ptr.h>
#include <coreobjects/property_object_class_factory.h>
#include <opendaq/input_port_private_ptr.h>
#include <opendaq/scheduler_factory.h>

using FunctionBlockTest = testing::Test;

TEST_F(FunctionBlockTest, Folders)
{
    daq::FunctionBlockTypePtr fbType = daq::FunctionBlockType("test_uid", "test_name", "test_description");
    auto fb = daq::createWithImplementation<daq::IFunctionBlock, daq::FunctionBlock>(fbType, daq::NullContext(), nullptr, "fb");

    ASSERT_EQ(fb.getSignals().getElementInterfaceId(), daq::ISignal::Id);
    ASSERT_EQ(fb.getFunctionBlocks().getElementInterfaceId(), daq::IFunctionBlock::Id);
    ASSERT_EQ(fb.getInputPorts().getElementInterfaceId(), daq::IInputPort::Id);
}

TEST_F(FunctionBlockTest, FunctionBlockTypeStructType)
{
    const auto structType = daq::FunctionBlockTypeStructType();
    const daq::StructPtr structPtr = daq::FunctionBlockType("Id", "Name", "Desc");
    ASSERT_EQ(structType, structPtr.getStructType());
}

TEST_F(FunctionBlockTest, FunctionBlockTypeStructFields)
{
    const daq::StructPtr structPtr = daq::FunctionBlockType("Id", "Name", "Desc");
    ASSERT_EQ(structPtr.get("Id"), "Id");
    ASSERT_EQ(structPtr.get("Name"), "Name");
    ASSERT_EQ(structPtr.get("Description"), "Desc");
}

TEST_F(FunctionBlockTest, FunctionBlockTypeStructNames)
{
    const auto structType = daq::FunctionBlockTypeStructType();
    const daq::StructPtr structPtr = daq::FunctionBlockType("Id", "Name", "Desc");
    ASSERT_EQ(structType.getFieldNames(), structPtr.getFieldNames());
}

TEST_F(FunctionBlockTest, FunctionBlockTypeSerializationDeserialization)
{
    auto defConfig = daq::PropertyObject();
    defConfig.addProperty(daq::StringPropertyBuilder("cfg", "val").build());

    const daq::FunctionBlockTypePtr fbType = daq::FunctionBlockType("Id", "Name", "Desc", defConfig);

    const auto serializer = daq::JsonSerializer();
    fbType.serialize(serializer);

    const auto serializedFbType = serializer.getOutput();

    const auto deserializer = daq::JsonDeserializer();
    const daq::FunctionBlockTypePtr newFbType = deserializer.deserialize(serializedFbType);

    ASSERT_EQ(newFbType.getId(), fbType.getId());
    ASSERT_EQ(newFbType.getName(), fbType.getName());
    ASSERT_EQ(newFbType.getDescription(), fbType.getDescription());

    const auto newDefConfig = newFbType.createDefaultConfig();
    ASSERT_EQ(defConfig.getPropertyValue("cfg"), newDefConfig.getPropertyValue("cfg"));
}


TEST_F(FunctionBlockTest, HasItem)
{
    daq::FunctionBlockTypePtr fbType = daq::FunctionBlockType("test_uid", "test_name", "test_description");
    const auto fb = daq::createWithImplementation<daq::IFunctionBlock, daq::FunctionBlock>(fbType, daq::NullContext(), nullptr, "fb");

    ASSERT_TRUE(fb.hasItem("Sig"));
    ASSERT_TRUE(fb.hasItem("IP"));
    ASSERT_TRUE(fb.hasItem("FB"));
    ASSERT_FALSE(fb.hasItem("none"));
}

class MockFbImpl final : public daq::FunctionBlock
{
public:
    MockFbImpl(const daq::ContextPtr& ctx, const daq::ComponentPtr& parent, const daq::StringPtr& localId, const daq::StringPtr& className, bool nested)
        : daq::FunctionBlock(daq::FunctionBlockType("test_uid", "test_name", "test_description"), ctx, parent, localId, className)
    {
        createAndAddSignal("sig1");
        createAndAddInputPort("ip1", daq::PacketReadyNotification::None);
        if (!nested)
        {
            const auto nestedFb =
                daq::createWithImplementation<daq::IFunctionBlock, MockFbImpl>(ctx, this->functionBlocks, "nestedFb", className, true);
            addNestedFunctionBlock(nestedFb);
        }
    }
};

TEST_F(FunctionBlockTest, SerializeAndDeserialize)
{
    const auto fb = daq::createWithImplementation<daq::IFunctionBlock, MockFbImpl>(daq::NullContext(), nullptr, "fb", nullptr, false);
    fb.setName("fb_name");
    fb.setDescription("fb_desc");
    fb.getTags().asPtr<daq::ITagsPrivate>().add("fld_tag");

    const auto serializer = daq::JsonSerializer(daq::True);
    fb.serialize(serializer);
    const auto str1 = serializer.getOutput();

    const auto deserializer = daq::JsonDeserializer();

    const auto deserializeContext = daq::ComponentDeserializeContext(daq::NullContext(), nullptr, nullptr, "fb");

    const daq::FunctionBlockPtr newFb = deserializer.deserialize(str1, deserializeContext, nullptr);

    ASSERT_EQ(newFb.getName(), fb.getName());
    ASSERT_EQ(newFb.getDescription(), fb.getDescription());
    ASSERT_EQ(newFb.getTags(), fb.getTags());

    ASSERT_EQ(newFb.getSignals().getElementInterfaceId(), daq::ISignal::Id);
    ASSERT_EQ(newFb.getInputPorts().getElementInterfaceId(), daq::IInputPort::Id);
    ASSERT_EQ(newFb.getFunctionBlocks().getElementInterfaceId(), daq::IFunctionBlock::Id);

    const auto serializer2 = daq::JsonSerializer(daq::True);
    newFb.serialize(serializer2);
    const auto str2 = serializer2.getOutput();

    ASSERT_EQ(str1, str2);
}

TEST_F(FunctionBlockTest, SerializeAndDeserializeWithClassName)
{
    const auto ctx = daq::NullContext();

    const auto fbClass =
        daq::PropertyObjectClassBuilder("FBClass").addProperty(daq::StringPropertyBuilder("StringProp", "-").build()).build();

    ctx.getTypeManager().addType(fbClass);

    const auto fb = daq::createWithImplementation<daq::IFunctionBlock, MockFbImpl>(ctx, nullptr, "fb", "FBClass", false);
    fb.setName("fb_name");
    fb.setDescription("fb_desc");
    fb.getTags().asPtr<daq::ITagsPrivate>().add("fld_tag");
    fb.setPropertyValue("StringProp", "Value");

    const auto serializer = daq::JsonSerializer(daq::True);
    fb.serialize(serializer);
    const auto str1 = serializer.getOutput();

    const auto deserializer = daq::JsonDeserializer();

    const auto deserializeContext = daq::ComponentDeserializeContext(ctx, nullptr, nullptr, "fb");

    const daq::FunctionBlockPtr newFb = deserializer.deserialize(str1, deserializeContext, nullptr);

    ASSERT_EQ(newFb.getName(), fb.getName());
    ASSERT_EQ(newFb.getDescription(), fb.getDescription());
    ASSERT_EQ(newFb.getTags(), fb.getTags());

    ASSERT_EQ(newFb.getSignals().getElementInterfaceId(), daq::ISignal::Id);
    ASSERT_EQ(newFb.getInputPorts().getElementInterfaceId(), daq::IInputPort::Id);
    ASSERT_EQ(newFb.getFunctionBlocks().getElementInterfaceId(), daq::IFunctionBlock::Id);

    const auto serializer2 = daq::JsonSerializer(daq::True);
    newFb.serialize(serializer2);
    const auto str2 = serializer2.getOutput();

    ASSERT_EQ(str1, str2);
}

TEST_F(FunctionBlockTest, BeginUpdateEndUpdate)
{
    const auto fb = daq::createWithImplementation<daq::IFunctionBlock, MockFbImpl>(daq::NullContext(), nullptr, "fb", nullptr, false);
    fb.addProperty(daq::StringPropertyBuilder("FbProp", "-").build());

    const auto sig = fb.getSignals()[0];
    sig.addProperty(daq::StringPropertyBuilder("SigProp", "-").build());

    fb.beginUpdate();

    fb.setPropertyValue("FbProp", "s");
    ASSERT_EQ(fb.getPropertyValue("FbProp"), "-");

    sig.setPropertyValue("SigProp", "cs");
    ASSERT_EQ(sig.getPropertyValue("SigProp"), "-");

    fb.endUpdate();

    ASSERT_EQ(fb.getPropertyValue("FbProp"), "s");
    ASSERT_EQ(sig.getPropertyValue("SigProp"), "cs");
}

class MockFbImpl1 final : public daq::FunctionBlock
{
public:
    MockFbImpl1(const daq::ContextPtr& context)
        : daq::FunctionBlock(daq::FunctionBlockType("test_uid", "test_name", "test_description"), context, nullptr, "MockFb", "")
    {
        createAndAddInputPort("IP", daq::PacketReadyNotification::SameThread);
        objPtr.addProperty(daq::IntProperty("ConnectIp", 100));
    }
    
    void onPacketReceived(const daq::InputPortPtr& /*port*/) override
    {
        auto lock = getAcquisitionLock();
    }
};

TEST_F(FunctionBlockTest, SetDomainDescriptorUnderLock)
{
    const auto logger = daq::Logger();
    auto context = daq::Context(daq::Scheduler(logger), logger, daq::TypeManager(), nullptr, nullptr);
    auto fb = daq::createWithImplementation<daq::IFunctionBlock, MockFbImpl1>(context);

    auto desc1 = daq::DataDescriptorBuilder().build();
    auto desc2 = daq::DataDescriptorBuilder().setSampleType(daq::SampleType::Int16).build();

    const auto signal1 = Signal(context, nullptr, "sig");
    const auto signal2 = Signal(context, nullptr, "domainSig");

    signal1.setDescriptor(desc1);
    signal2.setDescriptor(desc2);

    fb.getOnPropertyValueWrite("ConnectIp") += [&fb, &signal1, &signal2](daq::PropertyObjectPtr&, const daq::PropertyValueEventArgsPtr& args)
    {
        if (static_cast<int>(args.getValue()) % 2 == 0)
            ASSERT_NO_THROW(fb.getInputPorts()[0].asPtr<daq::IInputPortPrivate>().connectSignalSchedulerNotification(signal1));
        else
            ASSERT_NO_THROW(fb.getInputPorts()[0].asPtr<daq::IInputPortPrivate>().connectSignalSchedulerNotification(signal2));
    };

    for (int i = 0; i < 10; ++i)
        fb.setPropertyValue("ConnectIp", i);
}
