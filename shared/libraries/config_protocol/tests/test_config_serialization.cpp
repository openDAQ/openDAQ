#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <opendaq/context_factory.h>
#include <opendaq/component_factory.h>
#include <opendaq/folder_factory.h>
#include <opendaq/input_port_factory.h>
#include <opendaq/io_folder_factory.h>
#include <opendaq/function_block_impl.h>
#include <opendaq/signal_factory.h>
#include <opendaq/channel_impl.h>
#include <opendaq/device_impl.h>
#include <config_protocol/config_protocol_client.h>
#include <config_protocol/config_client_component_impl.h>
#include <config_protocol/config_protocol_deserialize_context_impl.h>
#include <config_protocol/config_client_folder_impl.h>
#include <config_protocol/config_client_io_folder_impl.h>
#include <config_protocol/config_client_input_port_impl.h>
#include <config_protocol/config_client_function_block_impl.h>
#include <config_protocol/config_client_channel_impl.h>
#include <config_protocol/config_client_signal_impl.h>
#include <config_protocol/config_client_device_impl.h>
#include <coreobjects/callable_info_factory.h>
#include <coreobjects/argument_info_factory.h>
#include <config_protocol/config_client_device_impl.h>
#include <coreobjects/property_object_class_factory.h>

using namespace daq;
using namespace config_protocol;

using ConfigProtocolSerializationTest = testing::Test;

TEST_F(ConfigProtocolSerializationTest, PropertyObject)
{
    const auto propObj = PropertyObject();
    propObj.addProperty(StringPropertyBuilder("StringProperty", "-").build());
    propObj.setPropertyValue("StringProperty", "value");

    const auto serializer = JsonSerializer(True);
    propObj.serialize(serializer);
    const auto str1 = serializer.getOutput();

    const auto deserializer = JsonDeserializer();

    auto clientComm = std::make_shared<ConfigProtocolClientComm>(NullContext(), nullptr, nullptr);

    bool configPropertyObjectInstantiated = false;
    const PropertyObjectPtr newPropObj = deserializer.deserialize(
        str1,
        nullptr,
        [&configPropertyObjectInstantiated](
           const StringPtr& typeId,
           const SerializedObjectPtr& serObj,
           const BaseObjectPtr& context,
           const FunctionPtr& factoryCallback) -> BaseObjectPtr
        {
             if (typeId == "PropertyObject")
             {
                  BaseObjectPtr obj;
                  checkErrorInfo(PropertyObjectImpl::Deserialize(serObj, context, factoryCallback, &obj));
                  configPropertyObjectInstantiated = true;
                  return obj;
             }

             return nullptr;
         });

    ASSERT_TRUE(configPropertyObjectInstantiated);

    const auto serializer2 = JsonSerializer(True);
    newPropObj.serialize(serializer2);
    const auto str2 = serializer2.getOutput();

    ASSERT_EQ(str1, str2);
}

TEST_F(ConfigProtocolSerializationTest, Function)
{
    const auto propObj = PropertyObject();
    propObj.addProperty(FunctionPropertyBuilder("Proc", ProcedureInfo(List<IArgumentInfo>(ArgumentInfo("P1", ctInt))))
                            .setReadOnly(True)
                            .build());
    propObj.asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue("Proc", Procedure([](Int p1) {}));

    const auto serializer = JsonSerializer(True);
    propObj.serialize(serializer);
    const auto str1 = serializer.getOutput();

    const auto deserializer = JsonDeserializer();

    auto clientComm = std::make_shared<ConfigProtocolClientComm>(NullContext(), nullptr, nullptr);

    bool configPropertyObjectInstantiated = false;
    const PropertyObjectPtr newPropObj =
        deserializer.deserialize(str1,
                                 TypeManager(),
                                 [&configPropertyObjectInstantiated](const StringPtr& typeId,
                                                                     const SerializedObjectPtr& serObj,
                                                                     const BaseObjectPtr& context,
                                                                     const FunctionPtr& factoryCallback) -> BaseObjectPtr
                                 {
                                     if (typeId == "PropertyObject")
                                     {
                                         BaseObjectPtr obj;
                                         checkErrorInfo(PropertyObjectImpl::Deserialize(serObj, context, factoryCallback, &obj));
                                         configPropertyObjectInstantiated = true;
                                         return obj;
                                     }

                                     return nullptr;
                                 });

    ASSERT_TRUE(configPropertyObjectInstantiated);

    const auto serializer2 = JsonSerializer(True);
    newPropObj.serialize(serializer2);
    const auto str2 = serializer2.getOutput();

    ASSERT_EQ(str1, str2);
}

