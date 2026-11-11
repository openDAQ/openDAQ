#include <opendaq/function_block_impl.h>
#include <opendaq/function_block_ptr.h>
#include <gtest/gtest.h>
#include <opendaq/context_factory.h>
#include <opendaq/function_block_type_factory.h>
#include <opendaq/function_block_type_impl.h>
#include <opendaq/component_type_builder_factory.h>
#include <coreobjects/property_factory.h>
#include <coreobjects/property_object_factory.h>
#include <opendaq/component_deserialize_context_factory.h>
#include <opendaq/input_port_config_ptr.h>
#include <opendaq/tags_private_ptr.h>
#include <coreobjects/property_object_class_factory.h>
#include <opendaq/input_port_private_ptr.h>
#include <opendaq/scheduler_factory.h>
#include <opendaq/update_parameters_factory.h>
#include <coreobjects/property_object_internal_ptr.h>
#include <testutils/testutils.h>

#include "opendaq/exceptions.h"

#include <atomic>

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
    ASSERT_EQ(structPtr.get("InputPolicy"), static_cast<daq::Int>(daq::FunctionBlockInputPolicy::Unspecified));
    ASSERT_EQ(structPtr.get("Singleton"), false);
    ASSERT_FALSE(structPtr.get("CommonSettingsTypeId").assigned());
}

TEST_F(FunctionBlockTest, FunctionBlockTypeStructFieldsWithOptions)
{
    const daq::StructPtr structPtr = daq::FunctionBlockTypeBuilder()
                                         .setId("Id")
                                         .setName("Name")
                                         .setDescription("Desc")
                                         .setInputPolicy(daq::FunctionBlockInputPolicy::OneAlwaysEmpty)
                                         .setSingleton(daq::True)
                                         .setCommonSettingsTypeId("SettingsId")
                                         .build();

    ASSERT_EQ(structPtr.get("InputPolicy"), static_cast<daq::Int>(daq::FunctionBlockInputPolicy::OneAlwaysEmpty));
    ASSERT_EQ(structPtr.get("Singleton"), true);
    ASSERT_EQ(structPtr.get("CommonSettingsTypeId"), "SettingsId");
}

TEST_F(FunctionBlockTest, FunctionBlockTypeStructFieldCoreTypes)
{
    const daq::StructPtr structPtr = daq::FunctionBlockTypeBuilder()
                                         .setId("Id")
                                         .setName("Name")
                                         .setDescription("Desc")
                                         .setInputPolicy(daq::FunctionBlockInputPolicy::OneAlwaysEmpty)
                                         .setCommonSettingsTypeId("SettingsId")
                                         .build();

    ASSERT_EQ(structPtr.get("InputPolicy").getCoreType(), daq::ctInt);
    ASSERT_EQ(structPtr.get("Singleton").getCoreType(), daq::ctBool);
    ASSERT_EQ(structPtr.get("CommonSettingsTypeId").getCoreType(), daq::ctString);
}

TEST_F(FunctionBlockTest, FunctionBlockTypeEqualityRespectsOptions)
{
    const auto createType = [](daq::FunctionBlockInputPolicy inputPolicy, daq::Bool singleton, const daq::StringPtr& settingsId)
    {
        return daq::FunctionBlockTypeBuilder()
            .setId("Id")
            .setName("Name")
            .setDescription("Desc")
            .setInputPolicy(inputPolicy)
            .setSingleton(singleton)
            .setCommonSettingsTypeId(settingsId)
            .build();
    };

    using Policy = daq::FunctionBlockInputPolicy;
    const auto reference = createType(Policy::OneAlwaysEmpty, daq::True, "SettingsId");

    ASSERT_EQ(reference, createType(Policy::OneAlwaysEmpty, daq::True, "SettingsId"));
    ASSERT_NE(reference, createType(Policy::Unspecified, daq::True, "SettingsId"));
    ASSERT_NE(reference, createType(Policy::Fixed, daq::True, "SettingsId"));
    ASSERT_NE(reference, createType(Policy::SettingsDriven, daq::True, "SettingsId"));
    ASSERT_NE(reference, createType(Policy::OneAlwaysEmpty, daq::False, "SettingsId"));
    ASSERT_NE(reference, createType(Policy::OneAlwaysEmpty, daq::True, "OtherId"));
    ASSERT_NE(reference, createType(Policy::OneAlwaysEmpty, daq::True, nullptr));
}

