#include <gtest/gtest.h>
#include <testutils/testutils.h>
#include <coreobjects/coreobjects.h>

using namespace daq;

class EvalValueTest : public testing::Test
{
protected:
    TypeManagerPtr manager;
    void SetUp() override
    {
        manager = TypeManager();
    }
};

TEST_F(EvalValueTest, Create)
{
    auto e = EvalValue("1+1");
}

TEST_F(EvalValueTest, ParseFailed)
{
    ASSERT_THROW_MSG(EvalValue(u8"1+1+\u20AC"), ParseFailedException, "syntax error");
    ASSERT_THROW_MSG(EvalValue("1+1+b"), ParseFailedException, "invalid identifier");
    ASSERT_THROW_MSG(EvalValue("(1, 2)"), ParseFailedException, "syntax error");
}

TEST_F(EvalValueTest, TestFloat)
{
    auto e = EvalValue("1+1.0");
    ASSERT_EQ(e.getCoreType(), ctFloat);
    e = EvalValue(".0");
    ASSERT_EQ(e.getCoreType(), ctFloat);
    e = EvalValue(".0e3");
    ASSERT_EQ(e.getCoreType(), ctFloat);
    e = EvalValue(".0e-2");
    ASSERT_EQ(e.getCoreType(), ctFloat);
    e = EvalValue(".3e+41");
    ASSERT_EQ(e.getCoreType(), ctFloat);
    e = EvalValue("4e3");
    ASSERT_EQ(e.getCoreType(), ctFloat);
    e = EvalValue("4e-3");
    ASSERT_EQ(e.getCoreType(), ctFloat);
    e = EvalValue("3.3e+41");
    ASSERT_EQ(e.getCoreType(), ctFloat);
}

TEST_F(EvalValueTest, TestBool)
{
    auto e = EvalValue("True && false");
    ASSERT_EQ(e.getCoreType(), ctBool);
}

TEST_F(EvalValueTest, TestInt)
{
    auto e = EvalValue("1 + 1");
    ASSERT_EQ(e.getCoreType(), ctInt);
}

TEST_F(EvalValueTest, TestString)
{
    auto e = EvalValue("'ab'");
    ASSERT_EQ(e.getCoreType(), ctString);
}

TEST_F(EvalValueTest, IntResult)
{
    Int r = EvalValue("1 +1");
    ASSERT_EQ(r, 2LL);

    r = EvalValue("2 - 1");
    ASSERT_EQ(r, 1LL);
}

TEST_F(EvalValueTest, FloatResult)
{
    Float r = EvalValue("1.0 +1");
    ASSERT_EQ(r, 2.0);
}

TEST_F(EvalValueTest, BoolResult)
{
    Bool r = EvalValue("True || False");
    ASSERT_EQ(r, True);

    r = EvalValue("True && False");
    ASSERT_EQ(r, False);
}

TEST_F(EvalValueTest, StringResult)
{
    std::string str = EvalValue("'ab' + 'cd'");
#ifndef __MINGW32__   // temp workaround for mingw issue
    ASSERT_EQ(str, "abcd");
#endif
}

TEST_F(EvalValueTest, TestPrecedence)
{
    Int r = EvalValue("1 + 2 * 3");
    ASSERT_EQ(r, 7LL);

    r = EvalValue("1 * 2 + 3");
    ASSERT_EQ(r, 5LL);

    Bool b = EvalValue("1 * 2 == 3 + -1");
    ASSERT_EQ(b, True);
}

TEST_F(EvalValueTest, TestAssociativity)
{
    Int r = EvalValue("6 - 3 - 1");
    ASSERT_EQ(r, 2LL);

    r = EvalValue("16 / 4 / 2");
    ASSERT_EQ(r, 2LL);

    Bool b = EvalValue("3 == -3 == False");
    ASSERT_EQ(b, True);
    
    b = EvalValue("1 > 0 && 2 > 1");
    ASSERT_EQ(b, True);

    b = EvalValue("1 > 2 && 2 > 1");
    ASSERT_EQ(b, False);

    b = EvalValue("1 > 2 || 2 > 1");
    ASSERT_EQ(b, True);

    b = EvalValue("1 > 2 || 2 > 3");
    ASSERT_EQ(b, False);
}