TEST_F(ConfigProtocolSerializationTest, Component)
{
    const auto ctx = NullContext();
    const auto name = "foo";
    const auto desc = "bar";
    const auto component = Component(ctx, nullptr, "temp");

    component.setName(name);
    component.setDescription(desc);
    component.getTags().asPtr<ITagsPrivate>().add("tag");

    const auto serializer = JsonSerializer(True);
    component.serialize(serializer);
    const auto str1 = serializer.getOutput();

    const auto deserializer = JsonDeserializer();

    auto clientComm = std::make_shared<ConfigProtocolClientComm>(ctx, nullptr, nullptr);

    const auto deserializeContext = createWithImplementation<IConfigProtocolDeserializeContext, ConfigProtocolDeserializeContextImpl>(
        clientComm, std::string {}, ctx, nullptr, nullptr, "temp", nullptr);
    bool configComponentInstantiated = false;
    const ComponentPtr newComponent = deserializer.deserialize(str1,
                                                               deserializeContext,
                                                               [&configComponentInstantiated](const StringPtr& typeId,
                                                               const SerializedObjectPtr& serObj,
                                                               const BaseObjectPtr& context,
                                                               const FunctionPtr& factoryCallback) -> BaseObjectPtr
        {
            if (typeId == "Component")
            {
                BaseObjectPtr obj;
                checkErrorInfo(ConfigClientComponentImpl::Deserialize(serObj, context, factoryCallback, &obj));
                configComponentInstantiated = true;
                return obj;
            }

            return nullptr;
        });

    ASSERT_TRUE(configComponentInstantiated);
    ASSERT_EQ(newComponent.getName(), name);
    ASSERT_EQ(newComponent.getDescription(), desc);
    ASSERT_EQ(newComponent.getTags(), component.getTags());

    const auto serializer2 = JsonSerializer(True);
    newComponent.serialize(serializer2);
    const auto str2 = serializer2.getOutput();

    ASSERT_EQ(str1, str2);
}

TEST_F(ConfigProtocolSerializationTest, FolderWithType)
{
    const auto ctx = NullContext();
    const auto folder = Folder(ctx, nullptr, "folder");
    folder.setName("folder_name");
    folder.setDescription("folder_desc");
    folder.getTags().asPtr<ITagsPrivate>().add("folder_tag");

    const auto serializer = JsonSerializer(True);
    folder.serialize(serializer);
    const auto str1 = serializer.getOutput();

    const auto deserializer = JsonDeserializer();

    auto clientComm = std::make_shared<ConfigProtocolClientComm>(ctx, nullptr, nullptr);

    IntfID intfId = IDevice::Id;

    const auto deserializeContext = createWithImplementation<IConfigProtocolDeserializeContext, ConfigProtocolDeserializeContextImpl>(
        clientComm, std::string{}, ctx, nullptr, nullptr, "folder", &intfId);
    int configComponentInstantiated = 0;
    const FolderPtr newFolder =
        deserializer.deserialize(str1,
                                 deserializeContext,
                                 [&configComponentInstantiated](const StringPtr& typeId,
                                                                const SerializedObjectPtr& serObj,
                                                                const BaseObjectPtr& context,
                                                                const FunctionPtr& factoryCallback) -> BaseObjectPtr
                                 {
                                     if (typeId == "Folder")
                                     {
                                         BaseObjectPtr obj;
                                         checkErrorInfo(ConfigClientFolderImpl::Deserialize(serObj, context, factoryCallback, &obj));
                                         configComponentInstantiated++;
                                         return obj;
                                     }

                                     return nullptr;
                                 });

    ASSERT_EQ(configComponentInstantiated, 1);
    ASSERT_EQ(newFolder.getName(), folder.getName());
    ASSERT_EQ(newFolder.getDescription(), folder.getDescription());
    ASSERT_EQ(newFolder.getTags(), folder.getTags());

    ASSERT_EQ(newFolder.getItems().getElementInterfaceId(), IDevice::Id);

    const auto serializer2 = JsonSerializer(True);
    newFolder.serialize(serializer2);
    const auto str2 = serializer2.getOutput();

    ASSERT_EQ(str1, str2);
}

