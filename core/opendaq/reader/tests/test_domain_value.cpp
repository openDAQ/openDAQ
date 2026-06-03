#include <gtest/gtest.h>

#include <coretypes/exceptions.h>

#include <opendaq/domain_value.h>
#include <opendaq/reader_utils.h>

#include <chrono>

using DomainValueTest = testing::Test;

TEST_F(DomainValueTest, DomainInfoComparison)
{
    daq::DomainInfo info1 = {daq::reader::parseEpoch("1970-01-01T00:00:00+00:00"), daq::Ratio(1, 1000)};
    daq::DomainInfo info2 = {daq::reader::parseEpoch("1970-01-01T00:00:00+00:00"), daq::Ratio(1, 1000)};
    ASSERT_TRUE(info1 == info2);

    // Different epoch
    daq::DomainInfo info3 = {daq::reader::parseEpoch("1999-01-01T00:00:00+00:00"), daq::Ratio(1, 1000)};
    ASSERT_FALSE(info3 == info1);

    // Different resolution denominator
    daq::DomainInfo info4 = {daq::reader::parseEpoch("1970-01-01T00:00:00+00:00"), daq::Ratio(1, 2000)};
    ASSERT_FALSE(info4 == info1);

    // Different resolution numerator
    daq::DomainInfo info5 = {daq::reader::parseEpoch("1970-01-01T00:00:00+00:00"), daq::Ratio(4, 1000)};
    ASSERT_FALSE(info5 == info1);

    // Different everything
    daq::DomainInfo info6 = {daq::reader::parseEpoch("1999-01-01T00:00:00+00:00"), daq::Ratio(5, 2000)};
    ASSERT_FALSE(info6 == info1);

    // Unassigned resolution
    daq::DomainInfo info7 = {daq::reader::parseEpoch("1999-01-01T00:00:00+00:00"), nullptr};
    ASSERT_THROW((void) (info7 == info1), daq::InvalidParameterException);
}

template <typename T>
void checkTypedDomainValue()
{
    auto epoch = daq::reader::parseEpoch("2026-01-01T00:00:00+00:00");
    daq::DomainInfo domain = {epoch, daq::Ratio(1, 1000000)};

    T tick;
    if constexpr (std::is_same_v<T, daq::RangeType64>)
    {
        tick = {12345, 12350};
    }
    else
    {
        tick = 12345;
    }
    std::unique_ptr<daq::DomainValue> value = std::make_unique<daq::DomainValueImpl<T>>(domain, tick);
    ASSERT_TRUE(value->getDomain() == domain);

    auto* castValue = dynamic_cast<daq::DomainValueImpl<T>*>(value.get());
    ASSERT_TRUE(castValue != nullptr);

    ASSERT_EQ(castValue->getValue(), tick);

    T greaterTick;
    if constexpr (std::is_same_v<T, daq::RangeType64>)
    {
        greaterTick = {12351, 12360};
    }
    else
    {
        greaterTick = 12351;
    }
    std::unique_ptr<daq::DomainValue> greaterValue = std::make_unique<daq::DomainValueImpl<T>>(domain, greaterTick);
    ASSERT_TRUE(*value < *greaterValue);
    ASSERT_FALSE(*greaterValue < *value);
    ASSERT_FALSE(*value < *value);
}

TEST_F(DomainValueTest, StoresDaqInt)
{
    checkTypedDomainValue<daq::Int>();
}

TEST_F(DomainValueTest, StoresDaqUInt)
{
    checkTypedDomainValue<daq::UInt>();
}

TEST_F(DomainValueTest, StoresDaqFloat)
{
    checkTypedDomainValue<daq::Float>();
}

TEST_F(DomainValueTest, StoresDaqRange)
{
    checkTypedDomainValue<daq::RangeType64>();
}

