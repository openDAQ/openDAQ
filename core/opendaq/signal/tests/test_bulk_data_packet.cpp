#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include <opendaq/data_rule_factory.h>
#include <opendaq/packet_factory.h>
#include <opendaq/sample_type_traits.h>
#include <opendaq/deleter_factory.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/bulk_data_packet_factory.h>

using namespace daq;

class BulkDataPacketTest: public ::testing::TestWithParam<size_t>
{
protected:
    static size_t alignTo(size_t value, size_t align)
    {
        return (value + align - 1) / align * align;
    }
};

TEST_P(BulkDataPacketTest, Implicit)
{
    const size_t align = GetParam();
    constexpr SampleType sampleType[] = {SampleType::Float32, SampleType::Float64, SampleType::Int32};

    const auto domainDescriptor = DataDescriptorBuilder().setSampleType(SampleType::Int64).setRule(LinearDataRule(1, 0)).setUnit(Unit("s", -1, "seconds", "time")).setTickResolution(Ratio(1, 1000)).build();
    const auto valueDescriptor0 = DataDescriptorBuilder().setSampleType(sampleType[0]).build();
    const auto valueDescriptor1 = DataDescriptorBuilder().setSampleType(sampleType[1]).build();
    const auto valueDescriptor2 = DataDescriptorBuilder().setSampleType(sampleType[2]).build();

    IDataDescriptor* valueDescriptors[] = {valueDescriptor0, valueDescriptor1, valueDescriptor2};
    IDataDescriptor* domainDescriptors[] = {domainDescriptor, domainDescriptor, domainDescriptor};
    size_t sampleCount[] = {1, 2, 4};
    Int offsets[] = {0, 0, 200};

    IDataPacket* dataPackets[3];

    daqBulkCreateDataPackets(dataPackets, valueDescriptors, domainDescriptors, sampleCount, offsets, 3, align);

    ASSERT_EQ(DataPacketPtr::Borrow(dataPackets[0]).getDataDescriptor(), DataDescriptorPtr::Borrow(valueDescriptors[0]));
    ASSERT_EQ(DataPacketPtr::Borrow(dataPackets[0]).getDomainPacket().getDataDescriptor(), DataDescriptorPtr::Borrow(domainDescriptor));
    ASSERT_EQ(DataPacketPtr::Borrow(dataPackets[1]).getDataDescriptor(), DataDescriptorPtr::Borrow(valueDescriptors[1]));
    ASSERT_EQ(DataPacketPtr::Borrow(dataPackets[1]).getDomainPacket().getDataDescriptor(), DataDescriptorPtr::Borrow(domainDescriptor));
    ASSERT_EQ(DataPacketPtr::Borrow(dataPackets[2]).getDataDescriptor(), DataDescriptorPtr::Borrow(valueDescriptors[2]));
    ASSERT_EQ(DataPacketPtr::Borrow(dataPackets[2]).getDomainPacket().getDataDescriptor(), DataDescriptorPtr::Borrow(domainDescriptor));

    const auto rawData0 = static_cast<float*>(DataPacketPtr::Borrow(dataPackets[0]).getRawData());
    const auto rawData1 = static_cast<double*>(DataPacketPtr::Borrow(dataPackets[1]).getRawData());
    const auto rawData2 = static_cast<int32_t*>(DataPacketPtr::Borrow(dataPackets[2]).getRawData());

    ASSERT_EQ(reinterpret_cast<uint8_t*>(rawData0) + alignTo(sampleCount[0] * getSampleSize(sampleType[0]), align),
              reinterpret_cast<uint8_t*>(rawData1));
    ASSERT_EQ(reinterpret_cast<uint8_t*>(rawData1) + alignTo(sampleCount[1] * getSampleSize(sampleType[1]), align),
              reinterpret_cast<uint8_t*>(rawData2));

    for (size_t i = 0; i < sampleCount[0]; i++)
        rawData0[i] = 1.0f;
    for (size_t i = 0; i < sampleCount[1]; i++)
        rawData1[i] = 2.0;
    for (size_t i = 0; i < sampleCount[2]; i++)
        rawData2[i] = 4;

    for (size_t i = 0; i < sampleCount[0]; i++)
        ASSERT_EQ(rawData0[i], 1.0f);
    for (size_t i = 0; i < sampleCount[1]; i++)
        ASSERT_EQ(rawData1[i],  2.0);
    for (size_t i = 0; i < sampleCount[2]; i++)
        ASSERT_EQ(rawData2[i], 4);

    for (size_t i = 0; i < 3; i++)
    {
        ASSERT_EQ(DataPacketPtr::Borrow(dataPackets[i]).getDomainPacket().getOffset(), offsets[i]);
        dataPackets[i]->releaseRef();
    }
}