TEST_F(ConfigProtocolSerializationTest, FolderWithComponent)
{
    const auto ctx = NullContext();
    const auto folder = Folder(ctx, nullptr, "folder");
    folder.setName("folder_name");
    folder.setDescription("folder_desc");
    folder.getTags().asPtr<ITagsPrivate>().add("folder_tag");

    const auto component = Component(ctx, folder, "component");
    component.setName("comp_name");
    component.setDescription("comp_desc");
    component.getTags().asPtr<ITagsPrivate>().add("comp_tag");

    folder.addItem(component);

    const auto serializer = JsonSerializer(True);
    folder.serialize(serializer);
    const auto str1 = serializer.getOutput();

    const auto deserializer = JsonDeserializer();

    auto clientComm = std::make_shared<ConfigProtocolClientComm>(ctx, nullptr, nullptr);

    const auto deserializeContext = createWithImplementation<IConfigProtocolDeserializeContext, ConfigProtocolDeserializeContextImpl>(
        clientComm, std::string {}, ctx, nullptr, nullptr, "folder", nullptr);
    int configComponentInstantiated = 0;
    const ComponentPtr newFolder =
        deserializer.deserialize(str1,
                                 deserializeContext,
                                 [&configComponentInstantiated](const StringPtr& typeId,
                                                                const SerializedObjectPtr& serObj,
                                                                const BaseObjectPtr& context,
                                                                const FunctionPtr& factoryCallback) -> BaseObjectPtr
                                 {
                                     if (typeId == "Folder")
                                     {
                                         BaseObjectPtr obj;
                                         checkErrorInfo(ConfigClientFolderImpl::Deserialize(serObj, context, factoryCallback, &obj));
                                         configComponentInstantiated++;
                                         return obj;
                                     }

                                     if (typeId == "Component")
                                     {
                                         BaseObjectPtr obj;
                                         checkErrorInfo(ConfigClientComponentImpl::Deserialize(serObj, context, factoryCallback, &obj));
                                         configComponentInstantiated++;
                                         return obj;
                                     }

                                     return nullptr;
                                 });

    ASSERT_EQ(configComponentInstantiated, 2);
    ASSERT_EQ(newFolder.getName(), folder.getName());
    ASSERT_EQ(newFolder.getDescription(), folder.getDescription());
    ASSERT_EQ(newFolder.getTags(), folder.getTags());

    const auto serializer2 = JsonSerializer(True);
    newFolder.serialize(serializer2);
    const auto str2 = serializer2.getOutput();

    ASSERT_EQ(str1, str2);
}

TEST_F(ConfigProtocolSerializationTest, IoFolderWithComponent)
{
    const auto ctx = NullContext();
    const auto folder = IoFolder(ctx, nullptr, "folder");
    folder.setName("folder_name");
    folder.setDescription("folder_desc");
    folder.getTags().asPtr<ITagsPrivate>().add("folder_tag");

    const auto subFolder = IoFolder(ctx, folder, "subfolder");
    subFolder.setName("sfld_name");
    subFolder.setDescription("sfld_desc");
    subFolder.getTags().asPtr<ITagsPrivate>().add("sfld_tag");

    folder.addItem(subFolder);

    const auto serializer = JsonSerializer(True);
    folder.serialize(serializer);
    const auto str1 = serializer.getOutput();

    const auto deserializer = JsonDeserializer();

    auto clientComm = std::make_shared<ConfigProtocolClientComm>(ctx, nullptr, nullptr);

    const auto deserializeContext = createWithImplementation<IConfigProtocolDeserializeContext, ConfigProtocolDeserializeContextImpl>(
        clientComm, std::string{}, ctx, nullptr, nullptr, "folder", nullptr);
    int configComponentInstantiated = 0;
    const ComponentPtr newFolder =
        deserializer.deserialize(str1,
                                 deserializeContext,
                                 [&configComponentInstantiated](const StringPtr& typeId,
                                                                const SerializedObjectPtr& serObj,
                                                                const BaseObjectPtr& context,
                                                                const FunctionPtr& factoryCallback) -> BaseObjectPtr
                                 {
                                     if (typeId == "IoFolder")
                                     {
                                         BaseObjectPtr obj;
                                         checkErrorInfo(ConfigClientIoFolderImpl::Deserialize(serObj, context, factoryCallback, &obj));
                                         configComponentInstantiated++;
                                         return obj;
                                     }
                                     return nullptr;
                                 });

    ASSERT_EQ(configComponentInstantiated, 2);
    ASSERT_EQ(newFolder.getName(), folder.getName());
    ASSERT_EQ(newFolder.getDescription(), folder.getDescription());
    ASSERT_EQ(newFolder.getTags(), folder.getTags());

    const auto serializer2 = JsonSerializer(True);
    newFolder.serialize(serializer2);
    const auto str2 = serializer2.getOutput();

    ASSERT_EQ(str1, str2);
}

