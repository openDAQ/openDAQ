#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <config_protocol/config_protocol_client.h>
#include <config_protocol/config_client_component_impl.h>
#include <opendaq/context_factory.h>
#include <opendaq/component_factory.h>
#include <config_protocol/config_protocol_deserialize_context_impl.h>
#include <config_protocol/config_client_component_impl.h>

using namespace daq;
using namespace config_protocol;

using ConfigProtocolSerializationTest = testing::Test;

TEST_F(ConfigProtocolSerializationTest, Test)
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
            if (typeId == "daq_Component")
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
