#include "multi_reader2_harness.h"

namespace daq::mr2test
{

class MultiReader2Data : public MultiReader2Test
{
};

// S1.DATA.01
TEST_F(MultiReader2Data, ReadConvertsInt32InputsToFloat64)
{
    given().value(in1, Value(SampleType::Int32)).value(in2, Value(SampleType::Int32));
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged().size(in1, 8).size(in2, 8));
    write(in1, 0, 99);
    write(in2, 0, 99);
    expect(read(100), count(100).at(0).noChanges());
}

// S1.DATA.02
TEST_F(MultiReader2Data, ReadConvertsInt32VectorAndMatrixInputsToFloat64)
{
    given().value(in1, Value(SampleType::Int32).dims({3})).value(in2, Value(SampleType::Int32).dims({2, 3}));
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged().size(in1, 24).size(in2, 48));
    write(in1, 0, 99);
    write(in2, 0, 99);
    expect(read(100), count(100).at(0).noChanges());
}

// S1.DATA.03
TEST_F(MultiReader2Data, ReadsFloat64VectorAndMatrixInputs)
{
    given().value(in1, Value().dims({3})).value(in2, Value().dims({2, 3}));
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged().size(in1, 24).size(in2, 48));
    write(in1, 0, 99);
    write(in2, 0, 99);
    expect(read(100), count(100).at(0).noChanges());
}

// S1.DATA.04
TEST_F(MultiReader2Data, StructInputNotConvertibleToFloat64IsExcludedWithWarning)
{
    given().policy(Exclude).value(in2, Value::structure());
    expect(read(0),
           count(0)
               .level(Warning)
               .domainChanged()
               .resync()
               .descChanged(in1)
               .inError(in2, InputError::ValueDescriptorInvalid)
               .msg("in2: value type not convertible to Float64")
               .size(in1, 8)
               .size(in2, 0));
}

// S1.DATA.05
TEST_F(MultiReader2Data, StructArrayInputsNotConvertibleToFloat64GiveError)
{
    given().value(in1, Value::structure().dims({3})).value(in2, Value::structure().dims({2, 3}));
    expect(read(0),
           count(0)
               .level(Error)
               .inError(in1, InputError::ValueDescriptorInvalid)
               .inError(in2, InputError::ValueDescriptorInvalid)
               .msg("in1: value type not convertible to Float64; in2: value type not convertible to Float64")
               .size(in1, 0)
               .size(in2, 0));
}

// S1.DATA.06
TEST_F(MultiReader2Data, ReadAppliesPostScalingForFloat64)
{
    given().value(in1, Value::scaled(SampleType::Int16, 0.5, 1)).value(in2, Value::scaled(SampleType::Int16, 0.5, 1));
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged().size(in1, 8).size(in2, 8));
    write(in1, 0, 99);
    write(in2, 0, 99);
    expect(
        read(100),
        count(100).at(0).values(in1, [](int64_t t) { return t * 0.5 + 1; }).values(in2, [](int64_t t) { return t * 0.5 + 1; }).noChanges());
}

// S1.DATA.07
TEST_F(MultiReader2Data, ReadConvertsMixedFloat64AndInt32InputsToFloat64)
{
    given().value(in2, Value(SampleType::Int32));
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged().size(in1, 8).size(in2, 8));
    write(in1, 0, 99);
    write(in2, 0, 99);
    expect(read(100), count(100).at(0).noChanges());
}

// S1.DATA.08
TEST_F(MultiReader2Data, ReadConvertsMixedDimensionedInputsToFloat64)
{
    given().value(in1, Value().dims({2, 3})).value(in2, Value(SampleType::Int32).dims({3}));
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged().size(in1, 48).size(in2, 24));
    write(in1, 0, 99);
    write(in2, 0, 99);
    expect(read(100), count(100).at(0).noChanges());
}

// S1.DATA.09
TEST_F(MultiReader2Data, ReadsInt32InputsAsInt32)
{
    given().readType(SampleType::Int32).value(in1, Value(SampleType::Int32)).value(in2, Value(SampleType::Int32));
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged().size(in1, 4).size(in2, 4));
    write(in1, 0, 99);
    write(in2, 0, 99);
    expect(read(100), count(100).at(0).noChanges());
}