TEST_F(ConfigProtocolSerializationTest, InputPort)
{
    const auto inputPort = InputPort(NullContext(), nullptr, "ip");

    inputPort.setName("ip_name");
    inputPort.getTags().asPtr<ITagsPrivate>().add("tag");

    const auto serializer = JsonSerializer(True);
    inputPort.serialize(serializer);
    const auto str1 = serializer.getOutput();

    const auto deserializer = JsonDeserializer();

    auto clientComm = std::make_shared<ConfigProtocolClientComm>(NullContext(), nullptr, nullptr);

    const auto deserializeContext = createWithImplementation<IConfigProtocolDeserializeContext, ConfigProtocolDeserializeContextImpl>(
        clientComm, std::string {}, NullContext(), nullptr, nullptr, "ip", nullptr);
    bool configComponentInstantiated = false;
    const InputPortPtr newInputPort = deserializer.deserialize(str1,
        deserializeContext,
        [&configComponentInstantiated](
             const StringPtr& typeId,
             const SerializedObjectPtr& serObj,
             const BaseObjectPtr& context,
             const FunctionPtr& factoryCallback) -> BaseObjectPtr
             {
                 if (typeId == "InputPort")
                 {
                     BaseObjectPtr obj;
                     checkErrorInfo(ConfigClientInputPortImpl::Deserialize(serObj, context, factoryCallback, &obj));
                     configComponentInstantiated = true;
                     return obj;
                 }

                 return nullptr;
             });

    ASSERT_TRUE(configComponentInstantiated);
    ASSERT_EQ(newInputPort.getName(), inputPort.getName());
    ASSERT_EQ(newInputPort.getTags(), inputPort.getTags());

    const auto serializer2 = JsonSerializer(True);
    newInputPort.serialize(serializer2);
    const auto str2 = serializer2.getOutput();

    ASSERT_EQ(str1, str2);
}

TEST_F(ConfigProtocolSerializationTest, Signal)
{
    const auto signal = Signal(NullContext(), nullptr, "sig");

    signal.setName("sig_name");
    signal.getTags().asPtr<ITagsPrivate>().add("sig_tag");

    const auto serializer = JsonSerializer(True);
    signal.serialize(serializer);
    const auto str1 = serializer.getOutput();

    const auto deserializer = JsonDeserializer();

    auto clientComm = std::make_shared<ConfigProtocolClientComm>(NullContext(), nullptr, nullptr);

    const auto deserializeContext = createWithImplementation<IConfigProtocolDeserializeContext, ConfigProtocolDeserializeContextImpl>(
        clientComm, std::string {}, NullContext(), nullptr, nullptr, "sig", nullptr);
    bool configComponentInstantiated = false;
    const SignalPtr newSignal =
        deserializer.deserialize(str1,
                                 deserializeContext,
                                 [&configComponentInstantiated](const StringPtr& typeId,
                                                                const SerializedObjectPtr& serObj,
                                                                const BaseObjectPtr& context,
                                                                const FunctionPtr& factoryCallback) -> BaseObjectPtr
                                 {
                                     if (typeId == "Signal")
                                     {
                                         BaseObjectPtr obj;
                                         checkErrorInfo(ConfigClientSignalImpl::Deserialize(serObj, context, factoryCallback, &obj));
                                         configComponentInstantiated = true;
                                         return obj;
                                     }

                                     return nullptr;
                                 });

    ASSERT_TRUE(configComponentInstantiated);
    ASSERT_EQ(newSignal.getName(), signal.getName());
    ASSERT_EQ(newSignal.getTags(), signal.getTags());

    const auto serializer2 = JsonSerializer(True);
    newSignal.serialize(serializer2);
    const auto str2 = serializer2.getOutput();

    ASSERT_EQ(str1, str2);
}

