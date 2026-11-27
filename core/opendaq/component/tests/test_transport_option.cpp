#include <opendaq/transport_option_factory.h>
#include <opendaq/transport_option_impl.h>
#include <coreobjects/property_object_ptr.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_object_internal_ptr.h>
#include <coreobjects/property_factory.h>
#include <coretypes/json_serializer_factory.h>
#include <coretypes/json_deserializer_factory.h>
#include <gtest/gtest.h>

using namespace daq;

using TransportOptionTest = testing::Test;

TEST_F(TransportOptionTest, Fields)
{
    const auto option =
        TransportOptionBuilder("tls").setPort(7422).setSecurityMode(SecurityMode::Encrypted).setCertificateRequired(True).build();

    ASSERT_EQ(option.getId(), "tls");
    ASSERT_EQ(option.getPort(), 7422);
    ASSERT_EQ(option.getSecurityMode(), SecurityMode::Encrypted);
    ASSERT_TRUE(option.getCertificateRequired());
}

TEST_F(TransportOptionTest, Defaults)
{
    const auto builder = TransportOptionBuilder("serial");

    ASSERT_EQ(builder.getId(), "serial");
    ASSERT_EQ(builder.getPort(), 0);
    ASSERT_EQ(builder.getSecurityMode(), SecurityMode::None);
    ASSERT_FALSE(builder.getCertificateRequired());

    const auto option = builder.build();
    ASSERT_EQ(option.getPort(), 0);
    ASSERT_EQ(option.getSecurityMode(), SecurityMode::None);
    ASSERT_FALSE(option.getCertificateRequired());
}

TEST_F(TransportOptionTest, BuilderGetters)
{
    const auto builder =
        TransportOptionBuilder("tls").setPort(7422).setSecurityMode(SecurityMode::Signed).setCertificateRequired(True);

    ASSERT_EQ(builder.getId(), "tls");
    ASSERT_EQ(builder.getPort(), 7422);
    ASSERT_EQ(builder.getSecurityMode(), SecurityMode::Signed);
    ASSERT_TRUE(builder.getCertificateRequired());
}

TEST_F(TransportOptionTest, BuilderKeepsBuiltOptionsApart)
{
    const auto builder = TransportOptionBuilder("plain").setPort(7420);
    const auto first = builder.build();

    builder.setPort(7500);
    const auto second = builder.build();

    ASSERT_EQ(first.getPort(), 7420);
    ASSERT_EQ(second.getPort(), 7500);
}

TEST_F(TransportOptionTest, BuilderCopy)
{
    const auto option =
        TransportOptionBuilder("tls").setPort(7422).setSecurityMode(SecurityMode::Encrypted).setCertificateRequired(True).build();

    const auto copy = TransportOptionBuilderCopy(option).setPort(7500).build();

    ASSERT_EQ(copy.getId(), "tls");
    ASSERT_EQ(copy.getPort(), 7500);
    ASSERT_EQ(copy.getSecurityMode(), SecurityMode::Encrypted);
    ASSERT_TRUE(copy.getCertificateRequired());
    ASSERT_EQ(option.getPort(), 7422);
}

TEST_F(TransportOptionTest, IsPropertyObject)
{
    const auto option =
        TransportOptionBuilder("tls").setPort(7422).setSecurityMode(SecurityMode::Encrypted).setCertificateRequired(True).build();

    ASSERT_EQ(option.getPropertyValue("Id"), "tls");
    ASSERT_EQ(option.getPropertyValue("Port"), 7422);
    ASSERT_EQ(option.getPropertySelectionValue("SecurityMode"), "Encrypted");
    ASSERT_EQ(option.getPropertyValue("CertificateRequired"), true);
}

// Like AddressInfo, the option can be the value of an object-type property
TEST_F(TransportOptionTest, ChildOfPropertyObject)
{
    const auto option = TransportOptionBuilder("tls").setPort(7422).build();

    auto parent = PropertyObject();
    parent.addProperty(ObjectProperty("tls", option));

    const TransportOptionPtr child = parent.getPropertyValue("tls").asPtr<ITransportOption>();
    ASSERT_EQ(child.getId(), "tls");
    ASSERT_EQ(child.getPort(), 7422);
}

