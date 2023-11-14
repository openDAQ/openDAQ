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
    ASSERT_EQ(rule.getParameters().get("delta"), 10);
    ASSERT_EQ(rule.getParameters().get("start"), 20);
}

TEST_F(DataRulesTest, ConstantDataRuleSetGet)
{
    const auto rule = ConstantDataRule(10.5);

    ASSERT_EQ(rule.getType(), DataRuleType::Constant);
    ASSERT_EQ(rule.getParameters().get("constant"), 10.5);
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

    ASSERT_EQ(ruleCopy.getParameters().get("delta"), 100);
    ASSERT_EQ(ruleCopy.getParameters().get("start"), 50);
}

TEST_F(DataRulesTest, ConstantDataRuleCopyFactory)
{
    const auto rule = ConstantDataRule(100);
    const auto ruleCopy = DataRuleBuilderCopy(rule).build();

    ASSERT_EQ(ruleCopy.getParameters().get("constant"), 100);
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

    params.set("delta", "wrong");
    ASSERT_THROW(ruleBuilder.build(), InvalidParametersException);

    params.set("delta", 10);
    params.set("start", 10);
    params.set("extra", 10);
    ASSERT_THROW(ruleBuilder.build(), InvalidParametersException);

    params.deleteItem("extra");
    ASSERT_NO_THROW(ruleBuilder.build());
}

TEST_F(DataRulesTest, ConstantDataRuleInvalidParameters)
{
    auto ruleBuilder = DataRuleBuilder().setType(DataRuleType::Constant);
    ASSERT_THROW(ruleBuilder.build(), InvalidParametersException);

    auto params = Dict<IString, IBaseObject>();
    ruleBuilder.setParameters(params);
    ASSERT_THROW(ruleBuilder.build(), InvalidParametersException);

    params.set("constant", "wrong");
    ASSERT_THROW(ruleBuilder.build(), InvalidParametersException);

    params.set("constant", 10);
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
    ASSERT_EQ(structPtr.get("type"), static_cast<Int>(DataRuleType::Linear));
    
    const auto params = Dict<IString, IBaseObject>({
            {"delta", 10},
            {"start", 10}
        });
    ASSERT_EQ(structPtr.get("parameters"), params);
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

END_NAMESPACE_OPENDAQ