TEST_F(EvalValueTest, IntResultConversion)
{
    Float r1 = EvalValue("1 + 1");
    ASSERT_EQ(r1, 2.0);

    Bool r2 = EvalValue("1 + 1");
    ASSERT_EQ(r2, True);

    std::string r3 = EvalValue("1 + 1");
    ASSERT_EQ(r3, "2");
}

TEST_F(EvalValueTest, FloatResultConversion)
{
    Int r1 = EvalValue("1.0 + 1.0");
    ASSERT_EQ(r1, 2LL);

    Bool r2 = EvalValue("1.0 + 1.0");
    ASSERT_EQ(r2, True);

    std::string r3 = EvalValue("1.0 + 1.0");
    ASSERT_EQ(r3, "2");
}

TEST_F(EvalValueTest, BoolResultConversion)
{
    Int r1 = EvalValue("True || False");
    ASSERT_EQ(r1, 1LL);

    Float r2 = EvalValue("True && False");
    ASSERT_EQ(r2, 0);

    std::string r3 = EvalValue("True || False");
    ASSERT_EQ(r3, "True");
}

TEST_F(EvalValueTest, StringResultConversion)
{
    Int r1 = EvalValue("'3'");
    ASSERT_EQ(r1, 3LL);

    Float r2 = EvalValue("'3'");
    ASSERT_EQ(r2, 3.0);

    Bool r3 = EvalValue("'3'");
    ASSERT_EQ(r3, True);
}

TEST_F(EvalValueTest, Brackets)
{
    Int r = EvalValue("2 * (3 + 2)");
    ASSERT_EQ(r, 10LL);
}

TEST_F(EvalValueTest, TestIf)
{
    auto e = EvalValue("if(True, 1, 0)");
}

TEST_F(EvalValueTest, IfResult)
{
    Int r = EvalValue("if(True, 1, 0)");
    ASSERT_EQ(r, 1);

    r = EvalValue("if(False, 1, 0)");
    ASSERT_EQ(r, 0);
}

TEST_F(EvalValueTest, TestList)
{
    auto e = EvalValue("[1, 2, 3]");
    ASSERT_EQ(e.getCoreType(), ctList);
}

TEST_F(EvalValueTest, TestEmptyList)
{
    auto e1 = EvalValue("[]");
    ASSERT_EQ(e1.getCoreType(), ctList);
}

TEST_F(EvalValueTest, ListResult)
{
    auto e = EvalValue("[1, 2, 3+4]");
    ListPtr<IBaseObject> list = e;
    ASSERT_EQ(list.getCount(), 3u);
    ASSERT_EQ(list.getItemAt(0), 1LL);
    ASSERT_EQ(list.getItemAt(1), 2LL);
    ASSERT_EQ(list.getItemAt(2), 7LL);
}

TEST_F(EvalValueTest, ListIfResult)
{
    auto e = EvalValue("If(True, [1, 2, 3+4], [])");
    ListPtr<IBaseObject> list = e;
    ASSERT_EQ(list.getCount(), 3u);
    ASSERT_EQ(list.getItemAt(0), 1LL);
    ASSERT_EQ(list.getItemAt(1), 2LL);
    ASSERT_EQ(list.getItemAt(2), 7LL);
}

TEST_F(EvalValueTest, TwoListAdd)
{
    auto e = EvalValue("[1, 2, 3] + [1, 2, 3]");
    ListPtr<IBaseObject> list = e;
    ASSERT_EQ(list.getCount(), 3u);
    ASSERT_EQ(list.getItemAt(0), 2);
    ASSERT_EQ(list.getItemAt(1), 4);
    ASSERT_EQ(list.getItemAt(2), 6);
}

TEST_F(EvalValueTest, ListMulScalar)
{
    auto e = EvalValue("[1, 2, 3] * 2");
    ListPtr<IBaseObject> list = e;
    ASSERT_EQ(list.getCount(), 3u);
    ASSERT_EQ(list.getItemAt(0), 2);
    ASSERT_EQ(list.getItemAt(1), 4);
    ASSERT_EQ(list.getItemAt(2), 6);
}