TEST_F(FunctionBlockTest, FunctionBlockTypeStructNames)
{
    const auto structType = daq::FunctionBlockTypeStructType();
    const daq::StructPtr structPtr = daq::FunctionBlockType("Id", "Name", "Desc");
    ASSERT_EQ(structType.getFieldNames(), structPtr.getFieldNames());
}

TEST_F(FunctionBlockTest, FunctionBlockTypeStructNamesWithOptions)
{
    const auto structType = daq::FunctionBlockTypeStructType();
    const daq::StructPtr structPtr = daq::FunctionBlockTypeBuilder()
                                         .setId("Id")
                                         .setName("Name")
                                         .setDescription("Desc")
                                         .setSingleton(daq::True)
                                         .setCommonSettingsTypeId("SettingsId")
                                         .build();

    ASSERT_EQ(structType, structPtr.getStructType());
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

    // Default options are left out of the serialized form
    const std::string json = serializedFbType;
    ASSERT_EQ(json.find("inputPolicy"), std::string::npos);
    ASSERT_EQ(json.find("singleton"), std::string::npos);
    ASSERT_EQ(json.find("commonSettingsTypeId"), std::string::npos);

    const auto deserializer = daq::JsonDeserializer();
    const daq::FunctionBlockTypePtr newFbType = deserializer.deserialize(serializedFbType);

    ASSERT_EQ(newFbType.getId(), fbType.getId());
    ASSERT_EQ(newFbType.getName(), fbType.getName());
    ASSERT_EQ(newFbType.getDescription(), fbType.getDescription());

    const auto newDefConfig = newFbType.createDefaultConfig();
    ASSERT_EQ(defConfig.getPropertyValue("cfg"), newDefConfig.getPropertyValue("cfg"));

    ASSERT_EQ(newFbType.getInputPolicy(), fbType.getInputPolicy());
    ASSERT_EQ(newFbType.getSingleton(), fbType.getSingleton());
    ASSERT_EQ(newFbType.getCommonSettingsTypeId(), fbType.getCommonSettingsTypeId());
}

TEST_F(FunctionBlockTest, FunctionBlockTypeDefaults)
{
    const daq::FunctionBlockTypePtr fbType = daq::FunctionBlockType("Id", "Name", "Desc");

    ASSERT_EQ(fbType.getInputPolicy(), daq::FunctionBlockInputPolicy::Unspecified);
    ASSERT_FALSE(fbType.getSingleton());
    ASSERT_FALSE(fbType.getCommonSettingsTypeId().assigned());
}

TEST_F(FunctionBlockTest, FunctionBlockTypeOptionsSerializationDeserialization)
{
    const daq::FunctionBlockTypePtr fbType = daq::FunctionBlockTypeBuilder()
                                                 .setId("Id")
                                                 .setName("Name")
                                                 .setDescription("Desc")
                                                 .setInputPolicy(daq::FunctionBlockInputPolicy::OneAlwaysEmpty)
                                                 .setSingleton(daq::True)
                                                 .setCommonSettingsTypeId("SettingsId")
                                                 .build();

    const auto serializer = daq::JsonSerializer();
    fbType.serialize(serializer);

    const auto deserializer = daq::JsonDeserializer();
    const daq::FunctionBlockTypePtr newFbType = deserializer.deserialize(serializer.getOutput());

    ASSERT_EQ(newFbType.getInputPolicy(), daq::FunctionBlockInputPolicy::OneAlwaysEmpty);
    ASSERT_TRUE(newFbType.getSingleton());
    ASSERT_EQ(newFbType.getCommonSettingsTypeId(), "SettingsId");

    const daq::StructPtr newStruct = newFbType;
    ASSERT_EQ(newStruct.get("InputPolicy"), static_cast<daq::Int>(daq::FunctionBlockInputPolicy::OneAlwaysEmpty));
    ASSERT_EQ(newStruct.get("Singleton"), true);
    ASSERT_EQ(newStruct.get("CommonSettingsTypeId"), "SettingsId");
    ASSERT_EQ(newFbType, fbType);
}