// S1.DATA.10
TEST_F(MultiReader2Data, ReadsInt32VectorAndMatrixInputsAsInt32)
{
    given().readType(SampleType::Int32).value(in1, Value(SampleType::Int32).dims({3})).value(in2, Value(SampleType::Int32).dims({2, 3}));
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged().size(in1, 12).size(in2, 24));
    write(in1, 0, 99);
    write(in2, 0, 99);
    expect(read(100), count(100).at(0).noChanges());
}

// S1.DATA.11
TEST_F(MultiReader2Data, ReadConvertsFloat64InputsToInt32)
{
    given().readType(SampleType::Int32);
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged().size(in1, 4).size(in2, 4));
    write(in1, 0, 99);
    write(in2, 0, 99);
    expect(read(100), count(100).at(0).noChanges());
}

// S1.DATA.12
TEST_F(MultiReader2Data, ReadConvertsFloat64VectorAndMatrixInputsToInt32)
{
    given().readType(SampleType::Int32).value(in1, Value().dims({3})).value(in2, Value().dims({2, 3}));
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged().size(in1, 12).size(in2, 24));
    write(in1, 0, 99);
    write(in2, 0, 99);
    expect(read(100), count(100).at(0).noChanges());
}

// S1.DATA.13
TEST_F(MultiReader2Data, StructInputNotConvertibleToInt32IsExcludedWithWarning)
{
    given().readType(SampleType::Int32).policy(Exclude).value(in1, Value(SampleType::Int32)).value(in2, Value::structure());
    expect(read(0),
           count(0)
               .level(Warning)
               .domainChanged()
               .resync()
               .descChanged(in1)
               .inError(in2, InputError::ValueDescriptorInvalid)
               .msg("in2: value type not convertible to Int32")
               .size(in1, 4)
               .size(in2, 0));
}

// S1.DATA.14
TEST_F(MultiReader2Data, StructArrayInputsNotConvertibleToInt32GiveError)
{
    given().readType(SampleType::Int32).value(in1, Value::structure().dims({3})).value(in2, Value::structure().dims({2, 3}));
    expect(read(0),
           count(0)
               .level(Error)
               .inError(in1, InputError::ValueDescriptorInvalid)
               .inError(in2, InputError::ValueDescriptorInvalid)
               .msg("in1: value type not convertible to Int32; in2: value type not convertible to Int32")
               .size(in1, 0)
               .size(in2, 0));
}

// S1.DATA.15
TEST_F(MultiReader2Data, ReadAppliesPostScalingAndRoundsToInt32)
{
    given()
        .readType(SampleType::Int32)
        .value(in1, Value::scaled(SampleType::Int16, 0.5, 1))
        .value(in2, Value::scaled(SampleType::Int16, 0.5, 1));
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged().size(in1, 4).size(in2, 4));
    write(in1, 0, 99);
    write(in2, 0, 99);
    expect(read(100),
           count(100)
               .at(0)
               .values(in1, [](int64_t t) { return std::round(t * 0.5 + 1); })
               .values(in2, [](int64_t t) { return std::round(t * 0.5 + 1); })
               .noChanges());
}

// S1.DATA.16
TEST_F(MultiReader2Data, ReadConvertsMixedFloat64AndInt32InputsToInt32)
{
    given().readType(SampleType::Int32).value(in2, Value(SampleType::Int32));
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged().size(in1, 4).size(in2, 4));
    write(in1, 0, 99);
    write(in2, 0, 99);
    expect(read(100), count(100).at(0).noChanges());
}

// S1.DATA.17
TEST_F(MultiReader2Data, ReadConvertsMixedDimensionedInputsToInt32)
{
    given().readType(SampleType::Int32).value(in1, Value().dims({2, 3})).value(in2, Value(SampleType::Int32).dims({3}));
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged().size(in1, 24).size(in2, 12));
    write(in1, 0, 99);
    write(in2, 0, 99);
    expect(read(100), count(100).at(0).noChanges());
}

// S1.DATA.18
TEST_F(MultiReader2Data, UndefinedReadTypeReadsInt32InputsInOwnType)
{
    given().readType(SampleType::Undefined).value(in1, Value(SampleType::Int32)).value(in2, Value(SampleType::Int32));
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged().size(in1, 4).size(in2, 4));
    write(in1, 0, 99);
    write(in2, 0, 99);
    expect(read(100), count(100).at(0).noChanges());
}

