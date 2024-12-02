#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/data_rule_factory.h>
#include <opendaq/dimension_factory.h>
#include <opendaq/packet_factory.h>
#include <opendaq/reusable_data_packet_ptr.h>
#include <opendaq/sample_type_traits.h>
#include <opendaq/scaling_factory.h>
#include <opendaq/scaling_ptr.h>

#include "opendaq/deleter_factory.h"

using DataPacketTest = testing::Test;

using namespace daq;

// Helper methods

template <typename T, size_t SIZE>
static DataPacketPtr createExplicitPacket(const DataDescriptorPtr& descriptor)
{
    const DataPacketPtr packet = DataPacket(descriptor, SIZE);

    T* data = static_cast<T*>(packet.getRawData());
    for (size_t i = 0; i < SIZE; ++i)
        data[i] = static_cast<T>(i);

    return packet;
}

static DataDescriptorPtr setupDescriptor(SampleType type, DataRulePtr rule, ScalingPtr scaling)
{
    return DataDescriptorBuilder().setSampleType(type).setRule(rule).setPostScaling(scaling).build();
}

template <typename T>
static void validateImplicitLinearDataRulePacket(const DataDescriptorPtr& descriptor, T delta, T start, const NumberPtr& packetOffset)
{
    const DataPacketPtr packet = DataPacket(descriptor, 100, packetOffset);
    const auto scaledData = static_cast<T*>(packet.getData());
    const T offsetT = static_cast<T>(packetOffset);
    for (uint64_t i = 0; i < packet.getSampleCount(); ++i)
        ASSERT_EQ(scaledData[i], static_cast<T>(offsetT + i * delta + start));
}

template <typename T, typename U>
static void validateLinearScalingPacket(const DataDescriptorPtr& descriptor, U scale, U offset)
{
    const DataPacketPtr packet = createExplicitPacket<T, 100>(descriptor);

    const auto scaledData = static_cast<U*>(packet.getData());
    for (uint64_t i = 0; i < packet.getSampleCount(); ++i)
        ASSERT_EQ(scaledData[i], static_cast<U>(i * scale + offset));
}

template <typename T>
static void validateImplicitConstantDataRulePacket(const DataDescriptorPtr& descriptor, T constant)
{
    const DataPacketPtr packet = ConstantDataPacketWithDomain(nullptr, descriptor, 100, constant);
    const auto scaledData = static_cast<T*>(packet.getData());
    for (uint64_t i = 0; i < packet.getSampleCount(); ++i)
        ASSERT_EQ(scaledData[i], constant);
}

// Tests

TEST_F(DataPacketTest, TestConstructorErrors)
{
    ASSERT_THROW(DataPacket(nullptr, 0, 10), ArgumentNullException);
}

TEST_F(DataPacketTest, DataPacketTestGetters)
{
    const auto desc = setupDescriptor(SampleType::Float64, LinearDataRule(10.5, 200), nullptr);
    const DataPacketPtr packet = DataPacket(desc, 100, 10);

    ASSERT_EQ(packet.getType(), PacketType::Data);

    ASSERT_EQ(packet.getDataDescriptor(), desc);
    ASSERT_EQ(packet.getSampleCount(), 100u);
    ASSERT_EQ(packet.getOffset(), 10);
}

TEST_F(DataPacketTest, DataPacketWithDomainGetters)
{
    const auto desc = setupDescriptor(SampleType::Float64, LinearDataRule(10.5, 200), nullptr);
    const DataPacketPtr domain = DataPacket(desc, 100, 10);
    const DataPacketPtr value = DataPacketWithDomain(domain, desc, 100, 10);

    ASSERT_TRUE(*value.getDomainPacket() == *domain);
}

TEST_F(DataPacketTest, TestDoubleLinearDataRule)
{
    const auto descriptor = setupDescriptor(SampleType::Float64, LinearDataRule(10.5, 200), nullptr);
    validateImplicitLinearDataRulePacket<double>(descriptor, 10.5, 200, 1000);
}

TEST_F(DataPacketTest, TestSingleLinearDataRule)
{
    const auto descriptor = setupDescriptor(SampleType::Float32, LinearDataRule(10.5, 0.2), nullptr);
    validateImplicitLinearDataRulePacket<float>(descriptor, static_cast<float>(10.5), static_cast<float>(0.2), 1000);
}

TEST_F(DataPacketTest, TestUInt8LinearDataRule)
{
    const auto descriptor = setupDescriptor(SampleType::UInt8, LinearDataRule(2, 0), nullptr);
    validateImplicitLinearDataRulePacket<uint8_t>(descriptor, 2, 0, 0);
}

TEST_F(DataPacketTest, TestInt8LinearDataRule)
{
    const auto descriptor = setupDescriptor(SampleType::Int8, LinearDataRule(1, 0), nullptr);
    validateImplicitLinearDataRulePacket<int8_t>(descriptor, 1, 0, 0);
}

TEST_F(DataPacketTest, TestUInt16LinearDataRule)
{
    const auto descriptor = setupDescriptor(SampleType::UInt16, LinearDataRule(3, 30), nullptr);
    validateImplicitLinearDataRulePacket<uint16_t>(descriptor, 3, 30, 100);
}

TEST_F(DataPacketTest, TestInt16LinearDataRule)
{
    const auto descriptor = setupDescriptor(SampleType::Int16, LinearDataRule(3, 30), nullptr);
    validateImplicitLinearDataRulePacket<int16_t>(descriptor, 3, 30, 100);
}

TEST_F(DataPacketTest, TestUInt32LinearDataRule)
{
    const auto descriptor = setupDescriptor(SampleType::UInt32, LinearDataRule(5, 9), nullptr);
    validateImplicitLinearDataRulePacket<uint32_t>(descriptor, 5, 9, 100);
}

TEST_F(DataPacketTest, TestInt32LinearDataRule)
{
    const auto descriptor = setupDescriptor(SampleType::Int32, LinearDataRule(5, 9), nullptr);
    validateImplicitLinearDataRulePacket<int32_t>(descriptor, 5, 9, 100);
}

