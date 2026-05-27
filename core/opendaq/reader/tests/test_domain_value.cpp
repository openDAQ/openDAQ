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
    daq::DomainInfo  info3 = {daq::reader::parseEpoch("1999-01-01T00:00:00+00:00"), daq::Ratio(1, 1000)};
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
    ASSERT_THROW((void)(info7 == info1), daq::InvalidParameterException);
}

template<typename T>
void checkTypedDomainValue(){
    auto epoch = daq::reader::parseEpoch("2026-01-01T00:00:00+00:00");
    daq::DomainInfo domain = {epoch, daq::Ratio(1, 1000000)};

    T tick;
    if constexpr (std::is_same_v<T, daq::RangeType64>){
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
    if constexpr (std::is_same_v<T, daq::RangeType64>){
        greaterTick = {12351, 12360};
    }
    else
    {
        greaterTick = 12351;
    }
    std::unique_ptr<daq::DomainValue> greaterValue = std::make_unique<daq::DomainValueImpl<T>>(domain, greaterTick);
    ASSERT_TRUE(*value < *greaterValue);
    ASSERT_FALSE(*greaterValue < *value);
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

template<typename T>
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
    ASSERT_THROW((void)(*value < *greaterValue), daq::NotSupportedException);
    ASSERT_THROW((void)(*greaterValue < *value), daq::NotSupportedException);
}

TEST_F(DomainValueTest, ThrowsForComplex)
{
    checkTypedDomainValueComplex<daq::ComplexFloat32>();
    checkTypedDomainValueComplex<daq::ComplexFloat64>();
}

TEST_F(DomainValueTest, Basic)
{
    std::string commonEpochString = "1970-01-01T00:00:00+00:00";
    std::chrono::system_clock::time_point commonEpoch = daq::reader::parseEpoch(commonEpochString);
    daq::RatioPtr commonResolution = daq::Ratio(1, 1000);
    
    daq::DomainInfo commonDomain = {commonEpoch, commonResolution};

    std::string epochString1 = "1970-01-01T00:00:50+00:00";
    std::chrono::system_clock::time_point epoch1 = daq::reader::parseEpoch(epochString1);
    daq::RatioPtr resolution1 = daq::Ratio(1, 500);

    daq::DomainInfo domain1 = {epoch1, resolution1};

    auto value1 = std::make_unique<daq::DomainValueImpl<int64_t>>(domain1, 2000);
    ASSERT_EQ(value1->getValue(), 2000u);

    auto value1InCommonDomain = value1->toCommonDomain(commonDomain);
    auto* value1InCommonDomainP = dynamic_cast<daq::DomainValueImpl<int64_t>*>(value1InCommonDomain.get());
    ASSERT_EQ(value1InCommonDomainP->getValue(), 54000u);
    
    auto value1BackInRegularDomain = value1InCommonDomain->fromCommonDomain(domain1);
    auto* value1BackInRegularDomainP = dynamic_cast<daq::DomainValueImpl<int64_t>*>(value1BackInRegularDomain.get());
    ASSERT_EQ(value1BackInRegularDomainP->getValue(), 2000u);
}