TEST_F(EvalValueTest, TestEquals)
{
    auto e = EvalValue("1 == 1");
    ASSERT_EQ(e.getCoreType(), ctBool);

    Bool b1 = EvalValue("1 == 1");
    ASSERT_EQ(b1, True);

    Bool b2 = EvalValue("1 == 0");
    ASSERT_EQ(b2, False);

    Bool b3 = EvalValue("1 != 2");
    ASSERT_EQ(b3, True);

    Bool b4 = EvalValue("1 != 1");
    ASSERT_EQ(b4, False);
}

TEST_F(EvalValueTest, TestGreater)
{
    auto e = EvalValue("2 > 1");
    ASSERT_EQ(e.getCoreType(), ctBool);

    Bool b1 = EvalValue("2 > 1");
    ASSERT_EQ(b1, True);

    Bool b2 = EvalValue("1 > 2");
    ASSERT_EQ(b2, False);

    Bool b3 = EvalValue("2 > 2");
    ASSERT_EQ(b3, False);

    Bool b4 = EvalValue("2 >= 2");
    ASSERT_EQ(b4, True);

    Bool b5 = EvalValue("2 >= 1");
    ASSERT_EQ(b5, True);

    Bool b6 = EvalValue("1 >= 2");
    ASSERT_EQ(b6, False);
}

TEST_F(EvalValueTest, TestLower)
{
    auto e = EvalValue("1 < 2");
    ASSERT_EQ(e.getCoreType(), ctBool);

    Bool b1 = EvalValue("1 < 2");
    ASSERT_EQ(b1, True);

    Bool b2 = EvalValue("2 < 1");
    ASSERT_EQ(b2, False);

    Bool b3 = EvalValue("2 < 2");
    ASSERT_EQ(b3, False);

    Bool b4 = EvalValue("2 <= 2");
    ASSERT_EQ(b4, True);

    Bool b5 = EvalValue("1 <= 2");
    ASSERT_EQ(b5, True);

    Bool b6 = EvalValue("2 <= 1");
    ASSERT_EQ(b6, False);
}

TEST_F(EvalValueTest, Switch)
{
    auto e = EvalValue("switch(1, 0, 0.1, 1, 0.2, 0.5)");
    ASSERT_EQ(e.getCoreType(), ctFloat);
    Float val = e;
    ASSERT_DOUBLE_EQ(val, 0.2);
}

TEST_F(EvalValueTest, SwitchWithExpression)
{
    auto e = EvalValue("switch(1, 0, 0.1, 1, 0.2 + 1, 0.5)");
    ASSERT_EQ(e.getCoreType(), ctFloat);
    Float val = e;
    ASSERT_DOUBLE_EQ(val, 1.2);
}

TEST_F(EvalValueTest, SwitchDefault)
{
    auto e = EvalValue("switch(10, 0, 0.1, 1, 0.2, 0.5)");
    ASSERT_EQ(e.getCoreType(), ctFloat);
    Float val = e;
    ASSERT_DOUBLE_EQ(val, 0.5);
}

TEST_F(EvalValueTest, SwitchNoDefault)
{
    auto e = EvalValue("switch(10, 0, 0.1, 1, 0.2)");
    ASSERT_THROW(e.getCoreType(), CalcFailedException);
    ASSERT_THROW(e.getResult(), CalcFailedException);
}

TEST_F(EvalValueTest, MixedTypes)
{
    auto e = EvalValue("switch(2, 0, 0.1, 1, 10, 2, 'Test')");
    ASSERT_EQ(e.getCoreType(), ctString);
    ASSERT_EQ(e, "Test");
}

TEST_F(EvalValueTest, Precedence)
{
    // OPENDAQ_TODO: precedence not working correct
    // auto e = EvalValue("0 == 1 && 0 == 1");
    // ASSERT_EQ(e, "False");
}

