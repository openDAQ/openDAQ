#include <opendaq/function_block_impl.h>
#include <opendaq/function_block_ptr.h>
#include <gtest/gtest.h>
#include <opendaq/context_factory.h>
#include <opendaq/function_block_type_factory.h>
#include <opendaq/component_deserialize_context_factory.h>
#include <opendaq/input_port_config_ptr.h>
#include <opendaq/tags_private_ptr.h>

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
    const daq::StructPtr structPtr = daq::FunctionBlockType("id", "name", "desc");
    ASSERT_EQ(structType, structPtr.getStructType());
}

TEST_F(FunctionBlockTest, FunctionBlockTypeStructFields)
{
    const daq::StructPtr structPtr = daq::FunctionBlockType("id", "name", "desc");
    ASSERT_EQ(structPtr.get("id"), "id");
    ASSERT_EQ(structPtr.get("name"), "name");
    ASSERT_EQ(structPtr.get("description"), "desc");
}

TEST_F(FunctionBlockTest, FunctionBlockTypeStructNames)
{
    const auto structType = daq::FunctionBlockTypeStructType();
    const daq::StructPtr structPtr = daq::FunctionBlockType("id", "name", "desc");
    ASSERT_EQ(structType.getFieldNames(), structPtr.getFieldNames());
}

TEST_F(FunctionBlockTest, FunctionBlockTypeSerializationDeserialization)
{
    auto defConfig = daq::PropertyObject();
    defConfig.addProperty(daq::StringPropertyBuilder("cfg", "val").build());

    const daq::FunctionBlockTypePtr fbType = daq::FunctionBlockType("id", "name", "desc", [&defConfig] { return defConfig; });

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
    MockFbImpl(const daq::ContextPtr& ctx, const daq::ComponentPtr& parent, const daq::StringPtr& localId, bool nested)
        : daq::FunctionBlock(daq::FunctionBlockType("test_uid", "test_name", "test_description"), ctx, parent, localId)
    {
        createAndAddSignal("sig1");
        createAndAddInputPort("ip1", daq::PacketReadyNotification::None);
        if (!nested)
        {
            const auto nestedFb =
                daq::createWithImplementation<daq::IFunctionBlock, MockFbImpl>(ctx, this->functionBlocks, "nestedFb", true);
            addNestedFunctionBlock(nestedFb);
        }
    }
};

TEST_F(FunctionBlockTest, SerializeAndDeserialize)
{
    const auto fb = daq::createWithImplementation<daq::IFunctionBlock, MockFbImpl>(daq::NullContext(), nullptr, "fb", false);
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

    const auto serializer2 = daq::JsonSerializer(daq::True);
    newFb.serialize(serializer2);
    const auto str2 = serializer2.getOutput();

    ASSERT_EQ(str1, str2);
}