TEST_F(DataPacketTest, TestUInt64LinearDataRule)
{
    const auto descriptor = setupDescriptor(SampleType::UInt64, LinearDataRule(10.5, 0), nullptr);
    validateImplicitLinearDataRulePacket<uint64_t>(descriptor, 10, 0, 1000);
}

TEST_F(DataPacketTest, TestInt64LinearDataRule)
{
    const auto descriptor = setupDescriptor(SampleType::Int64, LinearDataRule(10.5, 0), nullptr);
    validateImplicitLinearDataRulePacket<int64_t>(descriptor, 10, 0, 1000);
}

TEST_F(DataPacketTest, TestSingleConstantDataRule)
{
    const auto descriptor = setupDescriptor(SampleType::Float32, ConstantDataRule(), nullptr);
}

TEST_F(DataPacketTest, TestDoubleLinearScaling)
{
    auto descriptor =
        setupDescriptor(SampleType::Float32, ExplicitDataRule(), LinearScaling(4.5, 100.2, SampleType::Float64, ScaledSampleType::Float32));
    validateLinearScalingPacket<double, float>(descriptor, static_cast<float>(4.5), static_cast<float>(100.2));

    descriptor =
        setupDescriptor(SampleType::Float64, ExplicitDataRule(), LinearScaling(4.5, 100.2, SampleType::Float64, ScaledSampleType::Float64));
    validateLinearScalingPacket<double, double>(descriptor, 4.5, 100.2);
}

TEST_F(DataPacketTest, TestUInt8LinearScaling)
{
    auto descriptor =
        setupDescriptor(SampleType::Float32, ExplicitDataRule(), LinearScaling(2, 5, SampleType::UInt8, ScaledSampleType::Float32));
    validateLinearScalingPacket<uint8_t, float>(descriptor, 2, 5);

    descriptor =
        setupDescriptor(SampleType::Float64, ExplicitDataRule(), LinearScaling(2, 5, SampleType::UInt8, ScaledSampleType::Float64));
    validateLinearScalingPacket<uint8_t, double>(descriptor, 2, 5);
}

TEST_F(DataPacketTest, TestInt8LinearScaling)
{
    auto descriptor =
        setupDescriptor(SampleType::Float32, ExplicitDataRule(), LinearScaling(1, 0, SampleType::Int8, ScaledSampleType::Float32));
    validateLinearScalingPacket<int8_t, float>(descriptor, 1, 0);

    descriptor = setupDescriptor(SampleType::Float64, ExplicitDataRule(), LinearScaling(1, 0, SampleType::Int8, ScaledSampleType::Float64));
    validateLinearScalingPacket<int8_t, double>(descriptor, 1, 0);
}

TEST_F(DataPacketTest, TestUInt16LinearScaling)
{
    auto descriptor =
        setupDescriptor(SampleType::Float32, ExplicitDataRule(), LinearScaling(4, 15, SampleType::UInt16, ScaledSampleType::Float32));
    validateLinearScalingPacket<uint16_t, float>(descriptor, 4, 15);

    descriptor =
        setupDescriptor(SampleType::Float64, ExplicitDataRule(), LinearScaling(4, 15, SampleType::UInt16, ScaledSampleType::Float64));
    validateLinearScalingPacket<uint16_t, double>(descriptor, 4, 15);
}

TEST_F(DataPacketTest, TestInt16LinearScaling)
{
    auto descriptor =
        setupDescriptor(SampleType::Float32, ExplicitDataRule(), LinearScaling(4, 15, SampleType::Int16, ScaledSampleType::Float32));
    validateLinearScalingPacket<int16_t, float>(descriptor, 4, 15);

    descriptor =
        setupDescriptor(SampleType::Float64, ExplicitDataRule(), LinearScaling(4, 15, SampleType::Int16, ScaledSampleType::Float64));
    validateLinearScalingPacket<int16_t, double>(descriptor, 4, 15);
}

TEST_F(DataPacketTest, TestUInt32LinearScaling)
{
    auto descriptor =
        setupDescriptor(SampleType::Float32, ExplicitDataRule(), LinearScaling(102, 1000, SampleType::UInt32, ScaledSampleType::Float32));
    validateLinearScalingPacket<uint32_t, float>(descriptor, 102, 1000);

    descriptor =
        setupDescriptor(SampleType::Float64, ExplicitDataRule(), LinearScaling(102, 1000, SampleType::UInt32, ScaledSampleType::Float64));
    validateLinearScalingPacket<uint32_t, double>(descriptor, 102, 1000);
}

TEST_F(DataPacketTest, TestInt32LinearScaling)
{
    auto descriptor =
        setupDescriptor(SampleType::Float32, ExplicitDataRule(), LinearScaling(102, 1000, SampleType::Int32, ScaledSampleType::Float32));
    validateLinearScalingPacket<int32_t, float>(descriptor, 102, 1000);

    descriptor =
        setupDescriptor(SampleType::Float64, ExplicitDataRule(), LinearScaling(102, 1000, SampleType::Int32, ScaledSampleType::Float64));
    validateLinearScalingPacket<int32_t, double>(descriptor, 102, 1000);
}

TEST_F(DataPacketTest, TestUInt64LinearScaling)
{
    auto descriptor =
        setupDescriptor(SampleType::Float32, ExplicitDataRule(), LinearScaling(1012, 10020, SampleType::UInt64, ScaledSampleType::Float32));
    validateLinearScalingPacket<uint64_t, float>(descriptor, 1012, 10020);

    descriptor =
        setupDescriptor(SampleType::Float64, ExplicitDataRule(), LinearScaling(1012, 10020, SampleType::UInt64, ScaledSampleType::Float64));
    validateLinearScalingPacket<uint64_t, double>(descriptor, 1012, 10020);
}

TEST_F(DataPacketTest, TestInt64LinearScaling)
{
    auto descriptor =
        setupDescriptor(SampleType::Float32, ExplicitDataRule(), LinearScaling(1012, 10020, SampleType::Int64, ScaledSampleType::Float32));
    validateLinearScalingPacket<int64_t, float>(descriptor, 1012, 10020);

    descriptor =
        setupDescriptor(SampleType::Float64, ExplicitDataRule(), LinearScaling(1012, 10020, SampleType::Int64, ScaledSampleType::Float64));
    validateLinearScalingPacket<int64_t, double>(descriptor, 1012, 10020);
}

