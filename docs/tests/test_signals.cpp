#include <gtest/gtest.h>
#include <chrono>
#include <opendaq/opendaq.h>
#include "docs_test_helpers.h"

using SignalsTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

// Corresponding document: Antora/modules/explanation/pages/signals.adoc
TEST_F(SignalsTest, DomainSignal)
{
    auto instance = docs_test_helpers::setupInstance();
    auto signal = instance.getSignalsRecursive()[0];
    ASSERT_TRUE(signal.getDomainSignal().assigned());
}

// Corresponding document: Antora/modules/explanation/pages/signals.adoc
TEST_F(SignalsTest, SignalDescriptor)
{
    auto instance = docs_test_helpers::setupInstance();
    auto signal = instance.getSignalsRecursive()[0];
    auto descriptor = signal.getDescriptor();
    ASSERT_TRUE(descriptor.getName().assigned());
    ASSERT_TRUE(descriptor.getMetadata().assigned());
}


// Corresponding document: Antora/modules/explanation/pages/signals.adoc
TEST_F(SignalsTest, DataDescriptor)
{
    auto instance = docs_test_helpers::setupInstance();
    auto signal = instance.getSignalsRecursive()[0];
    auto descriptor = signal.getDescriptor();

    ASSERT_NO_THROW(descriptor.getName());
    ASSERT_NO_THROW(descriptor.getDimensions());
    ASSERT_NO_THROW(descriptor.getOrigin());
    ASSERT_NO_THROW(descriptor.getPostScaling());
    ASSERT_NO_THROW(descriptor.getRule());
    ASSERT_NO_THROW(descriptor.getSampleType());
    ASSERT_NO_THROW(descriptor.getStructFields());
    ASSERT_NO_THROW(descriptor.getTickResolution());
    ASSERT_NO_THROW(descriptor.getUnit());
    ASSERT_NO_THROW(descriptor.getValueRange());
}

// Corresponding document: Antora/modules/explanation/pages/signals.adoc
TEST_F(SignalsTest, DataRule)
{
    auto dataRule1 = LinearDataRule(10, 10);
    auto dataRule2 = ConstantDataRule();
    auto dataRule3 = ExplicitDataRule();

    ASSERT_TRUE(dataRule1.assigned());
    ASSERT_TRUE(dataRule2.assigned());
    ASSERT_TRUE(dataRule3.assigned());
}

// Corresponding document: Antora/modules/explanation/pages/signals.adoc
TEST_F(SignalsTest, Dimensions)
{
    auto dimensions1 = Dimension(LinearDimensionRule(10, 10, 100));
    ASSERT_EQ(dimensions1.getSize(), 100u);
    ASSERT_EQ(dimensions1.getLabels()[0], 10);

    auto dimensions2 = Dimension(LogarithmicDimensionRule(10, 0, 2, 1000));
    ASSERT_EQ(dimensions2.getSize(), 1000u);
    ASSERT_EQ(dimensions2.getLabels()[0], 1);

    auto dimensions3 = Dimension(ListDimensionRule(List<INumber>(1, 5, 10, 20, 40)));
    ASSERT_EQ(dimensions3.getSize(), 5u);
    ASSERT_EQ(dimensions3.getLabels()[0], 1);
}

// Corresponding document: Antora/modules/explanation/pages/signals.adoc
TEST_F(SignalsTest, PostScaling)
{
    auto scaling = LinearScaling(2, 10);
    ASSERT_EQ(scaling.getParameters()["scale"], 2);
    ASSERT_EQ(scaling.getParameters()["offset"], 10);
}

// Corresponding document: Antora/modules/explanation/pages/signals.adoc
TEST_F(SignalsTest, OriginAndTickResolution)
{
    auto instance = docs_test_helpers::setupInstance();
    auto signal = instance.getSignalsRecursive()[0].getDomainSignal();
    auto desc = signal.getDescriptor();
    ASSERT_TRUE(desc.getTickResolution().assigned());
    ASSERT_TRUE(desc.getOrigin().assigned());
}

// Corresponding document: Antora/modules/explanation/pages/signals.adoc
TEST_F(SignalsTest, StructData)
{
    auto builder = DataDescriptorBuilder().setSampleType(SampleType::Undefined);

    auto struct1 = builder.build();
    auto struct2 = builder.build();

    auto dataDesc = builder.setSampleType(SampleType::Struct).setName("Struct").setStructFields(List<IDataDescriptor>(struct1, struct2)).build();
}


END_NAMESPACE_OPENDAQ
