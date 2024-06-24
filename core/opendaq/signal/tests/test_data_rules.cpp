#include <gtest/gtest.h>
#include <opendaq/data_rule_factory.h>
#include <opendaq/signal_exceptions.h>

using DataRulesTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

TEST_F(DataRulesTest, DataRuleSetGet)
{
    const auto rule = DataRuleBuilder().addParameter("test", 10).addParameter("test1", 10.5).build();

    ASSERT_EQ(rule.getParameters().get("test"), 10);
    ASSERT_EQ(rule.getParameters().get("test1"), 10.5);
}

TEST_F(DataRulesTest, LinearDataRuleSetGet)
{
    const auto rule = LinearDataRule(10, 20);

    ASSERT_EQ(rule.getType(), DataRuleType::Linear);
    ASSERT_EQ(rule.getParameters().get("Delta"), 10);
    ASSERT_EQ(rule.getParameters().get("Start"), 20);
}

TEST_F(DataRulesTest, ConstantDataRuleSetGet)
{
    const auto rule = ConstantDataRule();

    ASSERT_EQ(rule.getType(), DataRuleType::Constant);
}

TEST_F(DataRulesTest, ExplicitDataRule)
{
    const auto rule = ExplicitDataRule();

    ASSERT_EQ(rule.getType(), DataRuleType::Explicit);
}

TEST_F(DataRulesTest, LinearDataRuleCopyFactory)
{
    const auto rule = LinearDataRule(100, 50);
    const auto ruleCopy = DataRuleBuilderCopy(rule).build();

    ASSERT_EQ(ruleCopy.getParameters().get("Delta"), 100);
    ASSERT_EQ(ruleCopy.getParameters().get("Start"), 50);
}

TEST_F(DataRulesTest, ConstantDataRuleCopyFactory)
{
    const auto rule = ConstantDataRule();
    const auto ruleCopy = DataRuleBuilderCopy(rule).build();

    ASSERT_EQ(ruleCopy.getType(), DataRuleType::Constant);
}

TEST_F(DataRulesTest, ExplicitDataRuleCopyFactory)
{
    const auto rule = ExplicitDataRule();
    const auto ruleCopy = DataRuleBuilderCopy(rule).build();
    ASSERT_EQ(ruleCopy.getType(), rule.getType());
}

TEST_F(DataRulesTest, LinearDataRuleInvalidParameters)
{
    auto ruleBuilder = DataRuleBuilder().setType(DataRuleType::Linear);
    ASSERT_THROW(ruleBuilder.build(), InvalidParametersException);

    auto params = Dict<IString, IBaseObject>();
    ruleBuilder.setParameters(params);
    ASSERT_THROW(ruleBuilder.build(), InvalidParametersException);

    params.set("Delta", "wrong");
    ASSERT_THROW(ruleBuilder.build(), InvalidParametersException);

    params.set("Delta", 10);
    params.set("Start", 10);
    params.set("extra", 10);
    ASSERT_THROW(ruleBuilder.build(), InvalidParametersException);

    params.deleteItem("extra");
    ASSERT_NO_THROW(ruleBuilder.build());
}

TEST_F(DataRulesTest, SerializeDeserialize)
{
    const auto rule = LinearDataRule(100, 50);
    auto serializer = JsonSerializer(False);
    rule.serialize(serializer);

    auto serialized = serializer.getOutput();

    auto deserializer = JsonDeserializer();
    auto rule1 = deserializer.deserialize(serialized.toStdString()).asPtr<IDataRule>();

    ASSERT_EQ(rule1, rule);
}

TEST_F(DataRulesTest, StructType)
{
    const auto structType = daq::DataRuleStructType();
    const daq::StructPtr structPtr = LinearDataRule(10, 10);
    ASSERT_EQ(structType, structPtr.getStructType());
}

TEST_F(DataRulesTest, StructFields)
{
    const daq::StructPtr structPtr = LinearDataRule(10, 10);
    ASSERT_EQ(structPtr.get("Type"), static_cast<Int>(DataRuleType::Linear));
    
    const auto params = Dict<IString, IBaseObject>({
            {"Delta", 10},
            {"Start", 10}
        });
    ASSERT_EQ(structPtr.get("Parameters"), params);
}

TEST_F(DataRulesTest, StructNames)
{
    const auto structType = daq::DataRuleStructType();
    const daq::StructPtr structPtr = LinearDataRule(10, 10);
    ASSERT_EQ(structType.getFieldNames(), structPtr.getFieldNames());
}

TEST_F(DataRulesTest, ExplicitDomainDataRule)
{
    const auto rule = ExplicitDomainDataRule(10, 20.5);
    const auto rule1 = ExplicitDomainDataRule(10);
    const auto rule2 = ExplicitDomainDataRule();

    ASSERT_EQ(rule.getParameters().get("minExpectedDelta"), 10);
    ASSERT_EQ(rule.getParameters().get("maxExpectedDelta"), 20.5);

    ASSERT_EQ(rule1.getParameters().get("minExpectedDelta"), 10);
    ASSERT_EQ(rule1.getParameters().get("maxExpectedDelta"), 0);

    ASSERT_EQ(rule2.getParameters().get("minExpectedDelta"), 0);
    ASSERT_EQ(rule2.getParameters().get("maxExpectedDelta"), 0);
}

TEST_F(DataRulesTest, DataRuleBuilderSetGet)
{
    const auto params = Dict<IString, IBaseObject>({
            {"Delta", 10},
            {"Start", 10}
        });
    const auto dataRuleBuilder = DataRuleBuilder()
                                .setType(DataRuleType::Linear)
                                .setParameters(params);
    
    ASSERT_EQ(dataRuleBuilder.getType(), DataRuleType::Linear);
    ASSERT_EQ(dataRuleBuilder.getParameters(), params);
}

TEST_F(DataRulesTest, DataRuleCreateFactory)
{
    const auto params = Dict<IString, IBaseObject>({
            {"Delta", 10},
            {"Start", 10}
        });
    const auto dataRuleBuilder = DataRuleBuilder()
                                .setType(DataRuleType::Linear)
                                .setParameters(params);
    const auto dataRule = DataRuleFromBuilder(dataRuleBuilder);

    ASSERT_EQ(dataRuleBuilder.getType(), DataRuleType::Linear);
    ASSERT_EQ(dataRuleBuilder.getParameters(), params);
}


END_NAMESPACE_OPENDAQ