TEST_F(EvalValueTest, ArgumentProperty)
{
    auto testProperty = StringProperty("Test", "default");
    //auto testProperty = Property("Test");

    GenericPropertyObjectPtr propObj = PropertyObject();
    propObj.addProperty(testProperty);

    //auto e = EvalValue("{0}.%Test", propObj);
    //PropertyPtr result = e.getResult();

    //ASSERT_EQ(testProperty.getName(), result.getName());
    //ASSERT_EQ(testProperty.getDefaultValue(), result.getDefaultValue());
}

TEST_F(EvalValueTest, ArgumentDefaultValue)
{
    GenericPropertyObjectPtr propObj = PropertyObject();
    propObj.addProperty(StringProperty("Test", "default"));

    auto e = EvalValue("{0}.$Test", propObj);
    ASSERT_EQ("default", e.getResult());
}

TEST_F(EvalValueTest, ArgumentNonDefaultValue)
{
    GenericPropertyObjectPtr obj = PropertyObject();
    obj.addProperty(StringProperty("Test", "default"));
    obj.setPropertyValue("Test", "NOT default");

    auto e = EvalValue("{0}.$Test", obj);
    ASSERT_EQ("NOT default", e.getResult());
}

TEST_F(EvalValueTest, ArgExpression)
{
    Int lhs = 1;
    Int rhs = 2;

    GenericPropertyObjectPtr obj = PropertyObject();
    obj.addProperty(IntProperty("TestLhs", lhs));
    obj.addProperty(IntProperty("TestRhs", rhs));

    auto e = EvalValue("{0}.$TestLhs + {0}.$TestRhs", obj);
    Int result = e.getResult();

    ASSERT_EQ(lhs + rhs, result);
}

TEST_F(EvalValueTest, AddPropertyValues)
{
    Int lhs = 1;
    Int rhs = 2;

    auto propClass = PropertyObjectClassBuilder("TestClass")
                     .addProperty(IntProperty("Addition", EvalValue("$TestLhs + $TestRhs")))
                     .build();

    manager.addType(propClass);

    GenericPropertyObjectPtr obj = PropertyObject(manager, propClass.getName());
    obj.addProperty(IntProperty("TestLhs", lhs));
    obj.addProperty(IntProperty("TestRhs", rhs));

    auto result = obj.getPropertyValue("Addition");

    ASSERT_EQ(lhs + rhs, result);
}

TEST_F(EvalValueTest, SelectionPropertyvalue)
{
    auto propObj = PropertyObject();
    propObj.addProperty(SelectionProperty("sel", List<IString>("foo", "bar"), 1));

    auto eval = EvalValue("%sel:SelectedValue").cloneWithOwner(propObj);
    auto eval1 = EvalValue("%sel:Value").cloneWithOwner(propObj);
    auto eval2 = EvalValue("$sel").cloneWithOwner(propObj);
    ASSERT_EQ(eval.getResult(), "bar");

    propObj.setPropertyValue("sel", 0);
    ASSERT_EQ(eval.getResult(), "foo");
}

TEST_F(EvalValueTest, Index)
{
    auto lhs = List<Int>(1, 2, 3);
    auto rhs = List<Int>(4, 5, 6);

    auto propClass = PropertyObjectClassBuilder("TestClass")
                     .addProperty(IntProperty("Addition", EvalValue("$TestLhs[1] + $TestRhs[2]")))
                     .build();

    manager.addType(propClass);

    GenericPropertyObjectPtr obj = PropertyObject(manager, propClass.getName());
    obj.addProperty(ListProperty("TestLhs", lhs));
    obj.addProperty(ListProperty("TestRhs", rhs));
    
    ASSERT_EQ(lhs[1] + rhs[2], obj.getPropertyValue("Addition"));
}

TEST_F(EvalValueTest, ArgumentNonDefaultValueIndex)
{
    GenericPropertyObjectPtr obj = PropertyObject();
    obj.addProperty(ListProperty("Test", List<Int>(1, 2, 3)));
    obj.setPropertyValue("Test", List<Int>(4, 5, 6));

    auto e = EvalValue("$Test[0]", obj);
    e.asPtr<IOwnable>().setOwner(obj);

    Int result = e.getResult();

    ASSERT_EQ(4, result);
}