// S1.DATA.19
TEST_F(MultiReader2Data, UndefinedReadTypeReadsInt32VectorAndMatrixInOwnType)
{
    given()
        .readType(SampleType::Undefined)
        .value(in1, Value(SampleType::Int32).dims({3}))
        .value(in2, Value(SampleType::Int32).dims({2, 3}));
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged().size(in1, 12).size(in2, 24));
    write(in1, 0, 99);
    write(in2, 0, 99);
    expect(read(100), count(100).at(0).noChanges());
}

// S1.DATA.20
TEST_F(MultiReader2Data, UndefinedReadTypeReadsFloat64InputsInOwnType)
{
    given().readType(SampleType::Undefined);
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged().size(in1, 8).size(in2, 8));
    write(in1, 0, 99);
    write(in2, 0, 99);
    expect(read(100), count(100).at(0).noChanges());
}

// S1.DATA.21
TEST_F(MultiReader2Data, UndefinedReadTypeReadsFloat64VectorAndMatrixInOwnType)
{
    given().readType(SampleType::Undefined).value(in1, Value().dims({3})).value(in2, Value().dims({2, 3}));
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged().size(in1, 24).size(in2, 48));
    write(in1, 0, 99);
    write(in2, 0, 99);
    expect(read(100), count(100).at(0).noChanges());
}

// S1.DATA.22
TEST_F(MultiReader2Data, UndefinedReadTypeReadsStructInputsInOwnType)
{
    given().readType(SampleType::Undefined).value(in1, Value::structure()).value(in2, Value::structure());
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged().size(in1, 12).size(in2, 12));
    write(in1, 0, 99);
    write(in2, 0, 99);
    expect(read(100), count(100).at(0).noChanges());
}

// S1.DATA.23
TEST_F(MultiReader2Data, UndefinedReadTypeReadsStructVectorAndMatrixInOwnType)
{
    given().readType(SampleType::Undefined).value(in1, Value::structure().dims({3})).value(in2, Value::structure().dims({2, 3}));
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged().size(in1, 36).size(in2, 72));
    write(in1, 0, 99);
    write(in2, 0, 99);
    expect(read(100), count(100).at(0).noChanges());
}

// S1.DATA.24
TEST_F(MultiReader2Data, UndefinedReadTypeReadsPostScaledInputsAsScaledFloat64)
{
    given()
        .readType(SampleType::Undefined)
        .value(in1, Value::scaled(SampleType::Int16, 0.5, 1))
        .value(in2, Value::scaled(SampleType::Int16, 0.5, 1));
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged().size(in1, 8).size(in2, 8));
    write(in1, 0, 99);
    write(in2, 0, 99);
    expect(
        read(100),
        count(100).at(0).values(in1, [](int64_t t) { return t * 0.5 + 1; }).values(in2, [](int64_t t) { return t * 0.5 + 1; }).noChanges());
}

// S1.DATA.25
TEST_F(MultiReader2Data, UndefinedReadTypeReadsMixedInputsEachInOwnType)
{
    given().readType(SampleType::Undefined).value(in2, Value(SampleType::Int32));
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged().size(in1, 8).size(in2, 4));
    write(in1, 0, 99);
    write(in2, 0, 99);
    expect(read(100), count(100).at(0).noChanges());
}

// S1.DATA.26
TEST_F(MultiReader2Data, UndefinedReadTypeReadsMixedDimensionedInputsEachInOwnType)
{
    given().readType(SampleType::Undefined).value(in1, Value().dims({2, 3})).value(in2, Value(SampleType::Int32).dims({3}));
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged().size(in1, 48).size(in2, 12));
    write(in1, 0, 99);
    write(in2, 0, 99);
    expect(read(100), count(100).at(0).noChanges());
}

// S1.DATA.27
TEST_F(MultiReader2Data, DomainWithoutOriginIsDomainDescriptorInvalid)
{
    given().policy(Exclude).domain(in2, Domain().noOrigin());
    expect(read(0),
           count(0)
               .level(Warning)
               .domainChanged()
               .resync()
               .descChanged(in1)
               .inError(in2, InputError::DomainDescriptorInvalid)
               .msg("in2: domain descriptor invalid, no origin")
               .size(in1, 8)
               .size(in2, 0));
}

