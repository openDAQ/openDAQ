#include <opendaq/transport_option_factory.h>
#include <opendaq/transport_option_impl.h>
#include <coreobjects/property_object_ptr.h>
#include <coretypes/json_serializer_factory.h>
#include <coretypes/json_deserializer_factory.h>
#include <gtest/gtest.h>

using namespace daq;

using TransportOptionTest = testing::Test;

TEST_F(TransportOptionTest, Fields)
{
    const auto option = TransportOption("tls", 7422, TransportProtection::Encrypted, True);

    ASSERT_EQ(option.getId(), "tls");
    ASSERT_EQ(option.getPort(), 7422);
    ASSERT_EQ(option.getProtection(), TransportProtection::Encrypted);
    ASSERT_TRUE(option.getCertificateRequired());
}

TEST_F(TransportOptionTest, Defaults)
{
    const auto option = TransportOption("plain", 7420);

    ASSERT_EQ(option.getProtection(), TransportProtection::None);
    ASSERT_FALSE(option.getCertificateRequired());
}

TEST_F(TransportOptionTest, PortZeroForCarrierWithoutPorts)
{
    const auto option = TransportOption("serial", 0);
    ASSERT_EQ(option.getPort(), 0);
}

TEST_F(TransportOptionTest, NotAPropertyObject)
{
    const auto option = TransportOption("plain", 7420);
    ASSERT_FALSE(option.supportsInterface<IPropertyObject>());
}

TEST_F(TransportOptionTest, EmptyIdRejected)
{
    ASSERT_THROW(TransportOption("", 7420), InvalidParameterException);
    ASSERT_THROW(TransportOption(nullptr, 7420), InvalidParameterException);
}

TEST_F(TransportOptionTest, NegativePortRejected)
{
    ASSERT_THROW(TransportOption("plain", -1), InvalidParameterException);
}

TEST_F(TransportOptionTest, UnassignedPortRejected)
{
    ITransportOption* option = nullptr;
    ASSERT_EQ(createTransportOption(&option, String("plain"), nullptr, TransportProtection::None, False), OPENDAQ_ERR_ARGUMENT_NULL);
    daqClearErrorInfo();
    ASSERT_EQ(option, nullptr);
}

TEST_F(TransportOptionTest, InvalidProtectionRejected)
{
    ASSERT_THROW(TransportOption("plain", 7420, static_cast<TransportProtection>(7)), InvalidParameterException);
}

TEST_F(TransportOptionTest, SerializeDeserialize)
{
    const auto option = TransportOption("tls", 7422, TransportProtection::Encrypted, True);

    const auto serializer = JsonSerializer();
    option.serialize(serializer);
    const std::string json = serializer.getOutput();

    ASSERT_NE(json.find(R"("__type":"TransportOption")"), std::string::npos);
    ASSERT_NE(json.find(R"("protection":"Encrypted")"), std::string::npos);

    const TransportOptionPtr deserialized = JsonDeserializer().deserialize(String(json), nullptr, nullptr);

    ASSERT_NE(deserialized.getObject(), option.getObject());
    ASSERT_EQ(deserialized.getId(), "tls");
    ASSERT_EQ(deserialized.getPort(), 7422);
    ASSERT_EQ(deserialized.getProtection(), TransportProtection::Encrypted);
    ASSERT_TRUE(deserialized.getCertificateRequired());
}

TEST_F(TransportOptionTest, DeserializeEachProtection)
{
    for (const auto protection : {TransportProtection::None, TransportProtection::Signed, TransportProtection::Encrypted})
    {
        const auto serializer = JsonSerializer();
        TransportOption("plain", 7420, protection).serialize(serializer);

        const TransportOptionPtr deserialized = JsonDeserializer().deserialize(serializer.getOutput(), nullptr, nullptr);
        ASSERT_EQ(deserialized.getProtection(), protection);
    }
}

TEST_F(TransportOptionTest, DeserializeUnknownProtectionRejected)
{
    const auto serializer = JsonSerializer();
    TransportOption("plain", 7420, TransportProtection::Signed).serialize(serializer);

    std::string json = serializer.getOutput();
    const std::string signedValue = R"("protection":"Signed")";
    json.replace(json.find(signedValue), signedValue.size(), R"("protection":"Scrambled")");

    ASSERT_THROW(JsonDeserializer().deserialize(String(json), nullptr, nullptr), InvalidParameterException);
}