template <typename DataType>
class ConstantRuleTest : public DataPacketTest
{
public:
    static_assert(std::is_arithmetic_v<DataType>);

    void DoTestWithSingleValue()
    {
        auto descriptor = setupDescriptor(SampleTypeFromType<DataType>::SampleType, ConstantDataRule(), nullptr);

        if constexpr (std::is_integral_v<DataType>)
            validateImplicitConstantDataRulePacket<DataType>(descriptor, 123);
        else if constexpr (std::is_floating_point_v<DataType>)
            validateImplicitConstantDataRulePacket<DataType>(descriptor, static_cast<DataType>(20.5));
    }

    void DoTestWithMultipleValues()
    {
        constexpr size_t sampleCount = 100;
        auto sampleType = SampleTypeFromType<DataType>::SampleType;
        auto descriptor = setupDescriptor(sampleType, ConstantDataRule(), nullptr);
        const DataPacketPtr packet =
            ConstantDataPacketWithDomain<DataType>(nullptr, descriptor, sampleCount, 12, {{10, 16}, {70, 18}, {90, 20}});
        const auto scaledData = static_cast<DataType*>(packet.getData());
        if constexpr (std::is_unsigned_v<DataType>)
        {
            for (uint64_t i = 0; i < 9; ++i)
                ASSERT_EQ(scaledData[i], 12u);
            for (uint64_t i = 10; i < 69; ++i)
                ASSERT_EQ(scaledData[i], 16u);
            for (uint64_t i = 70; i < 89; ++i)
                ASSERT_EQ(scaledData[i], 18u);
            for (uint64_t i = 90; i < packet.getSampleCount(); ++i)
                ASSERT_EQ(scaledData[i], 20u);
        }
        else
        {
            for (uint64_t i = 0; i < 9; ++i)
                ASSERT_EQ(scaledData[i], 12);
            for (uint64_t i = 10; i < 69; ++i)
                ASSERT_EQ(scaledData[i], 16);
            for (uint64_t i = 70; i < 89; ++i)
                ASSERT_EQ(scaledData[i], 18);
            for (uint64_t i = 90; i < packet.getSampleCount(); ++i)
                ASSERT_EQ(scaledData[i], 20);
        }

        ASSERT_EQ(packet.getSampleCount(), sampleCount);
        ASSERT_EQ(packet.getRawDataSize(), getSampleSize(sampleType) * 4 + 12);
        ASSERT_EQ(packet.getDataSize(), getSampleSize(sampleType) * sampleCount);
    }
};

TYPED_TEST_SUITE_P(ConstantRuleTest);

TYPED_TEST_P(ConstantRuleTest, TestConstantRule)
{
    this->DoTestWithSingleValue();
}

TYPED_TEST_P(ConstantRuleTest, TestConstantRuleWithMultipleValues)
{
    this->DoTestWithMultipleValues();
}

REGISTER_TYPED_TEST_SUITE_P(ConstantRuleTest, TestConstantRule, TestConstantRuleWithMultipleValues);

using SampleTypes = testing::Types<int8_t, int16_t, int32_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t, float, double>;
INSTANTIATE_TYPED_TEST_SUITE_P(ConstantRuleTestTyped, ConstantRuleTest, SampleTypes);

TEST_F(DataPacketTest, TestRangeType)
{
    RangeType<uint64_t> t1(10, 20);
    RangeType<uint64_t> t2(10, 20);
    RangeType<uint64_t> t3(20, 10);

    ASSERT_TRUE(t1 == t2);
    ASSERT_FALSE(t1 != t2);
    ASSERT_FALSE(t1 == t3);
    ASSERT_TRUE(t1 != t3);
}

TEST_F(DataPacketTest, PacketEqualsExplicit)
{
    auto descriptor = setupDescriptor(SampleType::UInt16, ExplicitDataRule(), nullptr);
    auto packet1 = createExplicitPacket<uint16_t, 100>(descriptor);
    auto packet2 = createExplicitPacket<uint16_t, 100>(descriptor);
    auto packet3 = createExplicitPacket<uint16_t, 50>(descriptor);

    ASSERT_EQ(packet1, packet2);
    ASSERT_NE(packet1, packet3);
}

TEST_F(DataPacketTest, PacketEqualsExplicitWithPostScale)
{
    auto descriptor =
        setupDescriptor(SampleType::Float64, ExplicitDataRule(), LinearScaling(4, 15, SampleType::UInt16, ScaledSampleType::Float64));
    auto packet1 = createExplicitPacket<uint16_t, 100>(descriptor);
    auto packet2 = createExplicitPacket<uint16_t, 100>(descriptor);
    auto packet3 = createExplicitPacket<uint16_t, 50>(descriptor);

    ASSERT_EQ(packet1, packet2);
    ASSERT_NE(packet1, packet3);
}

TEST_F(DataPacketTest, PacketEqualsImplicit)
{
    auto descriptor = setupDescriptor(SampleType::UInt64, LinearDataRule(1, 0), nullptr);
    auto packet1 = DataPacket(descriptor, 100, 100);
    auto packet2 = DataPacket(descriptor, 100, 100);
    auto packet3 = DataPacket(descriptor, 100, 50);
    auto packet4 = DataPacket(descriptor, 50, 100);

    ASSERT_EQ(packet1, packet2);
    ASSERT_NE(packet1, packet3);
    ASSERT_NE(packet1, packet4);
}

TEST_F(DataPacketTest, PacketEqualsMixTypes)
{
    auto packetExplicit = createExplicitPacket<uint16_t, 100>(setupDescriptor(SampleType::Float64, ExplicitDataRule(), nullptr));
    auto packetExplicitWithPostScaling = createExplicitPacket<uint16_t, 100>(
        setupDescriptor(SampleType::Float64, ExplicitDataRule(), LinearScaling(4, 15, SampleType::UInt16, ScaledSampleType::Float64)));
    auto packetimplicit = DataPacket(setupDescriptor(SampleType::UInt64, LinearDataRule(1, 0), nullptr), 100, 100);

    ASSERT_NE(packetExplicit, packetExplicitWithPostScaling);
    ASSERT_NE(packetExplicit, packetimplicit);
    ASSERT_NE(packetExplicitWithPostScaling, packetimplicit);
}