TEST_F(FunctionBlockTest, FunctionBlockTypeInputPolicySerializationDeserialization)
{
    for (const auto policy : {daq::FunctionBlockInputPolicy::Unspecified,
                            daq::FunctionBlockInputPolicy::Fixed,
                            daq::FunctionBlockInputPolicy::OneAlwaysEmpty,
                            daq::FunctionBlockInputPolicy::SettingsDriven})
    {
        const daq::FunctionBlockTypePtr fbType = daq::FunctionBlockTypeBuilder()
                                                     .setId("Id")
                                                     .setName("Name")
                                                     .setDescription("Desc")
                                                     .setInputPolicy(policy)
                                                     .build();

        const auto serializer = daq::JsonSerializer();
        fbType.serialize(serializer);

        const auto deserializer = daq::JsonDeserializer();
        const daq::FunctionBlockTypePtr newFbType = deserializer.deserialize(serializer.getOutput());

        ASSERT_EQ(newFbType.getInputPolicy(), policy);
        ASSERT_EQ(newFbType, fbType);
    }
}

TEST_F(FunctionBlockTest, FunctionBlockTypeUnknownInputPolicyDeserializesAsUnspecified)
{
    const auto json = R"({"__type":"FunctionBlockType","id":"Id","name":"Name","inputPolicy":42})";

    const auto deserializer = daq::JsonDeserializer();
    const daq::FunctionBlockTypePtr fbType = deserializer.deserialize(json);

    ASSERT_EQ(fbType.getInputPolicy(), daq::FunctionBlockInputPolicy::Unspecified);
}

TEST_F(FunctionBlockTest, FunctionBlockTypeBuilderRejectsUnknownInputPolicy)
{
    const auto builder = daq::FunctionBlockTypeBuilder().setId("Id").setInputPolicy(static_cast<daq::FunctionBlockInputPolicy>(42));

    ASSERT_THROW(builder.build(), daq::InvalidParameterException);
}

TEST_F(FunctionBlockTest, FunctionBlockTypeRejectsUnknownInputPolicy)
{
    ASSERT_THROW((daq::createWithImplementation<daq::IFunctionBlockType, daq::FunctionBlockTypeImpl>(
                     "Id", "Name", "Desc", nullptr, static_cast<daq::FunctionBlockInputPolicy>(-1), daq::False, nullptr)),
                 daq::InvalidParameterException);
}

TEST_F(FunctionBlockTest, FunctionBlockTypeEmptyCommonSettingsTypeIdIsUnassigned)
{
    const auto builder = daq::FunctionBlockTypeBuilder().setId("Id").setCommonSettingsTypeId("");
    ASSERT_FALSE(builder.getCommonSettingsTypeId().assigned());

    const daq::FunctionBlockTypePtr fbType = builder.build();
    ASSERT_FALSE(fbType.getCommonSettingsTypeId().assigned());
    ASSERT_FALSE(fbType.asPtr<daq::IStruct>().get("CommonSettingsTypeId").assigned());
    ASSERT_EQ(fbType, daq::FunctionBlockTypeBuilder().setId("Id").build());

    const daq::FunctionBlockTypePtr direct = daq::createWithImplementation<daq::IFunctionBlockType, daq::FunctionBlockTypeImpl>(
        "Id", "Name", "Desc", nullptr, daq::FunctionBlockInputPolicy::Unspecified, daq::False, "");
    ASSERT_FALSE(direct.getCommonSettingsTypeId().assigned());
}

