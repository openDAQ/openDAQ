#include <opendaq/scaling_factory.h>
#include <opendaq/signal_exceptions.h>
#include <gtest/gtest.h>

using ScalingTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

TEST_F(ScalingTest, ScalingSetGet)
{
    const auto rule = ScalingBuilder()
                      .addParameter("test", 10)
                      .addParameter("test1", 10.5)
                      .build();

    ASSERT_EQ(rule.getParameters().get("test"), 10);
    ASSERT_EQ(rule.getParameters().get("test1"), 10.5);
}

TEST_F(ScalingTest, LinearScalingSetGet)
{
    const auto rule = LinearScaling(10, 20);

    ASSERT_EQ(rule.getType(), ScalingType::Linear);
    ASSERT_EQ(rule.getParameters().get("scale"), 10);
    ASSERT_EQ(rule.getParameters().get("offset"), 20);
}

TEST_F(ScalingTest, LinearScalingCopyFactory)
{
    const auto rule = LinearScaling(100, 50);
    const auto ruleCopy = ScalingBuilderCopy(rule).build();

    ASSERT_EQ(ruleCopy.getParameters().get("scale"), 100);
    ASSERT_EQ(ruleCopy.getParameters().get("offset"), 50);
}

TEST_F(ScalingTest, LinearScalingInvalidParameters)
{
    auto ruleBuilder = ScalingBuilder().setScalingType(ScalingType::Linear);
    ASSERT_THROW(ruleBuilder.build(), InvalidParametersException);

    auto params = Dict<IString, IBaseObject>();
    ruleBuilder.setParameters(params);
    ASSERT_THROW(ruleBuilder.build(), InvalidParametersException);

    params.set("scale", "wrong");
    ASSERT_THROW(ruleBuilder.build(), InvalidParametersException);

    params.set("scale", 10);
    params.set("offset", 10);
    params.set("extra", 10);
    ASSERT_THROW(ruleBuilder.build(), InvalidParametersException);

    params.deleteItem("extra");
    ASSERT_NO_THROW(ruleBuilder.build());
}

TEST_F(ScalingTest, InvalidInputDataType)
{
    ASSERT_THROW(LinearScaling(10, 10, SampleType::ComplexFloat32), InvalidSampleTypeException);
    ASSERT_THROW(LinearScaling(10, 10, SampleType::Binary), InvalidSampleTypeException);
    ASSERT_THROW(LinearScaling(10, 10, SampleType::Struct), InvalidSampleTypeException);
}

TEST_F(ScalingTest, LinearScalingSerializeDeserialize)
{
    const auto scaling = LinearScaling(10, 20, SampleType::Int16, ScaledSampleType::Float32);
    auto serializer = JsonSerializer(False);
    scaling.serialize(serializer);

    auto serialized = serializer.getOutput();

    auto deserializer = JsonDeserializer();
    auto scaling1 = deserializer.deserialize(serialized.toStdString()).asPtr<IScaling>();

    ASSERT_EQ(scaling1, scaling);
}

TEST_F(ScalingTest, StructType)
{
    const auto structType = ScalingStructType();
    const daq::StructPtr structPtr = LinearScaling(10, 10);
    ASSERT_EQ(structType, structPtr.getStructType());
}

TEST_F(ScalingTest, StructFields)
{
    const daq::StructPtr structPtr = LinearScaling(10, 10);
    ASSERT_EQ(structPtr.get("RuleType"), static_cast<Int>(ScalingType::Linear));

    const auto params = Dict<IString, IBaseObject>({
            {"scale", 10},
            {"offset", 10}
        });
    ASSERT_EQ(structPtr.get("Parameters"), params);
    ASSERT_EQ(structPtr.get("InputDataType"), static_cast<Int>(SampleType::Float64));
    ASSERT_EQ(structPtr.get("OutputDataType"), static_cast<Int>(SampleType::Float64));
}

TEST_F(ScalingTest, StructNames)
{
    const auto structType = daq::ScalingStructType();
    const daq::StructPtr structPtr = LinearScaling(10, 10);
    ASSERT_EQ(structType.getFieldNames(), structPtr.getFieldNames());
}

TEST_F(ScalingTest, ScalingBuilderSetGet)
{
    const auto params = Dict<IString, IBaseObject>({
            {"scale", 10},
            {"offset", 10}
        });
    const auto scalingBuilder = ScalingBuilder()
                                .setInputDataType(SampleType::Int16)
                                .setOutputDataType(ScaledSampleType::Float32)
                                .setScalingType(ScalingType::Linear)
                                .setParameters(params);
    
    ASSERT_EQ(scalingBuilder.getInputDataType(), SampleType::Int16);
    ASSERT_EQ(scalingBuilder.getOutputDataType(), ScaledSampleType::Float32);
    ASSERT_EQ(scalingBuilder.getScalingType(), ScalingType::Linear);
    ASSERT_EQ(scalingBuilder.getParameters(), params);
}

TEST_F(ScalingTest, ScalingCreateFactory)
{
    const auto params = Dict<IString, IBaseObject>({
            {"scale", 10},
            {"offset", 10}
        });
    const auto scalingBuilder = ScalingBuilder()
                                .setInputDataType(SampleType::Int16)
                                .setOutputDataType(ScaledSampleType::Float32)
                                .setScalingType(ScalingType::Linear)
                                .setParameters(params);
    const auto scaling = ScalingFromBuilder(scalingBuilder);

    ASSERT_EQ(scaling.getInputSampleType(), SampleType::Int16);
    ASSERT_EQ(scaling.getOutputSampleType(), ScaledSampleType::Float32);
    ASSERT_EQ(scaling.getType(), ScalingType::Linear);
    ASSERT_EQ(scaling.getParameters(), params);
}

END_NAMESPACE_OPENDAQ