TEST_F(EvalValueTest, UnaryMinus)
{
    int a = EvalValue("-1");

    ASSERT_EQ(a, -1);

    a = EvalValue("-3 + -2");

    ASSERT_EQ(a, -5);

    a = EvalValue("--3---4");

    ASSERT_EQ(a, -1);
}

TEST_F(EvalValueTest, UnaryMinusFloat)
{
    Float a = EvalValue("-1.5");

    ASSERT_EQ(a, -1.5);
}

TEST_F(EvalValueTest, UnaryMinusRef)
{
    GenericPropertyObjectPtr obj = PropertyObject();
    obj.addProperty(FloatProperty("Test", 1.5));
    obj.setPropertyValue("Test", 2.6);

    auto e = EvalValue("-$Test", obj);
    e.asPtr<IOwnable>().setOwner(obj);

    Float result = e.getResult();

    ASSERT_EQ(result, -2.6);
}

TEST_F(EvalValueTest, UnaryMinusListItem)
{
    GenericPropertyObjectPtr obj = PropertyObject();
    obj.addProperty(ListProperty("Test", List<Float>(8.321, 2, 3)));
    obj.setPropertyValue("Test", List<Float>(321.8, 5, 6));

    auto e = EvalValue("-$Test[0]", obj);
    e.asPtr<IOwnable>().setOwner(obj);

    Float result = e.getResult();

    ASSERT_EQ(-321.8, result);
}

TEST_F(EvalValueTest, FuncOneTag)
{
    auto hasTagFunc = Function([](const BaseObjectPtr& tag)
    {
        return tag == "channel";
    });

    auto ev1 = EvalValueFunc("channel", hasTagFunc);
    Bool tagOk1 = ev1;
    ASSERT_TRUE(tagOk1);

    auto ev2 = EvalValueFunc("device", hasTagFunc);
    Bool tagOk2 = ev2;
    ASSERT_FALSE(tagOk2);
}

TEST_F(EvalValueTest, FuncMultipleTags)
{
    auto hasTagFunc = Function([](const BaseObjectPtr& tag)
    {
        return tag == "channel" || tag == "device";
    });

    auto ev1 = EvalValueFunc("channel && device", hasTagFunc);
    Bool tagOk1 = ev1;
    ASSERT_TRUE(tagOk1);

    auto ev2 = EvalValueFunc("channel || device", hasTagFunc);
    Bool tagOk2 = ev2;
    ASSERT_TRUE(tagOk2);

    auto ev3 = EvalValueFunc("channel && !module", hasTagFunc);
    Bool tagOk3 = ev3;
    ASSERT_TRUE(tagOk3);

    auto ev4 = EvalValueFunc("module || !node", hasTagFunc);
    Bool tagOk4 = ev4;
    ASSERT_TRUE(tagOk4);
}

TEST_F(EvalValueTest, Inspectable)
{
    auto obj = EvalValue("-1");

    auto ids = obj.asPtr<IInspectable>(true).getInterfaceIds();
    ASSERT_EQ(ids[0], IEvalValue::Id);
}

TEST_F(EvalValueTest, ImplementationName)
{
    auto obj = EvalValue("-1");

    StringPtr className = obj.asPtr<IInspectable>(true).getRuntimeClassName();
    ASSERT_EQ(className, "daq::EvalValueImpl");
}

TEST_F(EvalValueTest, Unit)
{
    UnitPtr unit = EvalValue("Unit('s', 'seconds', 'time', 0)");

    ASSERT_EQ(unit.getSymbol(), "s");
    ASSERT_EQ(unit.getName(), "seconds");
    ASSERT_EQ(unit.getQuantity(), "time");
    ASSERT_EQ(unit.getId(), 0);
}

TEST_F(EvalValueTest, UnitPartial)
{
    UnitPtr unit1 = EvalValue("Unit('s')");
    UnitPtr unit2 = EvalValue("Unit('s', 'seconds')");
    UnitPtr unit3 = EvalValue("Unit('s', 'seconds', 'time')");

    ASSERT_EQ(unit1.getName(), "");
    ASSERT_EQ(unit2.getQuantity(), "");
    ASSERT_EQ(unit3.getId(), -1);
}