// S1.DATA.28
TEST_F(MultiReader2Data, MidStreamOriginNotOnFullSecondIsDomainDescriptorInvalid)
{
    given().policy(Exclude);
    setupBase();
    announce(in2, Domain().originMs(500));
    expect(read(100),
           count(100)
               .at(0)
               .level(Warning)
               .inError(in2, InputError::DomainDescriptorInvalid)
               .msg("in2: domain descriptor invalid, origin not an ISO 8601 time on a full second")
               .size(in2, 0));
}

// S1.DATA.30
TEST_F(MultiReader2Data, FloatDomainSampleTypeIsDomainDescriptorInvalid)
{
    given().policy(Exclude).domain(in2, Domain().type(SampleType::Float64));
    expect(read(0),
           count(0)
               .level(Warning)
               .domainChanged()
               .resync()
               .descChanged(in1)
               .inError(in2, InputError::DomainDescriptorInvalid)
               .msg("in2: domain descriptor invalid, ticks not of an integer type")
               .size(in1, 8)
               .size(in2, 0));
}

// S1.DATA.31
TEST_F(MultiReader2Data, MidStreamDimensionedDomainTypeIsDomainDescriptorInvalid)
{
    given().policy(Exclude);
    setupBase();
    announce(in2, Domain().type(SampleType::Int64, {2}));
    expect(read(100),
           count(100)
               .at(0)
               .level(Warning)
               .inError(in2, InputError::DomainDescriptorInvalid)
               .msg("in2: domain descriptor invalid, ticks not scalar"));
}

// S1.DATA.32
TEST_F(MultiReader2Data, MidStreamMainDomainUnitNotSecondsMovesAutomaticMain)
{
    given().policy(Exclude);
    setupBase();
    announce(in1, Domain().unitMs());
    expect(read(100),
           count(100)
               .at(0)
               .level(Warning)
               .main(in2)
               .domainChanged()
               .resync()
               .descChanged(in2)
               .inError(in1, InputError::DomainDescriptorInvalid)
               .msg("in1: domain descriptor invalid, unit not seconds"));
}

// S1.DATA.33
TEST_F(MultiReader2Data, ExplicitDomainRuleIsDomainDescriptorInvalid)
{
    given().policy(Exclude).domain(in2, Domain().explicitRule());
    expect(read(0),
           count(0)
               .level(Warning)
               .domainChanged()
               .resync()
               .descChanged(in1)
               .inError(in2, InputError::DomainDescriptorInvalid)
               .msg("in2: domain descriptor invalid, rule not linear")
               .size(in1, 8)
               .size(in2, 0));
}

// S1.DATA.34
TEST_F(MultiReader2Data, MidStreamDomainDeltaZeroUnderInvalidateGivesError)
{
    setupBase();
    announce(in2, Domain().delta(0));
    expect(read(100),
           count(100)
               .at(0)
               .level(Error)
               .inError(in2, InputError::DomainDescriptorInvalid)
               .msg("in2: domain descriptor invalid, rule delta not an integer above 0")
               .size(in1, 0)
               .size(in2, 0));
}

// S1.DATA.35
TEST_F(MultiReader2Data, MidStreamNonIntegerDomainStartIsDomainDescriptorInvalid)
{
    given().policy(Exclude);
    setupBase();
    announce(in2, Domain().start(1.5));
    expect(read(100),
           count(100)
               .at(0)
               .level(Warning)
               .inError(in2, InputError::DomainDescriptorInvalid)
               .msg("in2: domain descriptor invalid, rule start not an integer"));
}

// S1.DATA.36
TEST_F(MultiReader2Data, ReadingReportsPacketOffsetsAsMainTicks)
{
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged());
    write(in1, 0, 99);
    write(in2, 0, 99);
    expect(skip(10), count(10).at(0).noChanges());
    expect(read(40), count(40).at(10).noChanges());
    expect(readWithDomain(50), count(50).at(50).noChanges());
}

// S1.DATA.37
TEST_F(MultiReader2Data, ReadingWithNonSimplifiedMainResolutionReportsMainTicks)
{
    given().domain(in1, Domain().resolution(2, 2000));
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged());
    write(in1, 0, 99);
    write(in2, 0, 99);
    expect(skip(10), count(10).at(0).noChanges());
    expect(read(40), count(40).at(10).noChanges());
    expect(readWithDomain(50), count(50).at(50).noChanges());
}