TEST_P(BulkDataPacketTest, Explicit)
{
    const size_t align = GetParam();
    constexpr SampleType sampleType[] = {SampleType::Float32, SampleType::Float64, SampleType::Int32};

    const auto domainDescriptor = DataDescriptorBuilder()
                                      .setSampleType(SampleType::Int64)
                                      .setUnit(Unit("s", -1, "seconds", "time"))
                                      .setTickResolution(Ratio(1, 1000))
                                      .build();
    const auto valueDescriptor0 = DataDescriptorBuilder().setSampleType(sampleType[0]).build();
    const auto valueDescriptor1 = DataDescriptorBuilder().setSampleType(sampleType[1]).build();
    const auto valueDescriptor2 = DataDescriptorBuilder().setSampleType(sampleType[2]).build();

    IDataDescriptor* valueDescriptors[] = {valueDescriptor0, valueDescriptor1, valueDescriptor2};
    IDataDescriptor* domainDescriptors[] = {domainDescriptor, domainDescriptor, domainDescriptor};
    size_t sampleCount[] = {1, 2, 4};

    IDataPacket* dataPackets[3];

    daqBulkCreateDataPackets(dataPackets, valueDescriptors, domainDescriptors, sampleCount, nullptr, 3, align);

    ASSERT_EQ(DataPacketPtr::Borrow(dataPackets[0]).getDataDescriptor(), DataDescriptorPtr::Borrow(valueDescriptors[0]));
    ASSERT_EQ(DataPacketPtr::Borrow(dataPackets[0]).getDomainPacket().getDataDescriptor(), DataDescriptorPtr::Borrow(domainDescriptor));
    ASSERT_EQ(DataPacketPtr::Borrow(dataPackets[1]).getDataDescriptor(), DataDescriptorPtr::Borrow(valueDescriptors[1]));
    ASSERT_EQ(DataPacketPtr::Borrow(dataPackets[1]).getDomainPacket().getDataDescriptor(), DataDescriptorPtr::Borrow(domainDescriptor));
    ASSERT_EQ(DataPacketPtr::Borrow(dataPackets[2]).getDataDescriptor(), DataDescriptorPtr::Borrow(valueDescriptors[2]));
    ASSERT_EQ(DataPacketPtr::Borrow(dataPackets[2]).getDomainPacket().getDataDescriptor(), DataDescriptorPtr::Borrow(domainDescriptor));

    const auto rawData0 = static_cast<float*>(DataPacketPtr::Borrow(dataPackets[0]).getRawData());
    const auto rawData1 = static_cast<double*>(DataPacketPtr::Borrow(dataPackets[1]).getRawData());
    const auto rawData2 = static_cast<int32_t*>(DataPacketPtr::Borrow(dataPackets[2]).getRawData());

    const auto rawDomainData0 = static_cast<int64_t*>(DataPacketPtr::Borrow(dataPackets[0]).getDomainPacket().getRawData());
    const auto rawDomainData1 = static_cast<int64_t*>(DataPacketPtr::Borrow(dataPackets[1]).getDomainPacket().getRawData());
    const auto rawDomainData2 = static_cast<int64_t*>(DataPacketPtr::Borrow(dataPackets[2]).getDomainPacket().getRawData());

    ASSERT_EQ(reinterpret_cast<uint8_t*>(rawData0) + alignTo(sampleCount[0] * getSampleSize(sampleType[0]), align) + alignTo(sampleCount[1] * sizeof(int64_t), align), reinterpret_cast<uint8_t*>(rawData1));
    ASSERT_EQ(reinterpret_cast<uint8_t*>(rawData1) + alignTo(sampleCount[1] * getSampleSize(sampleType[1]), align) + alignTo(sampleCount[2] * sizeof(int64_t), align), reinterpret_cast<uint8_t*>(rawData2));

    ASSERT_EQ(reinterpret_cast<uint8_t*>(rawDomainData0) + alignTo(sampleCount[0] * getSampleSize(sampleType[0]), align) + alignTo(sampleCount[0] * sizeof(int64_t), align), reinterpret_cast<uint8_t*>(rawDomainData1));
    ASSERT_EQ(reinterpret_cast<uint8_t*>(rawDomainData1) + alignTo(sampleCount[1] * getSampleSize(sampleType[1]), align) + alignTo(sampleCount[1] * sizeof(int64_t), align), reinterpret_cast<uint8_t*>(rawDomainData2));

    for (size_t i = 0; i < sampleCount[0]; i++)
    {
        rawData0[i] = 1.0f;
        rawDomainData0[i] = 1;
    }
    for (size_t i = 0; i < sampleCount[1]; i++)
    {
        rawData1[i] = 2.0;
        rawDomainData1[i] = 2.0;
    }
    for (size_t i = 0; i < sampleCount[2]; i++)
    {
        rawData2[i] = 4;
        rawDomainData2[i] = 4;
    }

    for (size_t i = 0; i < sampleCount[0]; i++)
    {
        ASSERT_EQ(rawData0[i], 1.0f);
        ASSERT_EQ(rawDomainData0[i], 1);
    }
    for (size_t i = 0; i < sampleCount[1]; i++)
    {
        ASSERT_EQ(rawData1[i], 2.0);
        ASSERT_EQ(rawDomainData1[i], 2.0);
    }
    for (size_t i = 0; i < sampleCount[2]; i++)
    {
        ASSERT_EQ(rawData2[i], 4);
        ASSERT_EQ(rawDomainData2[i], 4);
    }

    for (size_t i = 0; i < 3; i++)
    {
        ASSERT_FALSE(DataPacketPtr::Borrow(dataPackets[i]).getDomainPacket().getOffset().assigned());
        dataPackets[i]->releaseRef();
    }
}