class MockFbImpl final : public FunctionBlock
{
public:
    MockFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId, const StringPtr& className, bool nested)
        : FunctionBlock(FunctionBlockType("test_uid", "test_name", "test_description"), ctx, parent, localId, className)
    {
        createAndAddSignal("sig1");
        createAndAddInputPort("ip1", PacketReadyNotification::None);
        if (!nested)
        {
            const auto nestedFb =
                createWithImplementation<IFunctionBlock, MockFbImpl>(ctx, this->functionBlocks, "nestedFb", className, true);
            addNestedFunctionBlock(nestedFb);
        }
    }
};

TEST_F(ConfigProtocolSerializationTest, FunctionBlock)
{
    const auto fb = createWithImplementation<IFunctionBlock, MockFbImpl>(NullContext(), nullptr, "fb", nullptr, false);
    fb.setName("fb_name");
    fb.setDescription("fb_desc");
    fb.getTags().asPtr<ITagsPrivate>().add("fld_tag");

    const auto serializer = JsonSerializer(True);
    fb.serialize(serializer);
    const auto str1 = serializer.getOutput();

    const auto deserializer = JsonDeserializer();

    auto clientComm = std::make_shared<ConfigProtocolClientComm>(NullContext(), nullptr, nullptr);

    const auto deserializeContext = createWithImplementation<IConfigProtocolDeserializeContext, ConfigProtocolDeserializeContextImpl>(
        clientComm, std::string {} , NullContext(), nullptr, nullptr, "fb", nullptr);

    int configComponentInstantiated = 0;
    const FunctionBlockPtr newFunctionBlock =
        deserializer.deserialize(str1,
                                 deserializeContext,
                                 [&configComponentInstantiated](const StringPtr& typeId,
                                                                const SerializedObjectPtr& serObj,
                                                                const BaseObjectPtr& context,
                                                                const FunctionPtr& factoryCallback) -> BaseObjectPtr
                                 {
                                     if (typeId == "Folder")
                                     {
                                         BaseObjectPtr obj;
                                         checkErrorInfo(ConfigClientFolderImpl::Deserialize(serObj, context, factoryCallback, &obj));
                                         configComponentInstantiated++;
                                         return obj;
                                     }

                                     if (typeId == "InputPort")
                                     {
                                         BaseObjectPtr obj;
                                         checkErrorInfo(ConfigClientInputPortImpl::Deserialize(serObj, context, factoryCallback, &obj));
                                         configComponentInstantiated++;
                                         return obj;
                                     }

                                     if (typeId == "Signal")
                                     {
                                         BaseObjectPtr obj;
                                         checkErrorInfo(ConfigClientSignalImpl::Deserialize(serObj, context, factoryCallback, &obj));
                                         configComponentInstantiated++;
                                         return obj;
                                     }

                                     if (typeId == "FunctionBlock")
                                     {
                                         BaseObjectPtr obj;
                                         checkErrorInfo(ConfigClientFunctionBlockImpl::Deserialize(serObj, context, factoryCallback, &obj));
                                         configComponentInstantiated++;
                                         return obj;
                                     }

                                     return nullptr;
                                 });

    const FunctionBlockPtr newFb = deserializer.deserialize(str1, deserializeContext, nullptr);

    ASSERT_EQ(configComponentInstantiated, 12);
    ASSERT_EQ(newFb.getName(), fb.getName());
    ASSERT_EQ(newFb.getDescription(), fb.getDescription());
    ASSERT_EQ(newFb.getTags(), fb.getTags());

    const auto serializer2 = JsonSerializer(True);
    newFb.serialize(serializer2);
    const auto str2 = serializer2.getOutput();

    ASSERT_EQ(str1, str2);
}