// S1.DATA.38
TEST_F(MultiReader2Data, ReadingWithFinerResolutionAndDeltaTwoReportsMainTicks)
{
    given().domain(in1, Domain().resolution(1, 2000).delta(2)).domain(in2, Domain().resolution(1, 2000).delta(2));
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged());
    write(in1, 0, 198);
    write(in2, 0, 198);
    expect(skip(10), count(10).at(0).noChanges());
    expect(read(40), count(40).at(20).noChanges());
    expect(readWithDomain(50), count(50).at(100).noChanges());
}

// S1.DATA.39
TEST_F(MultiReader2Data, ReadingWithNonZeroMainRuleStartReportsMainTicks)
{
    given().domain(in1, Domain().start(3));
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged());
    write(in1, 100, 199);
    write(in2, 100, 199);
    expect(skip(10), count(10).at(100).noChanges());
    expect(read(40), count(40).at(110).noChanges());
    expect(readWithDomain(50), count(50).at(150).noChanges());
}

// S1.DATA.40
TEST_F(MultiReader2Data, ReadingWithInputOriginOffsetByWholeSecondsReportsMainTicks)
{
    given().domain(in2, Domain().originSeconds(-1));
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged());
    write(in1, 0, 99);
    write(in2, 0, 99);
    expect(skip(10), count(10).at(0).noChanges());
    expect(read(40), count(40).at(10).noChanges());
    expect(readWithDomain(50), count(50).at(50).noChanges());
}

// S1.DATA.41
TEST_F(MultiReader2Data, InputWithDifferentSampleRateIsDomainDescriptorInvalid)
{
    given().policy(Exclude).domain(in2, Domain().resolution(1, 500));
    expect(read(0),
           count(0)
               .level(Warning)
               .domainChanged()
               .resync()
               .descChanged(in1)
               .inError(in2, InputError::DomainDescriptorInvalid)
               .msg("in2: domain descriptor invalid, sampling rate differs from the main input's"));
}

// S1.DATA.42
TEST_F(MultiReader2Data, NonSimplifiedResolutionAtDifferentRateIsDomainDescriptorInvalid)
{
    given().policy(Exclude).domain(in2, Domain().resolution(4, 2000));
    expect(read(0),
           count(0)
               .level(Warning)
               .domainChanged()
               .resync()
               .descChanged(in1)
               .inError(in2, InputError::DomainDescriptorInvalid)
               .msg("in2: domain descriptor invalid, sampling rate differs from the main input's"));
}

// S1.DATA.43
TEST_F(MultiReader2Data, FinerResolutionWithDeltaGivingDifferentRateIsDomainDescriptorInvalid)
{
    given().policy(Exclude).domain(in2, Domain().resolution(1, 2000).delta(4));
    expect(read(0),
           count(0)
               .level(Warning)
               .domainChanged()
               .resync()
               .descChanged(in1)
               .inError(in2, InputError::DomainDescriptorInvalid)
               .msg("in2: domain descriptor invalid, sampling rate differs from the main input's"));
}

// S1.DATA.44
TEST_F(MultiReader2Data, DifferentRateWithNonZeroRuleStartIsDomainDescriptorInvalid)
{
    given().policy(Exclude).domain(in2, Domain().resolution(1, 500).start(3));
    expect(read(0),
           count(0)
               .level(Warning)
               .domainChanged()
               .resync()
               .descChanged(in1)
               .inError(in2, InputError::DomainDescriptorInvalid)
               .msg("in2: domain descriptor invalid, sampling rate differs from the main input's"));
}

// S1.DATA.45
TEST_F(MultiReader2Data, DifferentRateWithOriginOffsetIsDomainDescriptorInvalid)
{
    given().policy(Exclude).domain(in2, Domain().resolution(1, 500).originSeconds(-1));
    expect(read(0),
           count(0)
               .level(Warning)
               .domainChanged()
               .resync()
               .descChanged(in1)
               .inError(in2, InputError::DomainDescriptorInvalid)
               .msg("in2: domain descriptor invalid, sampling rate differs from the main input's"));
}

// S1.DATA.46
TEST_F(MultiReader2Data, FinerMainTicksAlignWithSamePeriodNonMain)
{
    given().domain(in1, Domain().resolution(1, 2000).delta(2));
    setup(
        [&]
        {
            write(in1, 0, 198);
            write(in2, 0, 198);
            read(0, nullData);
        });
    expect(read(100), count(100).at(0).noChanges());
}

