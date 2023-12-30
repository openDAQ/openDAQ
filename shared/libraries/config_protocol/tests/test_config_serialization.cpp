#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <config_protocol/config_protocol_client.h>
#include <config_protocol/config_client_component_impl.h>
#include <opendaq/context_factory.h>
#include <opendaq/component_factory.h>
#include <opendaq/folder_factory.h>
#include <config_protocol/config_protocol_deserialize_context_impl.h>
#include <config_protocol/config_client_component_impl.h>

#include "config_protocol/config_client_folder_impl.h"

using namespace daq;
using namespace config_protocol;

using ConfigProtocolSerializationTest = testing::Test;

TEST_F(ConfigProtocolSerializationTest, Component)
{
    const auto name = "foo";
    const auto desc = "bar";
    const auto component = Component(nullptr, nullptr, "temp");

    component.setName(name);
    component.setDescription(desc);
    component.getTags().add("tag");

    const auto serializer = JsonSerializer(True);
    component.serialize(serializer);
    const auto str1 = serializer.getOutput();

    const auto deserializer = JsonDeserializer();

    auto clientComm = std::make_shared<ConfigProtocolClientComm>(nullptr);

    const auto deserializeContext = createWithImplementation<IConfigProtocolDeserializeContext, ConfigProtocolDeserializeContextImpl>(clientComm, nullptr, nullptr, "temp", TypeManager());
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

TEST_F(ConfigProtocolSerializationTest, FolderWithComponent)
{
    const auto folder = Folder(nullptr, nullptr, "folder");
    folder.setName("folder_name");
    folder.setDescription("folder_desc");
    folder.getTags().add("folder_tag");

    const auto component = Component(nullptr, folder, "component");
    component.setName("comp_name");
    component.setDescription("comp_desc");
    component.getTags().add("comp_tag");

    folder.addItem(component);

    const auto serializer = JsonSerializer(True);
    folder.serialize(serializer);
    const auto str1 = serializer.getOutput();

    const auto deserializer = JsonDeserializer();

    auto clientComm = std::make_shared<ConfigProtocolClientComm>(nullptr);

    const auto deserializeContext = createWithImplementation<IConfigProtocolDeserializeContext, ConfigProtocolDeserializeContextImpl>(
        clientComm, nullptr, nullptr, "folder", TypeManager());
    bool configComponentInstantiated = false;
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
                                         return obj;
                                     }

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
    ASSERT_EQ(newFolder.getName(), folder.getName());
    ASSERT_EQ(newFolder.getDescription(), folder.getDescription());
    ASSERT_EQ(newFolder.getTags(), folder.getTags());

    const auto serializer2 = JsonSerializer(True);
    newFolder.serialize(serializer2);
    const auto str2 = serializer2.getOutput();

    ASSERT_EQ(str1, str2);
}