TEST_F(DataPacketTest, PacketRefCount)
{
    const auto desc = setupDescriptor(SampleType::Float64, LinearDataRule(10.5, 200), nullptr);
    const DataPacketPtr packet = DataPacket(desc, 100, 10);

    ASSERT_EQ(packet.getRefCount(), 1u);
}

TEST_F(DataPacketTest, PacketWithStructSampleType)
{
    const auto arbIdDescriptor =
        DataDescriptorBuilder().setName("ArbId").setSampleType(SampleType::UInt32).setRule(ExplicitDataRule()).build();

    const auto lengthDescriptor =
        DataDescriptorBuilder().setName("Length").setSampleType(SampleType::UInt8).setRule(ExplicitDataRule()).build();

    const auto dataDescriptor = DataDescriptorBuilder()
                                    .setName("Data")
                                    .setSampleType(SampleType::UInt8)
                                    .setDimensions(List<IDimension>(DimensionBuilder().setRule(LinearDimensionRule(0, 1, 64)).build()))
                                    .setRule(ExplicitDataRule())
                                    .build();

    const auto canMsgDescriptor = DataDescriptorBuilder()
                                      .setName("CAN")
                                      .setSampleType(SampleType::Struct)
                                      .setStructFields(List<IDataDescriptor>(arbIdDescriptor, lengthDescriptor, dataDescriptor))
                                      .build();

    const DataPacketPtr packet = DataPacket(canMsgDescriptor, 100, 0);
}

TEST_F(DataPacketTest, QueryInterface)
{
    auto descriptor =
        setupDescriptor(SampleType::Float64, ExplicitDataRule(), LinearScaling(4, 15, SampleType::UInt16, ScaledSampleType::Float64));
    auto packet = createExplicitPacket<uint16_t, 100>(descriptor);

    auto packet1 = packet.asPtr<IPacket>();
    auto packet2 = packet1.asPtr<IDataPacket>();

    ASSERT_EQ(packet2.getSampleCount(), packet.getSampleCount());

    auto packet3 = packet.asPtr<IPacket>(true);
    auto packet4 = packet3.asPtr<IDataPacket>(true);

    ASSERT_EQ(packet4.getSampleCount(), packet.getSampleCount());
}

TEST_F(DataPacketTest, Reuse)
{
    auto descriptor = setupDescriptor(SampleType::Float64, ExplicitDataRule(), nullptr);
    auto packet = createExplicitPacket<uint16_t, 100>(descriptor);

    const auto dataPtr = packet.getRawData();

    auto newDescriptor = setupDescriptor(SampleType::Float32, ExplicitDataRule(), nullptr);
    bool success = packet.asPtr<IReusableDataPacket>(true).reuse(newDescriptor, 100, 1000, nullptr, false);
    ASSERT_TRUE(success);

    ASSERT_EQ(packet.getRawData(), dataPtr);
    ASSERT_EQ(packet.getOffset(), 1000u);
    ASSERT_EQ(packet.getSampleCount(), 100u);
    ASSERT_EQ(packet.getDataDescriptor(), newDescriptor);
}

TEST_F(DataPacketTest, ReuseSameDescriptorOffsetAndSampleCount)
{
    auto descriptor = setupDescriptor(SampleType::Float64, ExplicitDataRule(), nullptr);
    auto packet = createExplicitPacket<uint16_t, 100>(descriptor);

    const auto dataPtr = packet.getRawData();

    bool success = packet.asPtr<IReusableDataPacket>(true).reuse(nullptr, std::numeric_limits<SizeT>::max(), nullptr, nullptr, false);
    ASSERT_TRUE(success);

    ASSERT_EQ(packet.getRawData(), dataPtr);
    ASSERT_EQ(packet.getOffset(), nullptr);
    ASSERT_EQ(packet.getSampleCount(), 100u);
    ASSERT_EQ(packet.getDataDescriptor(), descriptor);
}

TEST_F(DataPacketTest, ReuseDenyBiggerSampleType)
{
    auto descriptor = setupDescriptor(SampleType::Float32, ExplicitDataRule(), nullptr);
    auto packet = createExplicitPacket<uint16_t, 100>(descriptor);

    auto newDescriptor = setupDescriptor(SampleType::Float64, ExplicitDataRule(), nullptr);
    bool success = packet.asPtr<IReusableDataPacket>(true).reuse(newDescriptor, 100, nullptr, nullptr, false);
    ASSERT_FALSE(success);
}

TEST_F(DataPacketTest, ReuseDenyMoreSamples)
{
    auto descriptor = setupDescriptor(SampleType::Float32, ExplicitDataRule(), nullptr);
    auto packet = createExplicitPacket<uint16_t, 100>(descriptor);

    auto newDescriptor = setupDescriptor(SampleType::Float32, ExplicitDataRule(), nullptr);
    bool success = packet.asPtr<IReusableDataPacket>(true).reuse(newDescriptor, 200, nullptr, nullptr, false);
    ASSERT_FALSE(success);
}

TEST_F(DataPacketTest, ReuseAllowBiggerSampleTypeLessSamples)
{
    auto descriptor = setupDescriptor(SampleType::Float32, ExplicitDataRule(), nullptr);
    auto packet = createExplicitPacket<uint16_t, 100>(descriptor);

    auto newDescriptor = setupDescriptor(SampleType::Float64, ExplicitDataRule(), nullptr);
    bool success = packet.asPtr<IReusableDataPacket>(true).reuse(newDescriptor, 50, nullptr, nullptr, false);
    ASSERT_TRUE(success);
}

TEST_F(DataPacketTest, ReuseAllowBiggerSampleTypeWithRealloc)
{
    auto descriptor = setupDescriptor(SampleType::Float32, ExplicitDataRule(), nullptr);
    auto packet = createExplicitPacket<uint16_t, 100>(descriptor);

    auto newDescriptor = setupDescriptor(SampleType::Float64, ExplicitDataRule(), nullptr);
    bool success = packet.asPtr<IReusableDataPacket>(true).reuse(newDescriptor, 100, nullptr, nullptr, true);
    ASSERT_TRUE(success);
}