TEST_F(ConfigProtocolSerializationTest, FunctionBlockWithClassName)
{
    const auto ctx = daq::NullContext();

    const auto fbClass =
        daq::PropertyObjectClassBuilder("FBClass").addProperty(daq::StringPropertyBuilder("StringProp", "-").build()).build();

    ctx.getTypeManager().addType(fbClass);

    const auto fb = createWithImplementation<IFunctionBlock, MockFbImpl>(ctx, nullptr, "fb", "FBClass", false);
    fb.setName("fb_name");
    fb.setDescription("fb_desc");
    fb.getTags().asPtr<ITagsPrivate>().add("fld_tag");
    fb.setPropertyValue("StringProp", "Value");

    const auto serializer = JsonSerializer(True);
    fb.serialize(serializer);
    const auto str1 = serializer.getOutput();

    const auto deserializer = JsonDeserializer();

    auto clientComm = std::make_shared<ConfigProtocolClientComm>(ctx, nullptr, nullptr);

    const auto deserializeContext = createWithImplementation<IConfigProtocolDeserializeContext, ConfigProtocolDeserializeContextImpl>(
        clientComm, std::string{}, ctx, nullptr, nullptr, "fb", nullptr);

    int configComponentInstantiated = 0;
    const FunctionBlockPtr newFunctionBlock =
        deserializer.deserialize(str1,
                                 deserializeContext,
                                 [&configComponentInstantiated](const StringPtr& typeId,
                                                                const SerializedObjectPtr& serObj,
                                                                const BaseObjectPtr& context,
                                                                const FunctionPtr& factoryCallback) -> BaseObjectPtr
                                 {
                                     if (typeId == "Folder")
                                     {
                                         BaseObjectPtr obj;
                                         checkErrorInfo(ConfigClientFolderImpl::Deserialize(serObj, context, factoryCallback, &obj));
                                         configComponentInstantiated++;
                                         return obj;
                                     }

                                     if (typeId == "InputPort")
                                     {
                                         BaseObjectPtr obj;
                                         checkErrorInfo(ConfigClientInputPortImpl::Deserialize(serObj, context, factoryCallback, &obj));
                                         configComponentInstantiated++;
                                         return obj;
                                     }

                                     if (typeId == "Signal")
                                     {
                                         BaseObjectPtr obj;
                                         checkErrorInfo(ConfigClientSignalImpl::Deserialize(serObj, context, factoryCallback, &obj));
                                         configComponentInstantiated++;
                                         return obj;
                                     }

                                     if (typeId == "FunctionBlock")
                                     {
                                         BaseObjectPtr obj;
                                         checkErrorInfo(ConfigClientFunctionBlockImpl::Deserialize(serObj, context, factoryCallback, &obj));
                                         configComponentInstantiated++;
                                         return obj;
                                     }

                                     return nullptr;
                                 });

    const FunctionBlockPtr newFb = deserializer.deserialize(str1, deserializeContext, nullptr);

    ASSERT_EQ(configComponentInstantiated, 12);
    ASSERT_EQ(newFb.getName(), fb.getName());
    ASSERT_EQ(newFb.getDescription(), fb.getDescription());
    ASSERT_EQ(newFb.getTags(), fb.getTags());

    const auto serializer2 = JsonSerializer(True);
    newFb.serialize(serializer2);
    const auto str2 = serializer2.getOutput();

    ASSERT_EQ(str1, str2);
}

