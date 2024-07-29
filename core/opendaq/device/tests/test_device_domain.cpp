#include <gtest/gtest.h>
#include <opendaq/device_domain_factory.h>

using DeviceDomainTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

TEST_F(DeviceDomainTest, DeviceDomainGetters)
{
    auto deviceDomain = DeviceDomain(Ratio(1, 3), "1993", Unit("Symbol", -1, "Name", "Quantity"), "ReferenceDomainId", 666);
    ASSERT_EQ(deviceDomain.getTickResolution(), Ratio(1, 3));
    ASSERT_EQ(deviceDomain.getOrigin(), "1993");
    ASSERT_EQ(deviceDomain.getUnit(), Unit("Symbol", -1, "Name", "Quantity"));
    ASSERT_EQ(deviceDomain.getReferenceDomainId(), "ReferenceDomainId");
    ASSERT_EQ(deviceDomain.getReferenceDomainOffset(), 666);
}

TEST_F(DeviceDomainTest, DeviceDomainGettersBackwardsCompat)
{
    // Without referenceDomainId/referenceDomainOffset

    auto deviceDomain = DeviceDomain(Ratio(1, 3), "1993", Unit("Symbol", -1, "Name", "Quantity"));
    ASSERT_EQ(deviceDomain.getTickResolution(), Ratio(1, 3));
    ASSERT_EQ(deviceDomain.getOrigin(), "1993");
    ASSERT_EQ(deviceDomain.getUnit(), Unit("Symbol", -1, "Name", "Quantity"));
    ASSERT_EQ(deviceDomain.getReferenceDomainId(), nullptr);
    ASSERT_EQ(deviceDomain.getReferenceDomainOffset(), nullptr);
}

TEST_F(DeviceDomainTest, SerializeDeserialize)
{
    auto deviceDomain = DeviceDomain(Ratio(1, 3), "1993", Unit("Symbol", -1, "Name", "Quantity"), "ReferenceDomainId", 666);
    auto serializer = JsonSerializer(False);
    deviceDomain.serialize(serializer);
    auto serialized = serializer.getOutput();
    auto deserializer = JsonDeserializer();
    auto deserializedDeviceDomain = deserializer.deserialize(serialized.toStdString()).asPtr<IDeviceDomain>();
    ASSERT_EQ(deviceDomain, deserializedDeviceDomain);
}

TEST_F(DeviceDomainTest, DeserializeBackwardsCompat)
{
    // Without referenceDomainId/referenceDomainOffset

    std::string serialized =
        R"({"__type":"DeviceDomain","tickResolution":{"__type":"Ratio","num":1,"den":3},"origin":"1993","unit":{"__type":"Unit","symbol":"Symbol","name":"Name","quantity":"Quantity"}})";
    auto deserializer = JsonDeserializer();
    DeviceDomainPtr deviceDomain;
    ASSERT_NO_THROW(deviceDomain = deserializer.deserialize(serialized));
    ASSERT_EQ(deviceDomain.getOrigin(), "1993");
}

TEST_F(DeviceDomainTest, StructType)
{
    const auto correct =
        StructType("DeviceDomain",
                   List<IString>("TickResolution", "Origin", "Unit", "ReferenceDomainId", "ReferenceDomainOffset"),
                   List<IBaseObject>(Ratio(1, 1), "", Unit("s", -1, "second", "time"), nullptr, nullptr),
                   List<IType>(RatioStructType(), SimpleType(ctString), UnitStructType(), SimpleType(ctString), SimpleType(ctInt)));
    const auto structType = DeviceDomainStructType();
    ASSERT_EQ(structType, correct);
}

END_NAMESPACE_OPENDAQ