TEST_F(DataPacketTest, GetLastValue)
{
    const auto descriptor = DataDescriptorBuilder().setSampleType(SampleType::Int32).build();
    const auto packet = DataPacket(descriptor, 5);
    const auto data = static_cast<int32_t*>(packet.getData());
    data[4] = 42;

    const auto lv = packet.getLastValue();
    IntegerPtr ptr;
    ASSERT_NO_THROW(ptr = lv.asPtr<IInteger>());
    ASSERT_EQ(ptr, 42);
}

TEST_F(DataPacketTest, GetLastValueConstantPosAndValue)
{
    std::vector<daq::ConstantPosAndValue<uint64_t>> constantPosAndValue;
    constantPosAndValue.push_back({0, 0});
    constantPosAndValue.push_back({5, 5});
    constantPosAndValue.push_back({10, 10});
    constantPosAndValue.push_back({15, 15});

    DataPacketPtr packet;

    ASSERT_NO_THROW(packet = ConstantDataPacketWithDomain<uint64_t>(
                        nullptr,
                        DataDescriptorBuilder().setSampleType(SampleType::UInt64).setRule(ConstantDataRule()).build(),
                        50,
                        0,
                        constantPosAndValue));

    uint64_t lastValue;

    ASSERT_NO_THROW(lastValue = packet.getLastValue());

    ASSERT_EQ(lastValue, 15u);
}

TEST_F(DataPacketTest, GetLastValueNested)
{
    const auto descriptor =
        DataDescriptorBuilder()
            .setName("MyTestStructType")
            .setSampleType(SampleType::Struct)
            .setStructFields(List<DataDescriptorPtr>(DataDescriptorBuilder().setName("Int32").setSampleType(SampleType::Int32).build()))
            .build();

    const auto packet = DataPacket(descriptor, 5);
    const auto data = static_cast<int32_t*>(packet.getData());
    data[4] = 42;

    auto fieldNames = List<IString>();
    auto fieldTypes = List<IType>();

    fieldNames.pushBack("Int32");
    fieldTypes.pushBack(SimpleType(CoreType::ctInt));

    const auto structType = StructType("MyTestStructType", fieldNames, fieldTypes);

    const auto manager = TypeManager();
    manager.addType(structType);

    BaseObjectPtr lv;

    ASSERT_NO_THROW(lv = packet.getLastValue(manager));

    StructPtr ptr;

    ASSERT_NO_THROW(ptr = lv.asPtr<IStruct>());

    ASSERT_EQ(ptr.get("Int32"), 42);
}

TEST_F(DataPacketTest, GetLastValueNoTypeManager)
{
    const auto descriptor =
        DataDescriptorBuilder()
            .setName("MyTestStructType")
            .setSampleType(SampleType::Struct)
            .setStructFields(List<DataDescriptorPtr>(DataDescriptorBuilder().setName("Int32").setSampleType(SampleType::Int32).build()))
            .build();

    const auto packet = DataPacket(descriptor, 5);
    const auto data = static_cast<int32_t*>(packet.getData());
    data[4] = 42;

    ASSERT_THROW(packet.getLastValue(), InvalidParameterException);
}

TEST_F(DataPacketTest, ReferenceDomainOffsetLinearDataRuleInt8)
{
    const auto descriptor = DataDescriptorBuilder()
                                .setSampleType(SampleType::Int8)
                                .setRule(LinearDataRule(2, 6))
                                .setReferenceDomainInfo(ReferenceDomainInfoBuilder().setReferenceDomainOffset(100).build())
                                .build();
    const auto packet = DataPacket(descriptor, 3, 2);

    // offset = 2, start = 6, referenceDomainOffset = 100: 2 + 6 + 100 = 108
    int8_t expected[] = {108, 110, 112};

    const auto data1 = static_cast<int8_t*>(packet.getData());

    ASSERT_EQ(expected[0], data1[0]);
    ASSERT_EQ(expected[1], data1[1]);
    ASSERT_EQ(expected[2], data1[2]);

    // Tests a different path

    const auto data2 = static_cast<int8_t*>(packet.getData());

    ASSERT_EQ(expected[0], data2[0]);
    ASSERT_EQ(expected[1], data2[1]);
    ASSERT_EQ(expected[2], data2[2]);
}

TEST_F(DataPacketTest, ReferenceDomainOffsetLinearDataRuleInt32)
{
    const auto descriptor = DataDescriptorBuilder()
                                .setSampleType(SampleType::Int32)
                                .setRule(LinearDataRule(2, 6))
                                .setReferenceDomainInfo(ReferenceDomainInfoBuilder().setReferenceDomainOffset(100).build())
                                .build();
    const auto packet = DataPacket(descriptor, 3, 2);

    // offset = 2, start = 6, referenceDomainOffset = 100: 2 + 6 + 100 = 108
    int32_t expected[] = {108, 110, 112};

    const auto data1 = static_cast<int32_t*>(packet.getData());

    ASSERT_EQ(expected[0], data1[0]);
    ASSERT_EQ(expected[1], data1[1]);
    ASSERT_EQ(expected[2], data1[2]);

    // Tests a different path

    const auto data2 = static_cast<int32_t*>(packet.getData());

    ASSERT_EQ(expected[0], data2[0]);
    ASSERT_EQ(expected[1], data2[1]);
    ASSERT_EQ(expected[2], data2[2]);
}

TEST_F(DataPacketTest, ReferenceDomainOffsetLinearDataRuleInt64)
{
    const auto descriptor = DataDescriptorBuilder()
                                .setSampleType(SampleType::Int64)
                                .setRule(LinearDataRule(2, 6))
                                .setReferenceDomainInfo(ReferenceDomainInfoBuilder().setReferenceDomainOffset(100).build())
                                .build();
    const auto packet = DataPacket(descriptor, 3, 2);

    // offset = 2, start = 6, referenceDomainOffset = 100: 2 + 6 + 100 = 108
    int64_t expected[] = {108, 110, 112};

    const auto data1 = static_cast<int64_t*>(packet.getData());

    ASSERT_EQ(expected[0], data1[0]);
    ASSERT_EQ(expected[1], data1[1]);
    ASSERT_EQ(expected[2], data1[2]);

    // Tests a different path

    const auto data2 = static_cast<int64_t*>(packet.getData());

    ASSERT_EQ(expected[0], data2[0]);
    ASSERT_EQ(expected[1], data2[1]);
    ASSERT_EQ(expected[2], data2[2]);
}