class MockChannel final : public Channel
{
public:
    MockChannel(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
        : Channel(FunctionBlockType("Ch", "", ""), ctx, parent, localId)
    {
        createAndAddSignal("sig_ch");
    }
};

TEST_F(ConfigProtocolSerializationTest, Channel)
{
    const auto ch = createWithImplementation<IChannel, MockChannel>(NullContext(), nullptr, "Ch");
    ch.setName("fb_name");
    ch.setDescription("fb_desc");
    ch.getTags().asPtr<ITagsPrivate>().add("fld_tag");

    const auto serializer = JsonSerializer(True);
    ch.serialize(serializer);
    const auto str1 = serializer.getOutput();

    const auto deserializer = JsonDeserializer();

    auto clientComm = std::make_shared<ConfigProtocolClientComm>(NullContext(), nullptr, nullptr);

    const auto deserializeContext = createWithImplementation<IConfigProtocolDeserializeContext, ConfigProtocolDeserializeContextImpl>(
        clientComm, std::string {}, NullContext(), nullptr, nullptr, "Ch", nullptr);

    int configComponentInstantiated = 0;
    const ChannelPtr newCh =
        deserializer.deserialize(str1,
                                 deserializeContext,
                                 [&configComponentInstantiated](const StringPtr& typeId,
                                                                const SerializedObjectPtr& serObj,
                                                                const BaseObjectPtr& context,
                                                                const FunctionPtr& factoryCallback) -> BaseObjectPtr
                                 {
                                     if (typeId == "Folder")
                                     {
                                         BaseObjectPtr obj;
                                         checkErrorInfo(ConfigClientFolderImpl::Deserialize(serObj, context, factoryCallback, &obj));
                                         configComponentInstantiated++;
                                         return obj;
                                     }

                                     if (typeId == "Signal")
                                     {
                                         BaseObjectPtr obj;
                                         checkErrorInfo(ConfigClientSignalImpl::Deserialize(serObj, context, factoryCallback, &obj));
                                         configComponentInstantiated++;
                                         return obj;
                                     }

                                     if (typeId == "Channel")
                                     {
                                         BaseObjectPtr obj;
                                         checkErrorInfo(ConfigClientChannelImpl::Deserialize(serObj, context, factoryCallback, &obj));
                                         configComponentInstantiated++;
                                         return obj;
                                     }

                                     return nullptr;
                                 });

    ASSERT_EQ(configComponentInstantiated, 5);
    ASSERT_EQ(newCh.getName(), ch.getName());
    ASSERT_EQ(newCh.getDescription(), ch.getDescription());
    ASSERT_EQ(newCh.getTags(), ch.getTags());

    const auto serializer2 = JsonSerializer(True);
    newCh.serialize(serializer2);
    const auto str2 = serializer2.getOutput();

    ASSERT_EQ(str1, str2);
}

class MockDevice final : public Device
{
public:
    MockDevice(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
        : Device(ctx, parent, localId)
    {
        createAndAddSignal("sig_device");

        auto aiIoFolder = this->addIoFolder("AI", ioFolder);
        createAndAddChannel<MockChannel>(aiIoFolder, "Ch");

        const auto fb = createWithImplementation<IFunctionBlock, MockFbImpl>(ctx, this->functionBlocks, "fb", nullptr, true);
        addNestedFunctionBlock(fb);
    }
};

TEST_F(ConfigProtocolSerializationTest, Device)
{
    const auto dev = createWithImplementation<IDevice, MockDevice>(NullContext(), nullptr, "dev");

    const auto serializer = JsonSerializer(True);
    dev.serialize(serializer);
    const auto str1 = serializer.getOutput();

    const auto deserializer = JsonDeserializer();

    auto clientComm = std::make_shared<ConfigProtocolClientComm>(NullContext(), nullptr, nullptr);

    const auto deserializeContext = createWithImplementation<IConfigProtocolDeserializeContext, ConfigProtocolDeserializeContextImpl>(
        clientComm, std::string {}, NullContext(), nullptr, nullptr, "dev", nullptr);

    int configComponentInstantiated = 0;
    const DevicePtr newFunctionBlock =
        deserializer.deserialize(str1,
                                 deserializeContext,
                                 [&configComponentInstantiated](const StringPtr& typeId,
                                                                const SerializedObjectPtr& serObj,
                                                                const BaseObjectPtr& context,
                                                                const FunctionPtr& factoryCallback) -> BaseObjectPtr
                                 {
                                     if (typeId == "IoFolder")
                                     {
                                         BaseObjectPtr obj;
                                         checkErrorInfo(ConfigClientIoFolderImpl::Deserialize(serObj, context, factoryCallback, &obj));
                                         configComponentInstantiated++;
                                         return obj;
                                     }

                                     if (typeId == "Folder")
                                     {
                                         BaseObjectPtr obj;
                                         checkErrorInfo(ConfigClientFolderImpl::Deserialize(serObj, context, factoryCallback, &obj));
                                         configComponentInstantiated++;
                                         return obj;
                                     }


                                     if (typeId == "Signal")
                                     {
                                         BaseObjectPtr obj;
                                         checkErrorInfo(ConfigClientSignalImpl::Deserialize(serObj, context, factoryCallback, &obj));
                                         configComponentInstantiated++;
                                         return obj;
                                     }

                                     if (typeId == "Device")
                                     {
                                         BaseObjectPtr obj;
                                         checkErrorInfo(ConfigClientDeviceImpl::Deserialize(serObj, context, factoryCallback, &obj));
                                         configComponentInstantiated++;
                                         return obj;
                                     }

                                     return nullptr;
                                 });

    const DevicePtr newDev = deserializer.deserialize(str1, deserializeContext, nullptr);

    ASSERT_EQ(configComponentInstantiated, 15);
    const auto serializer2 = JsonSerializer(True);
    newDev.serialize(serializer2);
    const auto str2 = serializer2.getOutput();

    ASSERT_EQ(str1, str2);
}