TEST_F(FunctionBlockTest, FunctionBlockTypeEmptyCommonSettingsTypeIdDeserializesAsUnassigned)
{
    const auto json = R"({"__type":"FunctionBlockType","id":"Id","name":"Name","commonSettingsTypeId":""})";

    const auto deserializer = daq::JsonDeserializer();
    const daq::FunctionBlockTypePtr fbType = deserializer.deserialize(json);

    ASSERT_FALSE(fbType.getCommonSettingsTypeId().assigned());
}


namespace
{
    class SingletonTestParentFbImpl : public daq::FunctionBlock
    {
    public:
        SingletonTestParentFbImpl(const daq::ContextPtr& ctx, const daq::StringPtr& localId, bool singleton)
            : daq::FunctionBlock(daq::FunctionBlockType("Parent", "Parent", ""), ctx, nullptr, localId)
            , singleton(singleton)
        {
        }

        daq::DictPtr<daq::IString, daq::IFunctionBlockType> onGetAvailableFunctionBlockTypes() override
        {
            return daq::Dict<daq::IString, daq::IFunctionBlockType>(
                {{"Single", daq::FunctionBlockTypeBuilder().setId("Single").setName("Single").setSingleton(singleton).build()},
                 {"Multi", daq::FunctionBlockType("Multi", "Multi", "")},
                 // Module code that adds an instance of "Single" itself instead of through addFunctionBlock("Single")
                 {"SingleViaNested", daq::FunctionBlockType("SingleViaNested", "SingleViaNested", "")}});
        }

        daq::FunctionBlockPtr onAddFunctionBlock(const daq::StringPtr& typeId, const daq::PropertyObjectPtr& /*config*/) override
        {
            const auto types = onGetAvailableFunctionBlockTypes();
            const auto localId = typeId.toStdString() + std::to_string(nestedCount++);

            return addNested(typeId == "SingleViaNested" ? types.get("Single") : types.get(typeId), localId);
        }

    private:
        daq::FunctionBlockPtr addNested(const daq::FunctionBlockTypePtr& type, const std::string& localId)
        {
            auto fb = daq::createWithImplementation<daq::IFunctionBlock, daq::FunctionBlock>(type, context, functionBlocks, localId);
            addNestedFunctionBlock(fb);
            return fb;
        }

        std::atomic<int> nestedCount = 0;
        bool singleton;
    };

    daq::FunctionBlockPtr createSingletonTestParent(const daq::StringPtr& localId = "parent", bool singleton = true)
    {
        return daq::createWithImplementation<daq::IFunctionBlock, SingletonTestParentFbImpl>(daq::NullContext(), localId, singleton);
    }

    size_t countFunctionBlocksOfType(const daq::FunctionBlockPtr& parent, const std::string& typeId)
    {
        size_t count = 0;
        for (const auto& fb : parent.getFunctionBlocks())
        {
            if (fb.getFunctionBlockType().getId() == typeId)
                ++count;
        }
        return count;
    }

    // A setup with two instances of the singleton type, as saved from a parent whose type lacks the flag
    daq::StringPtr serializeSetupWithDuplicateSingleton()
    {
        const auto source = createSingletonTestParent("parent", false);
        source.addFunctionBlock("Single");
        source.addFunctionBlock("Single");
        source.addFunctionBlock("Multi");

        const auto serializer = daq::JsonSerializer();
        source.serialize(serializer);
        return serializer.getOutput();
    }

    void loadSetup(const daq::FunctionBlockPtr& target, const daq::StringPtr& setup)
    {
        daq::JsonDeserializer().update(target.asPtr<daq::IUpdatable>(true), setup, daq::UpdateParameters());
    }
}

