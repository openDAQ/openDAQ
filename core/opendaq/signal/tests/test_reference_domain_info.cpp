#include <gtest/gtest.h>
#include <opendaq/reference_domain_info_factory.h>
#include "testutils/testutils.h"

using ReferenceDomainInfoTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

TEST_F(ReferenceDomainInfoTest, ValueDescriptorSetGet)
{
    auto info = ReferenceDomainInfoBuilder()
                    .setReferenceDomainId("testReferenceDomainId")
                    .setReferenceDomainOffset(53)
                    .setReferenceTimeSource(TimeSource::Gps)
                    .setUsesOffset(UsesOffset::True)
                    .build();
    ASSERT_EQ(info.getReferenceDomainId(), "testReferenceDomainId");
    ASSERT_EQ(info.getReferenceDomainOffset(), 53);
    ASSERT_EQ(info.getReferenceTimeSource(), TimeSource::Gps);
    ASSERT_EQ(info.getUsesOffset(), UsesOffset::True);
}

TEST_F(ReferenceDomainInfoTest, ValueDescriptorCopyFactory)
{
    auto info = ReferenceDomainInfoBuilder()
                    .setReferenceDomainId("testReferenceDomainId")
                    .setReferenceDomainOffset(53)
                    .setReferenceTimeSource(TimeSource::Gps)
                    .setUsesOffset(UsesOffset::True)
                    .build();

    auto copy = ReferenceDomainInfoBuilderCopy(info).build();

    ASSERT_EQ(copy.getReferenceDomainId(), "testReferenceDomainId");
    ASSERT_EQ(copy.getReferenceDomainOffset(), 53);
    ASSERT_EQ(copy.getReferenceTimeSource(), TimeSource::Gps);
    ASSERT_EQ(copy.getUsesOffset(), UsesOffset::True);
}

TEST_F(ReferenceDomainInfoTest, SerializeDeserialize)
{
    auto info = ReferenceDomainInfoBuilder()
                    .setReferenceDomainId("testReferenceDomainId")
                    .setReferenceDomainOffset(53)
                    .setReferenceTimeSource(TimeSource::Gps)
                    .setUsesOffset(UsesOffset::True)
                    .build();

    auto serializer = JsonSerializer(False);
    info.serialize(serializer);

    auto serialized = serializer.getOutput();

    auto deserializer = JsonDeserializer();
    auto info1 = deserializer.deserialize(serialized.toStdString()).asPtr<IReferenceDomainInfo>();

    ASSERT_EQ(info, info1);
}

TEST_F(ReferenceDomainInfoTest, StructType)
{
    const auto structType = ReferenceDomainInfoStructType();
    const daq::StructPtr structPtr = ReferenceDomainInfoBuilder().build();
    ASSERT_EQ(structType, structPtr.getStructType());
}

TEST_F(ReferenceDomainInfoTest, StructFields)
{
    const StructPtr info = ReferenceDomainInfoBuilder()
                               .setReferenceDomainId("testReferenceDomainId")
                               .setReferenceDomainOffset(53)
                               .setReferenceTimeSource(TimeSource::Gps)
                               .setUsesOffset(UsesOffset::True)
                               .build();

    ASSERT_EQ(info.get("ReferenceDomainId"), "testReferenceDomainId");
    ASSERT_EQ(info.get("ReferenceDomainOffset"), 53);
    ASSERT_EQ(info.get("ReferenceTimeSource"), TimeSource::Gps);
    ASSERT_EQ(info.get("UsesOffset"), UsesOffset::True);
}

TEST_F(ReferenceDomainInfoTest, StructNames)
{
    const auto structType = ReferenceDomainInfoStructType();
    const daq::StructPtr structPtr = ReferenceDomainInfoBuilder().build();
    ASSERT_EQ(structType.getFieldNames(), structPtr.getFieldNames());
}

TEST_F(ReferenceDomainInfoTest, ReferenceDomainInfoBuilderSetGet)
{
    const auto infoBuilder = ReferenceDomainInfoBuilder()
                                 .setReferenceDomainId("testReferenceDomainId")
                                 .setReferenceDomainOffset(53)
                                 .setReferenceTimeSource(TimeSource::Gps)
                                 .setUsesOffset(UsesOffset::True);

    ASSERT_EQ(infoBuilder.getReferenceDomainId(), "testReferenceDomainId");
    ASSERT_EQ(infoBuilder.getReferenceDomainOffset(), 53);
    ASSERT_EQ(infoBuilder.getReferenceTimeSource(), TimeSource::Gps);
    ASSERT_EQ(infoBuilder.getUsesOffset(), UsesOffset::True);
}

TEST_F(ReferenceDomainInfoTest, ReferenceDomainInfoCreateFactory)
{
    const auto infoBuilder = ReferenceDomainInfoBuilder()
                                 .setReferenceDomainId("testReferenceDomainId")
                                 .setReferenceDomainOffset(53)
                                 .setReferenceTimeSource(TimeSource::Gps)
                                 .setUsesOffset(UsesOffset::True);

    const auto info = ReferenceDomainInfoFromBuilder(infoBuilder);
    ASSERT_EQ(info.getReferenceDomainId(), "testReferenceDomainId");
    ASSERT_EQ(info.getReferenceDomainOffset(), 53);
    ASSERT_EQ(info.getReferenceTimeSource(), TimeSource::Gps);
    ASSERT_EQ(info.getUsesOffset(), UsesOffset::True);
}

TEST_F(ReferenceDomainInfoTest, QueryInterface)
{
    auto info = ReferenceDomainInfoBuilder()
                    .setReferenceDomainId("A")
                    .setReferenceTimeSource(TimeSource::Gps)
                    .setReferenceDomainOffset(5)
                    .setUsesOffset(UsesOffset::True)
                    .build();

    auto info1 = info.asPtr<IReferenceDomainInfo>();
    ASSERT_EQ(info1.getReferenceDomainId(), "A");
    ASSERT_EQ(info1.getReferenceTimeSource(), TimeSource::Gps);
    ASSERT_EQ(info1.getReferenceDomainOffset(), 5);
    ASSERT_EQ(info1.getUsesOffset(), UsesOffset::True);

    auto info2 = info.asPtr<IReferenceDomainInfo>(true);
    ASSERT_EQ(info2.getReferenceDomainId(), "A");
    ASSERT_EQ(info2.getReferenceTimeSource(), TimeSource::Gps);
    ASSERT_EQ(info2.getReferenceDomainOffset(), 5);
    ASSERT_EQ(info2.getUsesOffset(), UsesOffset::True);
}

END_NAMESPACE_OPENDAQ