// S1.DATA.47
TEST_F(MultiReader2Data, FinerMainTicksAlignWithNonSimplifiedNonMainResolution)
{
    given().domain(in1, Domain().resolution(1, 2000).delta(2)).domain(in2, Domain().resolution(2, 2000));
    setup(
        [&]
        {
            write(in1, 0, 198);
            write(in2, 0, 198);
            read(0, nullData);
        });
    expect(read(100), count(100).at(0).noChanges());
}

// S1.DATA.48
TEST_F(MultiReader2Data, FinerMainTicksAlignWithNonMainNonZeroRuleStart)
{
    given().domain(in1, Domain().resolution(1, 2000).delta(2)).domain(in2, Domain().start(3));
    setup(
        [&]
        {
            write(in1, 200, 398);
            write(in2, 200, 398);
            read(0, nullData);
        });
    expect(read(100), count(100).at(200).noChanges());
}

// S1.DATA.49
TEST_F(MultiReader2Data, FinerMainTicksAlignWithNonMainOriginOffset)
{
    given().domain(in1, Domain().resolution(1, 2000).delta(2)).domain(in2, Domain().originSeconds(-1));
    setup(
        [&]
        {
            write(in1, 0, 198);
            write(in2, 0, 198);
            read(0, nullData);
        });
    expect(read(100), count(100).at(0).noChanges());
}

// S1.DATA.50
TEST_F(MultiReader2Data, NonMainOnBaseDomainOffFinerMainGridFailsSync)
{
    given().policy(Exclude).domain(in1, Domain().resolution(1, 2000).delta(2).start(1));
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged());
    write(in1, 1, 199);
    write(in2, 0, 198);
    expect(read(100), count(0).level(Warning).inError(in2, InputError::SyncFailed).msg("in2: not synchronized, off the main input's grid"));
}

// S1.DATA.51
TEST_F(MultiReader2Data, NonMainWithNonSimplifiedResolutionOffMainGridFailsSync)
{
    given().policy(Exclude).domain(in1, Domain().resolution(1, 2000).delta(2).start(1)).domain(in2, Domain().resolution(2, 2000));
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged());
    write(in1, 1, 199);
    write(in2, 0, 198);
    expect(read(100), count(0).level(Warning).inError(in2, InputError::SyncFailed).msg("in2: not synchronized, off the main input's grid"));
}

// S1.DATA.52
TEST_F(MultiReader2Data, NonMainWithFinerResolutionOffMainGridFailsSync)
{
    given().policy(Exclude).domain(in1, Domain().resolution(1, 2000).delta(2)).domain(in2, Domain().resolution(1, 2000).delta(2));
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged());
    write(in1, 2, 200);
    write(in2, 1, 199);
    expect(read(100), count(0).level(Warning).inError(in2, InputError::SyncFailed).msg("in2: not synchronized, off the main input's grid"));
}

// S1.DATA.53
TEST_F(MultiReader2Data, NonMainWithRuleStartOffMainGridFailsSync)
{
    given().policy(Exclude).domain(in1, Domain().resolution(1, 2000).delta(2)).domain(in2, Domain().resolution(1, 2000).delta(2).start(1));
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged());
    write(in1, 2, 200);
    write(in2, 1, 199);
    expect(read(100), count(0).level(Warning).inError(in2, InputError::SyncFailed).msg("in2: not synchronized, off the main input's grid"));
}

// S1.DATA.54
TEST_F(MultiReader2Data, NonMainWithOriginOffsetOffMainGridFailsSync)
{
    given().policy(Exclude).domain(in1, Domain().resolution(1, 2000).delta(2).start(1)).domain(in2, Domain().originSeconds(-1));
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged());
    write(in1, 1, 199);
    write(in2, 0, 198);
    expect(read(100), count(0).level(Warning).inError(in2, InputError::SyncFailed).msg("in2: not synchronized, off the main input's grid"));
}