TEST_F(TransportOptionTest, PropertiesAreReadOnly)
{
    const auto option = TransportOptionBuilder("tls").setPort(7422).build();

    ASSERT_THROW(option.setPropertyValue("Id", "plain"), AccessDeniedException);
    ASSERT_THROW(option.setPropertyValue("Port", 9000), AccessDeniedException);
    ASSERT_THROW(option.setPropertyValue("SecurityMode", static_cast<Int>(SecurityMode::Encrypted)), AccessDeniedException);
    ASSERT_THROW(option.setPropertyValue("CertificateRequired", true), AccessDeniedException);
    ASSERT_EQ(option.getId(), "tls");
    ASSERT_EQ(option.getPort(), 7422);
}

TEST_F(TransportOptionTest, Clone)
{
    const auto option =
        TransportOptionBuilder("tls").setPort(7422).setSecurityMode(SecurityMode::Encrypted).setCertificateRequired(True).build();

    const TransportOptionPtr cloned = option.asPtr<IPropertyObjectInternal>().clone().asPtr<ITransportOption>();

    ASSERT_NE(cloned.getObject(), option.getObject());
    ASSERT_EQ(cloned.getId(), "tls");
    ASSERT_EQ(cloned.getPort(), 7422);
    ASSERT_EQ(cloned.getSecurityMode(), SecurityMode::Encrypted);
    ASSERT_TRUE(cloned.getCertificateRequired());
    ASSERT_THROW(cloned.setPropertyValue("Port", 9000), AccessDeniedException);
}

TEST_F(TransportOptionTest, EmptyIdRejected)
{
    ASSERT_THROW(TransportOptionBuilder(nullptr).setPort(7420).build(), InvalidParameterException);
    ASSERT_THROW(TransportOptionBuilder("").build(), InvalidParameterException);
}

TEST_F(TransportOptionTest, InvalidSecurityModeRejected)
{
    ASSERT_THROW(TransportOptionBuilder("plain").setSecurityMode(static_cast<SecurityMode>(7)).build(),
                 InvalidParameterException);
}

TEST_F(TransportOptionTest, SerializeDeserialize)
{
    const auto option =
        TransportOptionBuilder("tls").setPort(7422).setSecurityMode(SecurityMode::Encrypted).setCertificateRequired(True).build();

    const auto serializer = JsonSerializer();
    option.serialize(serializer);
    const std::string json = serializer.getOutput();

    ASSERT_NE(json.find(R"("__type":"TransportOption")"), std::string::npos);

    const TransportOptionPtr deserialized = JsonDeserializer().deserialize(String(json), nullptr, nullptr);

    ASSERT_NE(deserialized.getObject(), option.getObject());
    ASSERT_EQ(deserialized.getId(), "tls");
    ASSERT_EQ(deserialized.getPort(), 7422);
    ASSERT_EQ(deserialized.getSecurityMode(), SecurityMode::Encrypted);
    ASSERT_TRUE(deserialized.getCertificateRequired());
    ASSERT_THROW(deserialized.setPropertyValue("Port", 9000), AccessDeniedException);
}

TEST_F(TransportOptionTest, DeserializeEachSecurityMode)
{
    for (const auto securityMode : {SecurityMode::None, SecurityMode::Signed, SecurityMode::Encrypted})
    {
        const auto serializer = JsonSerializer();
        TransportOptionBuilder("plain").setPort(7420).setSecurityMode(securityMode).build().serialize(serializer);

        const TransportOptionPtr deserialized = JsonDeserializer().deserialize(serializer.getOutput(), nullptr, nullptr);
        ASSERT_EQ(deserialized.getSecurityMode(), securityMode);
    }
}

TEST_F(TransportOptionTest, DeserializeUnknownSecurityModeRejected)
{
    const auto serializer = JsonSerializer();
    TransportOptionBuilder("plain").setPort(7420).setSecurityMode(SecurityMode::Signed).build().serialize(serializer);

    std::string json = serializer.getOutput();
    const std::string signedValue = R"("SecurityMode":1)";
    ASSERT_NE(json.find(signedValue), std::string::npos);
    json.replace(json.find(signedValue), signedValue.size(), R"("SecurityMode":7)");

    ASSERT_THROW(JsonDeserializer().deserialize(String(json), nullptr, nullptr), NotFoundException);
}