TEST_F(DataPacketTest, ReferenceDomainOffsetExplicitDataRule)
{
    const auto descriptor = DataDescriptorBuilder()
                                .setSampleType(SampleType::Int32)
                                .setRule(ExplicitDataRule())
                                .setReferenceDomainInfo(ReferenceDomainInfoBuilder().setReferenceDomainOffset(100).build())
                                .build();
    const auto packet = DataPacket(descriptor, 3);

    const auto rawData = static_cast<int32_t*>(packet.getRawData());
    rawData[0] = 8;
    rawData[1] = 10;
    rawData[2] = 12;

    int32_t expected[] = {108, 110, 112};

    const auto data1 = static_cast<int32_t*>(packet.getData());

    ASSERT_EQ(expected[0], data1[0]);
    ASSERT_EQ(expected[1], data1[1]);
    ASSERT_EQ(expected[2], data1[2]);

    // Tests a different path

    const auto data2 = static_cast<int32_t*>(packet.getData());

    ASSERT_EQ(expected[0], data2[0]);
    ASSERT_EQ(expected[1], data2[1]);
    ASSERT_EQ(expected[2], data2[2]);
}

TEST_F(DataPacketTest, GetValueByIndex)
{
    // DataPacket
    {
        // 0-dimension
        {
            // Non-struct
            {
                // Linear data rule
                {
                    const auto descriptor = setupDescriptor(SampleType::Int32, LinearDataRule(1, 0), nullptr);
                    const auto packet = DataPacket(descriptor, 3, 0);
                    IntegerPtr sampleValue0;
                    IntegerPtr sampleValue1;
                    IntegerPtr sampleValue2;
                    ASSERT_NO_THROW(sampleValue0 = packet.getValueByIndex(0));
                    ASSERT_NO_THROW(sampleValue1 = packet.getValueByIndex(1));
                    ASSERT_NO_THROW(sampleValue2 = packet.getValueByIndex(2));
                    ASSERT_EQ(sampleValue0, 0);
                    ASSERT_EQ(sampleValue1, 1);
                    ASSERT_EQ(sampleValue2, 2);
                }
                // Constant data rule
                {
                    const auto descriptor = setupDescriptor(SampleType::Int32, ConstantDataRule(), nullptr);
                    const auto packet = ConstantDataPacketWithDomain(nullptr, descriptor, 3, 2);
                    IntegerPtr sampleValue0;
                    IntegerPtr sampleValue1;
                    IntegerPtr sampleValue2;
                    ASSERT_NO_THROW(sampleValue0 = packet.getValueByIndex(0));
                    ASSERT_NO_THROW(sampleValue1 = packet.getValueByIndex(1));
                    ASSERT_NO_THROW(sampleValue2 = packet.getValueByIndex(2));
                    ASSERT_EQ(sampleValue0, 2);
                    ASSERT_EQ(sampleValue1, 2);
                    ASSERT_EQ(sampleValue2, 2);
                }
                // Explicit data rule
                {
                    const auto descriptor = setupDescriptor(SampleType::Int32, ExplicitDataRule(), nullptr);
                    const auto packet = DataPacket(descriptor, 3, 0);
                    using SampleTypePtr = SampleTypeToType<SampleType::Int32>::Type*;
                    auto* data = static_cast<SampleTypePtr>(packet.getRawData());
                    data[0] = 0;
                    data[1] = 1;
                    data[2] = 2;
                    IntegerPtr sampleValue0;
                    IntegerPtr sampleValue1;
                    IntegerPtr sampleValue2;
                    ASSERT_NO_THROW(sampleValue0 = packet.getValueByIndex(0));
                    ASSERT_NO_THROW(sampleValue1 = packet.getValueByIndex(1));
                    ASSERT_NO_THROW(sampleValue2 = packet.getValueByIndex(2));
                    ASSERT_EQ(sampleValue0, 0);
                    ASSERT_EQ(sampleValue1, 1);
                    ASSERT_EQ(sampleValue2, 2);
                }
                // Explicit domain data rule
                {
                }
                // Post scaled
                {
                    constexpr auto sampleType = SampleType::Float64;
                    constexpr auto scaledSampleType = ScaledSampleType::Float64;
                    constexpr auto scale = 2;
                    constexpr auto offset = 1;
                    const auto postScaling = LinearScaling(scale, offset, sampleType, scaledSampleType);
                    const auto descriptor =
                        DataDescriptorBuilder().setSampleType(sampleType).setRule(ExplicitDataRule()).setPostScaling(postScaling).build();
                    const auto packet = DataPacket(descriptor, 3, 0);
                    using SampleTypePtr = SampleTypeToType<sampleType>::Type*;
                    auto* data = static_cast<SampleTypePtr>(packet.getRawData());
                    data[0] = 0;
                    data[1] = 1;
                    data[2] = 2;
                    FloatPtr sampleValue0;
                    FloatPtr sampleValue1;
                    FloatPtr sampleValue2;
                    ASSERT_NO_THROW(sampleValue0 = packet.getValueByIndex(0));
                    ASSERT_NO_THROW(sampleValue1 = packet.getValueByIndex(1));
                    ASSERT_NO_THROW(sampleValue2 = packet.getValueByIndex(2));
                    ASSERT_FLOAT_EQ(sampleValue0, 1.0);
                    ASSERT_FLOAT_EQ(sampleValue1, 3.0);
                    ASSERT_FLOAT_EQ(sampleValue2, 5.0);
                }
            }
            // Struct
            {
                // Linear data rule
                {
                }
                // Constant data rule
                {
                }
                // Explicit data rule
                {
                    auto fieldNames = List<IString>();
                    auto fieldTypes = List<IType>();

                    fieldNames.pushBack("Voltage");
                    fieldTypes.pushBack(SimpleType(CoreType::ctInt));
                    fieldNames.pushBack("Current");
                    fieldTypes.pushBack(SimpleType(CoreType::ctInt));

                    const auto structType = StructType("Electric", fieldNames, fieldTypes);
                    const auto typeManager = TypeManager();

                    typeManager.addType(structType);

                    const auto voltageDescriptor = DataDescriptorBuilder()
                                                       .setSampleType(SampleType::Int32)
                                                       .setRule(ExplicitDataRule())
                                                       .setPostScaling(nullptr)
                                                       .setName(String("Voltage"))
                                                       .build();
                    const auto currentDescriptor = DataDescriptorBuilder()
                                                       .setSampleType(SampleType::Int32)
                                                       .setRule(ExplicitDataRule())
                                                       .setPostScaling(nullptr)
                                                       .setName(String("Current"))
                                                       .build();
                    const auto structFields = List<daq::IDataDescriptor>(voltageDescriptor, currentDescriptor);
                    const auto descriptorBuilder = DataDescriptorBuilder()
                                                       .setSampleType(SampleType::Struct)
                                                       .setRule(ExplicitDataRule())
                                                       .setPostScaling(nullptr)
                                                       .setStructFields(structFields)
                                                       .setName("Electric");
                    const auto descriptor = descriptorBuilder.build();
                    constexpr auto voltageSampleType = SampleType::Int32;
                    constexpr auto currentSampleType = SampleType::Int32;
#pragma pack(push, 1)
                    struct Measurements
                    {
                        using VoltageType = SampleTypeToType<voltageSampleType>::Type;
                        using CurrentType = SampleTypeToType<currentSampleType>::Type;
                        VoltageType voltage;
                        CurrentType current;
                    };
#pragma pack(pop)
                    const auto packet = DataPacket(descriptor, 3, 0);
                    auto* rawData = static_cast<Measurements*>(packet.getRawData());
                    auto* measurements = new (rawData) Measurements[3];

                    measurements[0].voltage = 10;
                    measurements[0].current = 10;

                    measurements[1].voltage = 20;
                    measurements[1].current = 20;

                    measurements[2].voltage = 30;
                    measurements[2].current = 30;

                    StructPtr sampleValue0;
                    StructPtr sampleValue1;
                    StructPtr sampleValue2;

                    ASSERT_NO_THROW(sampleValue0 = packet.getValueByIndex(0, typeManager));
                    ASSERT_NO_THROW(sampleValue1 = packet.getValueByIndex(1, typeManager));
                    ASSERT_NO_THROW(sampleValue2 = packet.getValueByIndex(2, typeManager));

                    ASSERT_EQ(sampleValue0.get("Voltage"), 10);
                    ASSERT_EQ(sampleValue0.get("Current"), 10);
                    ASSERT_EQ(sampleValue1.get("Voltage"), 20);
                    ASSERT_EQ(sampleValue1.get("Current"), 20);
                    ASSERT_EQ(sampleValue2.get("Voltage"), 30);
                    ASSERT_EQ(sampleValue2.get("Current"), 30);
                }
                // Explicit domain data rule
                {
                }
                // Post scaled
                {
                }
            }
        }
        // 1-dimension
        {
            // Non-struct
            {
                // Linear data rule
                {
                }
                // Constant data rule
                {
                }
                // Explicit data rule
                {
                    constexpr auto subSampleType = SampleType::Int32;
                    auto dimensionLabels = List<INumber>(0, 1, 2);
                    auto dimension = DimensionBuilder().setRule(ListDimensionRule(dimensionLabels)).build();
                    auto dimensions = List<IDimension>(dimension);
                    const auto descriptor = DataDescriptorBuilder()
                                                .setSampleType(subSampleType)
                                                .setRule(ExplicitDataRule())
                                                .setPostScaling(nullptr)
                                                .setDimensions(dimensions)
                                                .build();
                    const auto packet = DataPacket(descriptor, 3, 0);
                    auto* rawData = packet.getRawData();

                    using SubSampleType = SampleTypeToType<subSampleType>::Type;
                    auto subSamples = static_cast<SubSampleType(*)[3]>(rawData);
                    subSamples[0][0] = 0;
                    subSamples[0][1] = 1;
                    subSamples[0][2] = 2;
                    subSamples[1][0] = 3;
                    subSamples[1][1] = 4;
                    subSamples[1][2] = 5;
                    subSamples[2][0] = 6;
                    subSamples[2][1] = 7;
                    subSamples[2][2] = 8;

                    ListPtr<IInteger> sampleValue0;
                    ListPtr<IInteger> sampleValue1;
                    ListPtr<IInteger> sampleValue2;
                    ASSERT_NO_THROW(sampleValue0 = packet.getValueByIndex(0));
                    ASSERT_NO_THROW(sampleValue1 = packet.getValueByIndex(1));
                    ASSERT_NO_THROW(sampleValue2 = packet.getValueByIndex(2));
                    ASSERT_THAT(sampleValue0, testing::ElementsAre(0, 1, 2));
                    ASSERT_THAT(sampleValue1, testing::ElementsAre(3, 4, 5));
                    ASSERT_THAT(sampleValue2, testing::ElementsAre(6, 7, 8));
                }
                // Explicit domain data rule
                {
                }
                // Post scaled
                {
                }
            }
            // Struct
            {
                // Linear data rule
                {
                }
                // Constant data rule
                {
                }
                // Explicit data rule
                {
                }
                // Explicit domain data rule
                {
                }
                // Post scaled
                {
                }
            }
        }
    }
    // DataPacketWithExternalMemory
    {
        // 0-dimension
        {
            // Linear data rule
            {
                constexpr auto sampleType = SampleType::Int32;
                constexpr auto sampleCount = SizeT{3};
                SampleTypeToType<sampleType>::Type data[sampleCount] = {};
                const auto descriptor = setupDescriptor(sampleType, LinearDataRule(1, 0), nullptr);
                const auto packet = DataPacketWithExternalMemory(
                    nullptr,
                    descriptor,
                    sampleCount,
                    data,
                    Deleter([](auto* memory) {}),
                    0,
                    3);
                IntegerPtr sampleValue0;
                IntegerPtr sampleValue1;
                IntegerPtr sampleValue2;
                ASSERT_NO_THROW(sampleValue0 = packet.getValueByIndex(0));
                ASSERT_NO_THROW(sampleValue1 = packet.getValueByIndex(1));
                ASSERT_NO_THROW(sampleValue2 = packet.getValueByIndex(2));
                ASSERT_EQ(sampleValue0, 0);
                ASSERT_EQ(sampleValue1, 1);
                ASSERT_EQ(sampleValue2, 2);
            }
            // Explicit data rule
            {
                constexpr auto sampleType = SampleType::Int32;
                constexpr auto sampleCount = SizeT{3};
                SampleTypeToType<sampleType>::Type data[sampleCount] = {0, 1, 2};
                const auto descriptor = setupDescriptor(sampleType, ExplicitDataRule(), nullptr);
                const auto packet = DataPacketWithExternalMemory(
                    nullptr,
                    descriptor,
                    sampleCount,
                    data,
                    Deleter([](auto* memory) {}),
                    0,
                    3);
                IntegerPtr sampleValue0;
                IntegerPtr sampleValue1;
                IntegerPtr sampleValue2;
                ASSERT_NO_THROW(sampleValue0 = packet.getValueByIndex(0));
                ASSERT_NO_THROW(sampleValue1 = packet.getValueByIndex(1));
                ASSERT_NO_THROW(sampleValue2 = packet.getValueByIndex(2));
                ASSERT_EQ(sampleValue0, 0);
                ASSERT_EQ(sampleValue1, 1);
                ASSERT_EQ(sampleValue2, 2);
            }
            // Post scaled
            {
                constexpr auto sampleType = SampleType::Float64;
                constexpr auto scaledSampleType = ScaledSampleType::Float64;
                constexpr auto sampleCount = SizeT{3};
                constexpr auto scale = 2;
                constexpr auto offset = 1;
                const auto postScaling = LinearScaling(scale, offset, sampleType, scaledSampleType);
                SampleTypeToType<sampleType>::Type data[sampleCount] = {0.0, 1.0, 2.0};
                const auto descriptor =
                    DataDescriptorBuilder().setSampleType(sampleType).setRule(ExplicitDataRule()).setPostScaling(postScaling).build();
                const auto packet = DataPacketWithExternalMemory(
                    nullptr,
                    descriptor,
                    sampleCount,
                    data,
                    Deleter([](auto* memory) {}),
                    0,
                    getSampleSize(sampleType) * sampleCount);
                FloatPtr sampleValue0;
                FloatPtr sampleValue1;
                FloatPtr sampleValue2;
                ASSERT_NO_THROW(sampleValue0 = packet.getValueByIndex(0));
                ASSERT_NO_THROW(sampleValue1 = packet.getValueByIndex(1));
                ASSERT_NO_THROW(sampleValue2 = packet.getValueByIndex(2));
                ASSERT_FLOAT_EQ(sampleValue0, 1.0);
                ASSERT_EQ(sampleValue1, 3.0);
                ASSERT_EQ(sampleValue2, 5.0);
            }
            // Explicit data rule
            {
                auto fieldNames = List<IString>();
                auto fieldTypes = List<IType>();

                fieldNames.pushBack("Voltage");
                fieldTypes.pushBack(SimpleType(CoreType::ctInt));
                fieldNames.pushBack("Current");
                fieldTypes.pushBack(SimpleType(CoreType::ctInt));

                const auto structType = StructType("Electric", fieldNames, fieldTypes);
                const auto typeManager = TypeManager();

                typeManager.addType(structType);

                const auto voltageDescriptor = DataDescriptorBuilder()
                                                   .setSampleType(SampleType::Int32)
                                                   .setRule(ExplicitDataRule())
                                                   .setPostScaling(nullptr)
                                                   .setName(String("Voltage"))
                                                   .build();
                const auto currentDescriptor = DataDescriptorBuilder()
                                                   .setSampleType(SampleType::Int32)
                                                   .setRule(ExplicitDataRule())
                                                   .setPostScaling(nullptr)
                                                   .setName(String("Current"))
                                                   .build();
                const auto structFields = List<daq::IDataDescriptor>(voltageDescriptor, currentDescriptor);
                const auto descriptorBuilder = DataDescriptorBuilder()
                                                   .setSampleType(SampleType::Struct)
                                                   .setRule(ExplicitDataRule())
                                                   .setPostScaling(nullptr)
                                                   .setStructFields(structFields)
                                                   .setName("Electric");
                const auto descriptor = descriptorBuilder.build();
                constexpr auto voltageSampleType = SampleType::Int32;
                constexpr auto currentSampleType = SampleType::Int32;
#pragma pack(push, 1)
                struct Measurements
                {
                    using VoltageType = SampleTypeToType<voltageSampleType>::Type;
                    using CurrentType = SampleTypeToType<currentSampleType>::Type;
                    VoltageType voltage;
                    CurrentType current;
                };
#pragma pack(pop)
                // const auto packet = DataPacket(descriptor, 3, 0);
                // auto* measurements = new Measurements[3];
                constexpr auto sampleCount = 3;
                std::unique_ptr<Measurements[]> measurements{new Measurements[sampleCount]};
                const auto packet = DataPacketWithExternalMemory(
                    nullptr,
                    descriptor,
                    sampleCount,
                    measurements.get(),
                    Deleter([](auto* memory) {}),
                    0,
                    sampleCount * sizeof(Measurements));

                measurements[0].voltage = 10;
                measurements[0].current = 10;

                measurements[1].voltage = 20;
                measurements[1].current = 20;

                measurements[2].voltage = 30;
                measurements[2].current = 30;

                StructPtr sampleValue0;
                StructPtr sampleValue1;
                StructPtr sampleValue2;

                ASSERT_NO_THROW(sampleValue0 = packet.getValueByIndex(0, typeManager));
                ASSERT_NO_THROW(sampleValue1 = packet.getValueByIndex(1, typeManager));
                ASSERT_NO_THROW(sampleValue2 = packet.getValueByIndex(2, typeManager));

                ASSERT_EQ(sampleValue0.get("Voltage"), 10);
                ASSERT_EQ(sampleValue0.get("Current"), 10);
                ASSERT_EQ(sampleValue1.get("Voltage"), 20);
                ASSERT_EQ(sampleValue1.get("Current"), 20);
                ASSERT_EQ(sampleValue2.get("Voltage"), 30);
                ASSERT_EQ(sampleValue2.get("Current"), 30);
            }
        }
    }
}