// S1.DATA.55
TEST_F(MultiReader2Data, ReadingUnixEpochNanosecondTicksKeepsExactMainTicks)
{
    given()
        .valuesAsIndex()
        .domain(in1, Domain().resolution(1, 1000000000).delta(1000000).originEpoch())
        .domain(in2, Domain().resolution(1, 1000000000).delta(1000000).originEpoch());
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged());
    write(in1, 1767225600000000000, 1767225600099000000);
    write(in2, 1767225600000000000, 1767225600099000000);
    expect(skip(10), count(10).at(1767225600000000000).noChanges());
    expect(read(40), count(40).at(1767225600010000000).noChanges());
    expect(readWithDomain(50), count(50).at(1767225600050000000).noChanges());
}

// S1.DATA.56
TEST_F(MultiReader2Data, ReadingEpochTicksAlignsMicrosecondMainWithNanosecondInput)
{
    given()
        .valuesAsIndex()
        .domain(in1, Domain().resolution(1, 1000000).delta(1000).originEpoch())
        .domain(in2, Domain().resolution(1, 1000000000).delta(1000000).originEpoch());
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged());
    write(in1, 1767225600000000, 1767225600099000);
    write(in2, 1767225600000000, 1767225600099000);
    expect(skip(10), count(10).at(1767225600000000).noChanges());
    expect(read(40), count(40).at(1767225600010000).noChanges());
    expect(readWithDomain(50), count(50).at(1767225600050000).noChanges());
}

// S1.DATA.57
TEST_F(MultiReader2Data, ReadingEpochTicksAlignsNanosecondMainWithMicrosecondInput)
{
    given()
        .valuesAsIndex()
        .domain(in1, Domain().resolution(1, 1000000000).delta(1000000).originEpoch())
        .domain(in2, Domain().resolution(1, 1000000).delta(1000).originEpoch());
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged());
    write(in1, 1767225600000000000, 1767225600099000000);
    write(in2, 1767225600000000000, 1767225600099000000);
    expect(skip(10), count(10).at(1767225600000000000).noChanges());
    expect(read(40), count(40).at(1767225600010000000).noChanges());
    expect(readWithDomain(50), count(50).at(1767225600050000000).noChanges());
}

// S1.DATA.58
TEST_F(MultiReader2Data, ReadingEpochTicksAlignsInputWithLargeNonSimplifiedResolution)
{
    given()
        .valuesAsIndex()
        .domain(in1, Domain().resolution(1, 1000000000).delta(1000000).originEpoch())
        .domain(in2, Domain().resolution(1000, 1000000000000).delta(1000000).originEpoch());
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged());
    write(in1, 1767225600000000000, 1767225600099000000);
    write(in2, 1767225600000000000, 1767225600099000000);
    expect(skip(10), count(10).at(1767225600000000000).noChanges());
    expect(read(40), count(40).at(1767225600010000000).noChanges());
    expect(readWithDomain(50), count(50).at(1767225600050000000).noChanges());
}

// S1.DATA.59
TEST_F(MultiReader2Data, OriginDifferenceOverflowingInt64TicksIsDomainDescriptorInvalid)
{
    given()
        .policy(Exclude)
        .domain(in1, Domain().resolution(1, 1000000000000).delta(1000000000))
        .domain(in2, Domain().resolution(1, 1000000000).delta(1000000).originEpoch());
    expect(read(0),
           count(0)
               .level(Warning)
               .domainChanged()
               .resync()
               .descChanged(in1)
               .inError(in2, InputError::DomainDescriptorInvalid)
               .msg("in2: domain descriptor invalid, origin not a whole number of main ticks from the main input's"));
    write(in1, 0, 99000000000);
    expect(read(100), count(100).at(0).noChanges());
}

// S1.DATA.60
TEST_F(MultiReader2Data, ConstructingWithInputLackingOriginGivesErrorAndReadsNothing)
{
    given().signal(in2, sig::domainFault(2));
    expect(read(0),
           count(0).level(Error).inError(in2, InputError::DomainDescriptorInvalid).msg("in2: domain descriptor invalid, no origin"));
    write(in1, 0, 99);
    write(in2, 0, 99);
    expect(read(100), count(0).noChanges());
}

// S1.DATA.61
TEST_F(MultiReader2Data, ConstructingWithMainOriginOffFullSecondMovesAutomaticMain)
{
    given().policy(Exclude).signal(in1, sig::domainFault(3));
    expect(read(0),
           count(0)
               .level(Warning)
               .main(in2)
               .domainChanged()
               .resync()
               .descChanged(in2)
               .inError(in1, InputError::DomainDescriptorInvalid)
               .msg("in1: domain descriptor invalid, origin not an ISO 8601 time on a full second"));
    write(in2, 0, 99);
    expect(read(100), count(100).at(0).noChanges());
}

