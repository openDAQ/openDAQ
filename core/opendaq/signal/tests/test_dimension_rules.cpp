#include <opendaq/dimension_rule_factory.h>
#include <opendaq/signal_exceptions.h>
#include <gtest/gtest.h>

using DimensionRulesTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

TEST_F(DimensionRulesTest, DimensionRuleSetGet)
{
    const auto rule = DimensionRuleBuilder()
                      .addParameter("test", 10)
                      .addParameter("test1", 10.5)
                      .build();

    ASSERT_EQ(rule.getParameters().get("test"), 10);
    ASSERT_EQ(rule.getParameters().get("test1"), 10.5);
}

TEST_F(DimensionRulesTest, LinearDimensionRuleSetGet)
{
    const auto rule = LinearDimensionRule(10, 20, 10);

    ASSERT_EQ(rule.getType(), DimensionRuleType::Linear);
    ASSERT_EQ(rule.getParameters().get("delta"), 10);
    ASSERT_EQ(rule.getParameters().get("start"), 20);
    ASSERT_EQ(rule.getParameters().get("size"), 10);
}

TEST_F(DimensionRulesTest, ListDimensionRuleSetGet)
{
    const auto rule = ListDimensionRule(List<INumber>(1, 2, 3, 4, 5, 6, 7));

    ASSERT_EQ(rule.getType(), DimensionRuleType::List);
    ASSERT_EQ(rule.getParameters().get("List").asPtr<IList>()[2], 3);
}

TEST_F(DimensionRulesTest, LogarithmicDimensionRule)
{
    const auto rule = LogarithmicDimensionRule(1, 0, 10, 10);

    ASSERT_EQ(rule.getParameters().get("delta"), 1);
    ASSERT_EQ(rule.getParameters().get("start"), 0);
    ASSERT_EQ(rule.getParameters().get("base"), 10);
    ASSERT_EQ(rule.getParameters().get("size"), 10);
    ASSERT_EQ(rule.getType(), DimensionRuleType::Logarithmic);
}

TEST_F(DimensionRulesTest, DimensionRuleFreeze)
{
    const auto rule = LinearDimensionRule(50, 50, 10);
    ASSERT_THROW(rule.getParameters().set("delta", 10), FrozenException);
    ASSERT_THROW(rule.getParameters().set("start", 10.5), FrozenException);
}

TEST_F(DimensionRulesTest, LinearDimensionRuleCopyFactory)
{
    const auto rule = LinearDimensionRule(100, 50, 10);
    const auto ruleCopy = DimensionRuleBuilderCopy(rule).build();

    ASSERT_EQ(ruleCopy.getParameters().get("delta"), 100);
    ASSERT_EQ(ruleCopy.getParameters().get("start"), 50);
    ASSERT_EQ(ruleCopy.getParameters().get("size"), 10);
}

TEST_F(DimensionRulesTest, ListDimensionRuleCopyFactory)
{
    const auto rule = ListDimensionRule(List<INumber>(1, 2, 3, 4, 5, 6, 7));
    const auto ruleCopy = DimensionRuleBuilderCopy(rule).build();

    ASSERT_TRUE(ruleCopy.getParameters().hasKey("List"));
}

TEST_F(DimensionRulesTest, LogarithmicDimensionRuleCopyFactory)
{
    const auto rule = LogarithmicDimensionRule(1, 0, 10, 10);
    const auto ruleCopy = DimensionRuleBuilderCopy(rule).build();
    
    ASSERT_EQ(ruleCopy.getParameters().get("delta"), 1);
    ASSERT_EQ(ruleCopy.getParameters().get("start"), 0);
    ASSERT_EQ(ruleCopy.getParameters().get("base"), 10);
    ASSERT_EQ(ruleCopy.getParameters().get("size"), 10);
    ASSERT_EQ(ruleCopy.getType(), DimensionRuleType::Logarithmic);
}

TEST_F(DimensionRulesTest, LinearDimensionRuleInvalidParameters)
{
    auto ruleBuilder = DimensionRuleBuilder().setType(DimensionRuleType::Linear);
    ASSERT_THROW(ruleBuilder.build(), InvalidParametersException);

    auto params = Dict<IString, IBaseObject>();
    ruleBuilder.setParameters(params);
    ASSERT_THROW(ruleBuilder.build(), InvalidParametersException);

    params.set("delta", "wrong");
    ASSERT_THROW(ruleBuilder.build(), InvalidParametersException);

    params.set("delta", 10);
    params.set("start", 10);
    params.set("size", 10);
    params.set("extra", 10);
    ASSERT_THROW(ruleBuilder.build(), InvalidParametersException);

    params.deleteItem("extra");
    ASSERT_NO_THROW(ruleBuilder.build());
}

