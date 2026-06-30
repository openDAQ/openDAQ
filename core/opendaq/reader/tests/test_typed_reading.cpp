#include <gtest/gtest.h>

#include <coreobjects/unit_factory.h>
#include <coretypes/exceptions.h>
#include <coretypes/ratio_factory.h>


#include <opendaq/data_descriptor_factory.h>
#include <opendaq/data_rule_factory.h>
#include <opendaq/domain_value.h>
#include <opendaq/packet_factory.h>
#include <opendaq/reader_utils.h>
#include <opendaq/typed_reading_utils.h>

#include <chrono>

using TypedReadingTest = testing::Test;

TEST_F(TypedReadingTest, LinearRuleDomainReading)
{
    daq::RatioPtr resolution = daq::Ratio(1, 1'000'000'000);
    daq::DataDescriptorPtr domainDescriptor = daq::DataDescriptorBuilder()
                                                  .setSampleType(daq::SampleType::UInt64)
                                                  .setRule(daq::LinearDataRule(1'000'000, 0))
                                                  .setTickResolution(resolution)
                                                  .setUnit(daq::Unit("s", -1, "seconds", "time"))
                                                  .setOrigin("1970-01-01T00:00:00")
                                                  .setName("Time")
                                                  .build();

    daq::DataPacketPtr domainPacket = daq::DataPacket(domainDescriptor, 100, 115);

    daq::ReadLayout readLayout = daq::TypedReadingUtils::createReadLayout(domainDescriptor);
    daq::DomainInfo domainInfo = daq::DomainInfo::fromDescriptor(domainDescriptor);
    std::unique_ptr<daq::DomainValue> domainStart =
        daq::TypedReadingUtils::readDomainValue(daq::SampleType::UInt64, daq::SampleType::UInt64, readLayout, domainPacket, 12, domainInfo);

    auto* domainStartP = dynamic_cast<daq::DomainValueImpl<daq::UInt>*>(domainStart.get());
    ASSERT_FALSE(domainStartP == nullptr);

    ASSERT_EQ(domainStartP->getValue(), 12'000'115u);
}

TEST_F(TypedReadingTest, ExplicitRuleDomainReading)
{
    daq::RatioPtr resolution = daq::Ratio(1, 1'000'000'000);
    daq::DataDescriptorPtr domainDescriptor = daq::DataDescriptorBuilder()
                                                  .setSampleType(daq::SampleType::UInt64)
                                                  .setTickResolution(resolution)
                                                  .setUnit(daq::Unit("s", -1, "seconds", "time"))
                                                  .setOrigin("1970-01-01T00:00:00")
                                                  .setName("Time")
                                                  .build();

    constexpr daq::SizeT packetSize = 100;
    daq::DataPacketPtr domainPacket = daq::DataPacket(domainDescriptor, packetSize, 0);
    daq::UInt* data = static_cast<daq::UInt*>(domainPacket.getRawData());
    for (daq::SizeT i = 0; i < packetSize; ++i)
    {
        data[i] = 112 + 1'000'000 * i;
    }

    daq::ReadLayout readLayout = daq::TypedReadingUtils::createReadLayout(domainDescriptor);
    daq::DomainInfo domainInfo = daq::DomainInfo::fromDescriptor(domainDescriptor);
    std::unique_ptr<daq::DomainValue> domainStart =
        daq::TypedReadingUtils::readDomainValue(daq::SampleType::UInt64, daq::SampleType::UInt64, readLayout, domainPacket, 12, domainInfo);

    auto* domainStartP = dynamic_cast<daq::DomainValueImpl<daq::UInt>*>(domainStart.get());
    ASSERT_FALSE(domainStartP == nullptr);

    ASSERT_EQ(domainStartP->getValue(), 12'000'112u);
}

TEST_F(TypedReadingTest, LinearRuleFindDomain)
{
    daq::RatioPtr resolution = daq::Ratio(1, 1'000'000'000);
    daq::DataDescriptorPtr domainDescriptor = daq::DataDescriptorBuilder()
                                                  .setSampleType(daq::SampleType::UInt64)
                                                  .setRule(daq::LinearDataRule(1'000'000, 0))
                                                  .setTickResolution(resolution)
                                                  .setUnit(daq::Unit("s", -1, "seconds", "time"))
                                                  .setOrigin("1970-01-01T00:00:00")
                                                  .setName("Time")
                                                  .build();

    daq::DataPacketPtr domainPacket = daq::DataPacket(domainDescriptor, 100, 1'000'000'115);

    daq::ReadLayout readLayout = daq::TypedReadingUtils::createReadLayout(domainDescriptor);
    daq::DomainInfo domainInfo = daq::DomainInfo::fromDescriptor(domainDescriptor);
    std::unique_ptr<daq::DomainValue> domainTarget = std::make_unique<daq::DomainValueImpl<daq::UInt>>(domainInfo, 1'009'000'115);
    daq::SizeT index = daq::TypedReadingUtils::findDomainValue(
        daq::SampleType::UInt64, daq::SampleType::UInt64, readLayout, domainPacket, domainTarget.get());

    ASSERT_EQ(index, 9u);
}

TEST_F(TypedReadingTest, ExplicitRuleFindDomain)
{
    daq::RatioPtr resolution = daq::Ratio(1, 1'000'000'000);
    daq::DataDescriptorPtr domainDescriptor = daq::DataDescriptorBuilder()
                                                  .setSampleType(daq::SampleType::UInt64)
                                                  .setTickResolution(resolution)
                                                  .setUnit(daq::Unit("s", -1, "seconds", "time"))
                                                  .setOrigin("1970-01-01T00:00:00")
                                                  .setName("Time")
                                                  .build();

    constexpr daq::SizeT packetSize = 100;
    daq::DataPacketPtr domainPacket = daq::DataPacket(domainDescriptor, packetSize, 0);
    daq::UInt* data = static_cast<daq::UInt*>(domainPacket.getRawData());
    for (daq::SizeT i = 0; i < packetSize; ++i)
    {
        data[i] = 1'000'000'112 + 1'000'000 * i;
    }

    daq::ReadLayout readLayout = daq::TypedReadingUtils::createReadLayout(domainDescriptor);
    daq::DomainInfo domainInfo = daq::DomainInfo::fromDescriptor(domainDescriptor);
    std::unique_ptr<daq::DomainValue> domainTarget = std::make_unique<daq::DomainValueImpl<daq::UInt>>(domainInfo, 1'012'000'112);
    daq::SizeT index = daq::TypedReadingUtils::findDomainValue(
        daq::SampleType::UInt64, daq::SampleType::UInt64, readLayout, domainPacket, domainTarget.get());

    ASSERT_EQ(index, 12u);
}

TEST_F(TypedReadingTest, ExplicitRuleReadData)
{
    daq::RatioPtr resolution = daq::Ratio(1, 1'000'000'000);
    daq::DataDescriptorPtr domainDescriptor = daq::DataDescriptorBuilder()
                                                  .setSampleType(daq::SampleType::UInt64)
                                                  .setTickResolution(resolution)
                                                  .setUnit(daq::Unit("s", -1, "seconds", "time"))
                                                  .setOrigin("1970-01-01T00:00:00")
                                                  .setName("Time")
                                                  .build();

    constexpr daq::SizeT packetSize = 15;
    daq::DataPacketPtr domainPacket = daq::DataPacket(domainDescriptor, packetSize, 0);
    daq::UInt* data = static_cast<daq::UInt*>(domainPacket.getRawData());
    for (daq::SizeT i = 0; i < packetSize; ++i)
    {
        data[i] = 1'000'000'119 + 1'000'000 * i;
    }

    daq::ReadLayout readLayout = daq::TypedReadingUtils::createReadLayout(domainDescriptor);
    daq::DomainInfo domainInfo = daq::DomainInfo::fromDescriptor(domainDescriptor);

    std::vector<daq::UInt> buffer(packetSize);
    for (int i = 0; i < packetSize; ++i)
    {
        void* bufferP = buffer.data();
        daq::SizeT count = packetSize - i;
        daq::ErrCode err = daq::TypedReadingUtils::readData(
            daq::SampleType::UInt64, daq::SampleType::UInt64, true, readLayout, domainPacket.getRawData(), i, &bufferP, count);

        for (daq::SizeT j = 0; j < count; ++j)
        {
            ASSERT_EQ(buffer[j], data[i + j]);
        }
    }
}