TEST_P(BulkDataPacketTest, ValuePacketWithImplicitDomain)
{
    const size_t align = GetParam();

    const auto domainDescriptor = DataDescriptorBuilder()
                                      .setSampleType(SampleType::Int64)
                                      .setRule(LinearDataRule(1, 0))
                                      .setUnit(Unit("s", -1, "seconds", "time"))
                                      .setTickResolution(Ratio(1, 1000))
                                      .build();
    const auto valueDescriptor = DataDescriptorBuilder().setSampleType(SampleType::Float32).build();

    const auto valuePacket = daq::DataPacketWithImplicitDomainPacket(valueDescriptor, domainDescriptor, 4, 10, align);
    ASSERT_EQ(valuePacket.getDataDescriptor(), valueDescriptor);
    ASSERT_EQ(valuePacket.getDomainPacket().getDataDescriptor(), domainDescriptor);
    ASSERT_EQ(valuePacket.getDomainPacket().getOffset(), 10);
}

TEST_P(BulkDataPacketTest, ValuePacketWithExplicit)
{
    const size_t align = GetParam();

    const auto domainDescriptor = DataDescriptorBuilder()
                                      .setSampleType(SampleType::Int64)
                                      .setUnit(Unit("s", -1, "seconds", "time"))
                                      .setTickResolution(Ratio(1, 1000))
                                      .build();
    const auto valueDescriptor = DataDescriptorBuilder().setSampleType(SampleType::Float32).build();

    const auto valuePacket = daq::DataPacketWithExplicitDomainPacket(valueDescriptor, domainDescriptor, 4, align);
    ASSERT_EQ(valuePacket.getDataDescriptor(), valueDescriptor);
    ASSERT_EQ(valuePacket.getDomainPacket().getDataDescriptor(), domainDescriptor);
}

INSTANTIATE_TEST_CASE_P(BulkDataPacketTests,
                        BulkDataPacketTest,
                        ::testing::Values(1, 4, 16, 64, 128),
                        [](testing::TestParamInfo<size_t> paramInfo) { return fmt::format("align{}", paramInfo.param); });