// S1.DATA.62
TEST_F(MultiReader2Data, PinnedMainWithFloatDomainTypeGivesErrorUnderExclude)
{
    given().policy(Exclude).main(in2).signal(in2, sig::domainFault(5));
    expect(read(0),
           count(0)
               .level(Error)
               .main(in2)
               .inError(in2, InputError::DomainDescriptorInvalid)
               .msg("in2: domain descriptor invalid, ticks not of an integer type"));
}

// S1.DATA.63
TEST_F(MultiReader2Data, ConfigureAddingInputWithDimensionedDomainGivesError)
{
    given().signal(in3, sig::domainFault(6));
    setupBase();
    expect(configure(same().add(in3)),
           ok().level(Error)
               .mainNull()
               .inError(in3, InputError::DomainDescriptorInvalid)
               .msg("in3: domain descriptor invalid, ticks not scalar"));
    write(in1, 100, 199);
    write(in2, 100, 199);
    expect(read(100), count(0).noChanges());
}

// S1.DATA.64
TEST_F(MultiReader2Data, ConfigureAddingInputWithMsDomainUnitExcludesItWithWarning)
{
    given().policy(Exclude).signal(in3, sig::domainFault(7));
    setupBase();
    expect(configure(same().add(in3)),
           ok().level(Warning)
               .domainChanged()
               .resync()
               .allDescChanged()
               .inError(in3, InputError::DomainDescriptorInvalid)
               .msg("in3: domain descriptor invalid, unit not seconds"));
    write(in1, 100, 199);
    write(in2, 100, 199);
    expect(read(100), count(100).at(100).noChanges());
}

// S1.DATA.65
TEST_F(MultiReader2Data, ConfigureSwitchingToExcludeTurnsExplicitRuleErrorIntoWarning)
{
    given().signal(in2, sig::domainFault(8));
    setupBase();
    expect(configure(same().policy(Exclude)), ok().level(Warning).domainChanged().resync().allDescChanged());
    write(in1, 100, 199);
    expect(read(100), count(100).at(100).noChanges());
}

// S1.DATA.66
TEST_F(MultiReader2Data, ConstructingWithAllInputsDeltaZeroGivesErrorUnderExclude)
{
    given().policy(Exclude).signal(in1, sig::domainFault(9)).signal(in2, sig::domainFault(9));
    expect(read(0),
           count(0)
               .level(Error)
               .mainNull()
               .inError(in1, InputError::DomainDescriptorInvalid)
               .inError(in2, InputError::DomainDescriptorInvalid)
               .msg("in1: domain descriptor invalid, rule delta not an integer above 0; in2: domain descriptor invalid, rule delta not an "
                    "integer above 0"));
}

// S1.DATA.67
TEST_F(MultiReader2Data, ConfigureSwitchingToInvalidateTurnsNonIntegerStartIntoError)
{
    given().policy(Exclude).signal(in2, sig::domainFault(10));
    setupBase();
    expect(configure(same().policy(Invalidate)), ok().level(Error));
    write(in1, 100, 199);
    expect(read(100), count(0).noChanges());
}

// S1.DATA.68
TEST_F(MultiReader2Data, ReadClampsValuesAboveInt32MaxToInt32Max)
{
    given().readType(SampleType::Int32).signal(in2, sig::int64());
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged().size(in1, 4).size(in2, 4));
    write(in1, 2147483600, 2147483699);
    write(in2, 2147483600, 2147483699);
    expect(read(100),
           count(100)
               .at(2147483600)
               .values(in1, [](int64_t t) { return static_cast<double>(std::min<int64_t>(t, 2147483647)); })
               .values(in2, [](int64_t t) { return static_cast<double>(std::min<int64_t>(t, 2147483647)); })
               .noChanges());
}

// S1.DATA.69
TEST_F(MultiReader2Data, ReadConvertsNanValuesToZeroForInt32)
{
    given().readType(SampleType::Int32).valuesNaN();
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged().size(in1, 4).size(in2, 4));
    write(in1, 0, 99);
    write(in2, 0, 99);
    expect(read(100), count(100).at(0).values(in1, [](int64_t) { return 0.0; }).values(in2, [](int64_t) { return 0.0; }).noChanges());
}

}
