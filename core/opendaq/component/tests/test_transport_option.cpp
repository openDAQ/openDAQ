#include <opendaq/transport_option_factory.h>
#include <opendaq/transport_option_impl.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>
#include <coretypes/json_serializer_factory.h>
#include <coretypes/json_deserializer_factory.h>
#include <gtest/gtest.h>
#include <algorithm>

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

TEST_F(TransportOptionTest, FieldsAreProperties)
{
    const auto option = TransportOption("tls", 7422, TransportProtection::Signed, False);

    ASSERT_EQ(option.getPropertyValue("Id"), "tls");
    ASSERT_EQ(option.getPropertyValue("Port"), 7422);
    ASSERT_EQ(option.getPropertyValue("Protection"), static_cast<Int>(TransportProtection::Signed));
    ASSERT_EQ(option.getPropertySelectionValue("Protection"), "Signed");
    ASSERT_EQ(option.getPropertyValue("CertificateRequired"), false);
}

TEST_F(TransportOptionTest, Frozen)
{
    const auto option = TransportOption("plain", 7420);

    ASSERT_TRUE(option.isFrozen());
    ASSERT_THROW(option.setPropertyValue("Port", 7421), FrozenException);
    ASSERT_THROW(option.addProperty(IntProperty("Extra", 1)), FrozenException);
    ASSERT_EQ(option.getPort(), 7420);
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

TEST_F(TransportOptionTest, PropertyObjectInterfaceFirst)
{
    const auto option = TransportOption("plain", 7420);

    const auto ids = option.asPtr<IInspectable>(true).getInterfaceIds();
    ASSERT_EQ(ids[0], IPropertyObject::Id);
    ASSERT_NE(std::find(ids.begin(), ids.end(), ITransportOption::Id), ids.end());
}

TEST_F(TransportOptionTest, SerializedAsPlainPropertyObject)
{
    const auto option = TransportOption("tls", 7422, TransportProtection::Encrypted, True);

    const auto serializer = JsonSerializer();
    option.serialize(serializer);
    const std::string json = serializer.getOutput();

    ASSERT_NE(json.find(R"("__type":"PropertyObject")"), std::string::npos);
    ASSERT_EQ(json.find("TransportOption"), std::string::npos);

    const PropertyObjectPtr deserialized = JsonDeserializer().deserialize(String(json), nullptr, nullptr);

    ASSERT_FALSE(deserialized.supportsInterface<ITransportOption>());
    ASSERT_TRUE(deserialized.isFrozen());
    ASSERT_EQ(deserialized.getPropertyValue("Id"), "tls");
    ASSERT_EQ(deserialized.getPropertyValue("Port"), 7422);
    ASSERT_EQ(deserialized.getPropertyValue("Protection"), static_cast<Int>(TransportProtection::Encrypted));
    ASSERT_EQ(deserialized.getPropertyValue("CertificateRequired"), true);
}

TEST_F(TransportOptionTest, RestoredFromDeserializedObject)
{
    const auto option = TransportOption("tls", 7422, TransportProtection::Encrypted, True);

    const auto serializer = JsonSerializer();
    option.serialize(serializer);
    const PropertyObjectPtr deserialized = JsonDeserializer().deserialize(serializer.getOutput(), nullptr, nullptr);

    const auto restored = TransportOptionFromPropertyObject(deserialized);

    ASSERT_EQ(restored.getId(), "tls");
    ASSERT_EQ(restored.getPort(), 7422);
    ASSERT_EQ(restored.getProtection(), TransportProtection::Encrypted);
    ASSERT_TRUE(restored.getCertificateRequired());
    ASSERT_TRUE(restored.isFrozen());
}

TEST_F(TransportOptionTest, RestoringAnOptionReturnsItself)
{
    const auto option = TransportOption("plain", 7420);
    const auto restored = TransportOptionFromPropertyObject(option);

    ASSERT_EQ(restored.getObject(), option.getObject());
}

TEST_F(TransportOptionTest, RestoringOtherObjectRejected)
{
    ASSERT_THROW(TransportOptionFromPropertyObject(nullptr), ArgumentNullException);

    const auto incomplete = PropertyObject();
    incomplete.addProperty(StringProperty("Id", "plain"));
    incomplete.addProperty(IntProperty("Port", 7420));
    ASSERT_THROW(TransportOptionFromPropertyObject(incomplete), InvalidParameterException);

    const auto wrongType = PropertyObject();
    wrongType.addProperty(StringProperty("Id", "plain"));
    wrongType.addProperty(StringProperty("Port", "7420"));
    wrongType.addProperty(IntProperty("Protection", 0));
    wrongType.addProperty(BoolProperty("CertificateRequired", false));
    ASSERT_THROW(TransportOptionFromPropertyObject(wrongType), InvalidParameterException);
}

TEST_F(TransportOptionTest, Clone)
{
    const auto option = TransportOption("tls", 7422, TransportProtection::Encrypted, True);

    PropertyObjectPtr cloned;
    ASSERT_EQ(option.asPtr<IPropertyObjectInternal>()->clone(&cloned), OPENDAQ_SUCCESS);

    const auto clonedOption = cloned.asPtr<ITransportOption, TransportOptionPtr>();
    ASSERT_NE(clonedOption.getObject(), option.getObject());
    ASSERT_EQ(clonedOption.getId(), "tls");
    ASSERT_EQ(clonedOption.getPort(), 7422);
    ASSERT_EQ(clonedOption.getProtection(), TransportProtection::Encrypted);
    ASSERT_TRUE(clonedOption.getCertificateRequired());
    ASSERT_TRUE(clonedOption.isFrozen());
}