template <typename T>
void checkTypedDomainValueComplex()
{
    auto epoch = daq::reader::parseEpoch("2026-01-01T00:00:00+00:00");
    daq::DomainInfo domain = {epoch, daq::Ratio(1, 1000000)};

    T tick = {12.3, 15.4};
    std::unique_ptr<daq::DomainValue> value = std::make_unique<daq::DomainValueImpl<T>>(domain, tick);
    ASSERT_TRUE(value->getDomain() == domain);

    auto* castValue = dynamic_cast<daq::DomainValueImpl<T>*>(value.get());
    ASSERT_TRUE(castValue != nullptr);

    ASSERT_THROW(castValue->getValue(), daq::NotSupportedException);

    T greaterTick = {16.2, 25.3};

    std::unique_ptr<daq::DomainValue> greaterValue = std::make_unique<daq::DomainValueImpl<T>>(domain, greaterTick);
    ASSERT_THROW((void) (*value < *greaterValue), daq::NotSupportedException);
    ASSERT_THROW((void) (*greaterValue < *value), daq::NotSupportedException);
}

TEST_F(DomainValueTest, ThrowsForComplex)
{
    checkTypedDomainValueComplex<daq::ComplexFloat32>();
    checkTypedDomainValueComplex<daq::ComplexFloat64>();
}

TEST_F(DomainValueTest, Scaling)
{
    std::string commonEpochString = "2026-01-01T00:00:00+00:00";
    std::chrono::system_clock::time_point commonEpoch = daq::reader::parseEpoch(commonEpochString);
    daq::RatioPtr commonResolution = daq::Ratio(1, 1000000);

    daq::DomainInfo commonDomain = {commonEpoch, commonResolution};

    daq::RatioPtr resolution1 = daq::Ratio(1, 5000);
    daq::DomainInfo domain1 = {commonEpoch, resolution1};

    auto value1 = std::make_unique<daq::DomainValueImpl<daq::Int>>(domain1, 10000);
    ASSERT_EQ(value1->getValue(), 10000u);

    auto value1InCommonDomain = value1->toCommonDomain(commonDomain);
    auto* value1InCommonDomainP = dynamic_cast<daq::DomainValueImpl<daq::Int>*>(value1InCommonDomain.get());
    ASSERT_EQ(value1InCommonDomainP->getValue(), 2000000u);

    auto value1BackInRegularDomain = value1InCommonDomain->fromCommonDomain(domain1);
    auto* value1BackInRegularDomainP = dynamic_cast<daq::DomainValueImpl<daq::Int>*>(value1BackInRegularDomain.get());
    ASSERT_EQ(value1BackInRegularDomainP->getValue(), 10000u);
}

TEST_F(DomainValueTest, Offset)
{
    std::string commonEpochString = "1970-01-01T00:00:00+00:00";
    std::chrono::system_clock::time_point commonEpoch = daq::reader::parseEpoch(commonEpochString);
    daq::RatioPtr commonResolution = daq::Ratio(1, 1000);

    daq::DomainInfo commonDomain = {commonEpoch, commonResolution};

    std::string epochString1 = "1970-01-01T00:00:50+00:00";
    std::chrono::system_clock::time_point epoch1 = daq::reader::parseEpoch(epochString1);
    daq::RatioPtr resolution1 = daq::Ratio(1, 1000);
    daq::DomainInfo domain1 = {epoch1, resolution1};

    auto value1 = std::make_unique<daq::DomainValueImpl<daq::Int>>(domain1, 13000);
    ASSERT_EQ(value1->getValue(), 13000u);

    auto value1InCommonDomain = value1->toCommonDomain(commonDomain);
    auto* value1InCommonDomainP = dynamic_cast<daq::DomainValueImpl<daq::Int>*>(value1InCommonDomain.get());
    ASSERT_EQ(value1InCommonDomainP->getValue(), 63000u);
    ASSERT_EQ(value1InCommonDomainP->getDomain(), commonDomain);

    auto value1BackInRegularDomain = value1InCommonDomain->fromCommonDomain(domain1);
    auto* value1BackInRegularDomainP = dynamic_cast<daq::DomainValueImpl<daq::Int>*>(value1BackInRegularDomain.get());
    ASSERT_EQ(value1BackInRegularDomainP->getValue(), 13000u);
    ASSERT_EQ(value1BackInRegularDomainP->getDomain(), domain1);
}