TEST_F(FunctionBlockTest, SingletonFunctionBlockAddedOnlyOnce)
{
    const auto parent = createSingletonTestParent();

    ASSERT_NO_THROW(parent.addFunctionBlock("Single"));
    ASSERT_THROW(parent.addFunctionBlock("Single"), daq::AlreadyExistsException);
    ASSERT_EQ(parent.getFunctionBlocks().getCount(), 1u);
}

TEST_F(FunctionBlockTest, SingletonFunctionBlockErrorCode)
{
    const auto parent = createSingletonTestParent();
    parent.addFunctionBlock("Single");

    daq::FunctionBlockPtr fb;
    ASSERT_EQ(parent->addFunctionBlock(&fb, daq::String("Single"), nullptr), OPENDAQ_ERR_ALREADYEXISTS);
    ASSERT_FALSE(fb.assigned());
    daqClearErrorInfo();
}

TEST_F(FunctionBlockTest, SingletonFunctionBlockAddedAgainAfterRemoval)
{
    const auto parent = createSingletonTestParent();

    const auto fb = parent.addFunctionBlock("Single");
    parent.removeFunctionBlock(fb);

    ASSERT_NO_THROW(parent.addFunctionBlock("Single"));
    ASSERT_EQ(parent.getFunctionBlocks().getCount(), 1u);
}

TEST_F(FunctionBlockTest, SingletonFunctionBlockPerParent)
{
    const auto parent1 = createSingletonTestParent("parent1");
    const auto parent2 = createSingletonTestParent("parent2");

    ASSERT_NO_THROW(parent1.addFunctionBlock("Single"));
    ASSERT_NO_THROW(parent2.addFunctionBlock("Single"));
}

TEST_F(FunctionBlockTest, SingletonDoesNotLimitOtherTypes)
{
    const auto parent = createSingletonTestParent();

    parent.addFunctionBlock("Single");
    ASSERT_NO_THROW(parent.addFunctionBlock("Multi"));
    ASSERT_NO_THROW(parent.addFunctionBlock("Multi"));
    ASSERT_EQ(parent.getFunctionBlocks().getCount(), 3u);
}

TEST_F(FunctionBlockTest, SingletonAddedByAddNestedFunctionBlockIsRefused)
{
    const auto parent = createSingletonTestParent();
    parent.addFunctionBlock("Single");

    ASSERT_THROW(parent.addFunctionBlock("SingleViaNested"), daq::AlreadyExistsException);
    ASSERT_EQ(countFunctionBlocksOfType(parent, "Single"), 1u);
}

TEST_F(FunctionBlockTest, SingletonAddedToFolderIsRefused)
{
    const auto parent = createSingletonTestParent();
    const auto existing = parent.addFunctionBlock("Single");

    const auto folder = parent.getItem("FB").asPtr<daq::IFolderConfig>(true);
    const auto second = daq::createWithImplementation<daq::IFunctionBlock, daq::FunctionBlock>(
        existing.getFunctionBlockType(), daq::NullContext(), folder, "Single_second");

    ASSERT_EQ(folder->addItem(second), OPENDAQ_ERR_ALREADYEXISTS);
    daqClearErrorInfo();
    ASSERT_EQ(countFunctionBlocksOfType(parent, "Single"), 1u);
}