TEST_F(DimensionRulesTest, LogarithmicDimensionRuleInvalidParameters)
{
    auto ruleBuilder = DimensionRuleBuilder().setType(DimensionRuleType::Logarithmic);
    ASSERT_THROW(ruleBuilder.build(), InvalidParametersException);

    auto params = Dict<IString, IBaseObject>();
    ruleBuilder.setParameters(params);
    ASSERT_THROW(ruleBuilder.build(), InvalidParametersException);

    params.set("delta", "wrong");
    ASSERT_THROW(ruleBuilder.build(), InvalidParametersException);

    params.set("delta", 10);
    params.set("start", 10);
    params.set("size", 10);
    params.set("base", 10);
    params.set("extra", 10);
    ASSERT_THROW(ruleBuilder.build(), InvalidParametersException);

    params.deleteItem("extra");
    ASSERT_NO_THROW(ruleBuilder.build());
}

TEST_F(DimensionRulesTest, Equals)
{
    auto rule = LinearDimensionRule(10, 20, 10);
    auto ruleCopy = DimensionRuleBuilderCopy(rule).build();

    Bool eq{false};
    ruleCopy->equals(rule, &eq);
    ASSERT_TRUE(eq);

    /// TODO: to be enabled again after DictPtr has a working equals()
    //rule = LinearDimensionRule(10, 20, 5);
    //ruleCopy->equals(rule, &eq);
    //ASSERT_FALSE(eq);

    rule = LogarithmicDimensionRule(1, 0, 10, 10);
    ruleCopy->equals(rule, &eq);
    ASSERT_FALSE(eq);
}

TEST_F(DimensionRulesTest, DataRuleSerializeDeserialize)
{
    auto rule = LinearDimensionRule(10, 20, 10);
    auto serializer = JsonSerializer(False);
    rule.serialize(serializer);

    auto serialized = serializer.getOutput();

    auto deserializer = JsonDeserializer();
    auto rule1 = deserializer.deserialize(serialized.toStdString()).asPtr<IDimensionRule>();

    ASSERT_EQ(rule1, rule);
}

TEST_F(DimensionRulesTest, StructType)
{
    const auto structType = daq::DimensionRuleStructType();
    const daq::StructPtr structPtr = LinearDimensionRule(10, 10, 10);
    ASSERT_EQ(structType, structPtr.getStructType());
}

TEST_F(DimensionRulesTest, StructFields)
{
    const daq::StructPtr structPtr = LinearDimensionRule(10, 10, 10);
    ASSERT_EQ(structPtr.get("RuleType"), static_cast<Int>(DimensionRuleType::Linear));
    
    const auto params = Dict<IString, IBaseObject>({
            {"delta", 10},
            {"start", 10},
            {"size", 10}
        });
    ASSERT_EQ(structPtr.get("Parameters"), params);
}

TEST_F(DimensionRulesTest, StructNames)
{
    const auto structType = daq::DimensionRuleStructType();
    const daq::StructPtr structPtr = LinearDimensionRule(10, 10, 10);
    ASSERT_EQ(structType.getFieldNames(), structPtr.getFieldNames());
}

TEST_F(DimensionRulesTest, DimensionRuleBuilderSetGet)
{
    const auto ruleType = DimensionRuleType::Linear;
    const auto params = Dict<IString, IBaseObject>({
            {"delta", 10},
            {"start", 10},
            {"size", 10}
        });
    const auto dimensionRuleBuilder = DimensionRuleBuilder()
                                        .setType(ruleType)
                                        .setParameters(params);
    
    ASSERT_EQ(dimensionRuleBuilder.getType(), ruleType);
    ASSERT_EQ(dimensionRuleBuilder.getParameters(), params);
}

TEST_F(DimensionRulesTest, DimensionRuleCreateFactory)
{
    const auto ruleType = DimensionRuleType::Linear;
    const auto params = Dict<IString, IBaseObject>({
            {"delta", 10},
            {"start", 10},
            {"size", 10}
        });
    const auto dimensionRuleBuilder = DimensionRuleBuilder()
                                        .setType(ruleType)
                                        .setParameters(params);
    const auto dimensionRule = DimensionRuleFromBuilder(dimensionRuleBuilder);

    ASSERT_EQ(dimensionRule.getType(), ruleType);
    ASSERT_EQ(dimensionRule.getParameters(), params);
}

END_NAMESPACE_OPENDAQ