TEST_F(DomainValueTest, SameDomainSameType)
{
    std::string commonEpochString = "1970-01-01T00:00:00+00:00";
    std::chrono::system_clock::time_point commonEpoch = daq::reader::parseEpoch(commonEpochString);
    daq::RatioPtr commonResolution = daq::Ratio(1, 1000);

    daq::DomainInfo commonDomain = {commonEpoch, commonResolution};

    std::string epochString1 = "1970-01-01T00:00:50+00:00";
    std::chrono::system_clock::time_point epoch1 = daq::reader::parseEpoch(epochString1);
    daq::RatioPtr resolution1 = daq::Ratio(1, 1000);
    daq::DomainInfo domain1 = {epoch1, resolution1};

    auto value1 = std::make_unique<daq::DomainValueImpl<daq::Int>>(domain1, 13000);
    daq::DomainValue* value1P = value1.get();
    auto value1InCommonDomain = value1->toCommonDomain(commonDomain);
    auto* value1InCommonDomainP = dynamic_cast<daq::DomainValueImpl<daq::Int>*>(value1InCommonDomain.get());

    ASSERT_THROW((void) (*value1P < *value1InCommonDomainP), daq::InvalidParameterException);

    auto value2 = std::make_unique<daq::DomainValueImpl<daq::UInt>>(domain1, 1213000);
    daq::DomainValue* value2P = value2.get();

    ASSERT_THROW((void) (*value1 < *value2), daq::InvalidParameterException);
}

TEST_F(DomainValueTest, RealisticTimeStamp)
{
    std::string commonEpochString = "1970-01-01T00:00:00+00:00";
    std::chrono::system_clock::time_point commonEpoch = daq::reader::parseEpoch(commonEpochString);
    daq::RatioPtr commonResolution = daq::Ratio(1, 1000000000);
    daq::DomainInfo commonDomain = {commonEpoch, commonResolution};

    std::string epochString1 = "2026-05-27T00:00:01+00:00";
    std::chrono::system_clock::time_point epoch1 = daq::reader::parseEpoch(epochString1);
    daq::RatioPtr resolution1 = daq::Ratio(1, 1000000);
    daq::DomainInfo domain1 = {epoch1, resolution1};

    auto value1 = std::make_unique<daq::DomainValueImpl<daq::Int>>(domain1, 50000001);
    auto value1InCommonDomain = value1->toCommonDomain(commonDomain);

    auto value1BackInRegularDomain = value1InCommonDomain->fromCommonDomain(domain1);
    auto* value1BackInRegularDomainP = dynamic_cast<daq::DomainValueImpl<daq::Int>*>(value1BackInRegularDomain.get());
    ASSERT_EQ(value1BackInRegularDomainP->getValue(), value1->getValue());
}

TEST_F(DomainValueTest, NonRepresentibleConversions)
{
    std::string commonEpochString = "1970-01-01T00:00:00+00:00";
    std::chrono::system_clock::time_point commonEpoch = daq::reader::parseEpoch(commonEpochString);
    daq::RatioPtr commonResolution = daq::Ratio(1, 1000000);
    daq::RatioPtr coarseResolution = daq::Ratio(1, 1000);

    daq::DomainInfo commonDomain = {commonEpoch, commonResolution};
    daq::DomainInfo coarseDomain = {commonEpoch, coarseResolution};

    std::unique_ptr<daq::DomainValue> valueInFine = std::make_unique<daq::DomainValueImpl<daq::Int>>(commonDomain, 1002);
    auto valueInCoarse = valueInFine->fromCommonDomain(coarseDomain);
    auto valueBackInFine = valueInCoarse->toCommonDomain(commonDomain);

    // TODO: For the domain value finding it would probably be required that during a lossful conversion fine1->coarse->fine2
    // where coars == fine2 (but in different domains) it also holds that fine2 >= fine1. Because then if we find index that
    // satisfies domain[index] >= coarse we found the correct one. Right now the situation is reversed, lossful conversion
    // rounds down.
    ASSERT_FALSE(*valueInFine < *valueBackInFine);
}