TEST_F(FunctionBlockTest, LoadSkipsDuplicateSingleton)
{
    const auto target = createSingletonTestParent();

    ASSERT_NO_THROW(loadSetup(target, serializeSetupWithDuplicateSingleton()));
    ASSERT_EQ(countFunctionBlocksOfType(target, "Single"), 1u);
    ASSERT_EQ(countFunctionBlocksOfType(target, "Multi"), 1u);
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

TEST_F(FunctionBlockTest, SerializeCarriesTypeWithoutDefaultConfig)
{
    const auto defaultConfig = daq::PropertyObject();
    defaultConfig.addProperty(daq::StringProperty("DefaultConfigOnly", "value"));
    const auto type = daq::FunctionBlockType("test_uid", "test_name", "test_description", defaultConfig);
    const auto fb = daq::createWithImplementation<daq::IFunctionBlock, daq::FunctionBlock>(type, daq::NullContext(), nullptr, "fb");

    const auto serializer = daq::JsonSerializer();
    fb.serialize(serializer);
    const std::string json = serializer.getOutput();
    ASSERT_EQ(json.find("DefaultConfigOnly"), std::string::npos);

    const auto deserializeContext = daq::ComponentDeserializeContext(daq::NullContext(), nullptr, nullptr, "fb");
    const daq::FunctionBlockPtr newFb = daq::JsonDeserializer().deserialize(json, deserializeContext, nullptr);
    const auto newType = newFb.getFunctionBlockType();
    ASSERT_EQ(newType.getId(), "test_uid");
    ASSERT_EQ(newType.getName(), "test_name");
    ASSERT_EQ(newType.getDescription(), "test_description");
    ASSERT_EQ(newType.createDefaultConfig().getAllProperties().getCount(), 0u);
}

TEST_F(FunctionBlockTest, DeserializeWithoutComponentType)
{
    const auto fb = daq::createWithImplementation<daq::IFunctionBlock, daq::FunctionBlock>(
        daq::FunctionBlockType("test_uid", "test_name", "test_description"), daq::NullContext(), nullptr, "fb");

    const auto serializer = daq::JsonSerializer();
    fb.serialize(serializer);
    std::string json = serializer.getOutput();
    const auto keyPos = json.find("\"componentType\"");
    ASSERT_NE(keyPos, std::string::npos);
    json.replace(keyPos, std::string("\"componentType\"").size(), "\"unknownKey\"");

    const auto deserializeContext = daq::ComponentDeserializeContext(daq::NullContext(), nullptr, nullptr, "fb");
    const daq::FunctionBlockPtr newFb = daq::JsonDeserializer().deserialize(json, deserializeContext, nullptr);
    ASSERT_EQ(newFb.getFunctionBlockType().getId(), "test_uid");
    ASSERT_EQ(newFb.getFunctionBlockType().getName(), "test_uid");
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

TEST_F(FunctionBlockTest, CyclicRefCheck)
{
    const auto fb = daq::createWithImplementation<daq::IFunctionBlock, MockFbImpl>(daq::NullContext(), nullptr, "fb", nullptr, false);
    ASSERT_THROW(fb.getInputPorts()[0].connect(fb.getSignals()[0]), daq::CyclicReferenceException);
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
        auto lock = getAcquisitionLock2();
    }

    std::unordered_set<std::string> getDefaultComponents()
    {
        return this->defaultComponents;
    }
};

// Hangs on debug: self-deadlock in the debug object-tracking infra — stringifying a tracked
// EvalValue inside daqPrintTrackedObjects re-enters the same non-recursive mutex via
// daqTrackObject. Not specific to this test; any test leaving such an object alive at
// OnTestEnd can trigger it.
TEST_F_UNSTABLE_SKIPPED(FunctionBlockTest, SetDomainDescriptorUnderLock)
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

TEST_F(FunctionBlockTest, DefaultFolderLockingStrategy)
{
    const auto logger = daq::Logger();
    auto context = daq::Context(daq::Scheduler(logger), logger, daq::TypeManager(), nullptr, nullptr);
    auto fb = daq::createWithImplementation<daq::IFunctionBlock, MockFbImpl1>(context);

    auto testFbImpl = dynamic_cast<MockFbImpl1*>(fb.getObject());
    auto defaultComponents = testFbImpl->getDefaultComponents();
    for (const daq::ComponentPtr& component : fb.getItems())
    {
        if (defaultComponents.count(component.getName()))
            ASSERT_EQ(component.asPtr<daq::IPropertyObjectInternal>().getLockingStrategy(), daq::LockingStrategy::ForwardOwnerLockOwn);
    }
}
