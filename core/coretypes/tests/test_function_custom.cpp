#include <gtest/gtest.h>
#include <coretypes/function_factory.h>
#include <coretypes/baseobject_factory.h>
#include <coretypes/listptr.h>
#include <coretypes/inspectable_ptr.h>

using namespace daq;

using CustomFunctionTest = testing::Test;

TEST_F(CustomFunctionTest, Basic)
{
    auto funcObj = CustomFunction([](IBaseObject*, IBaseObject**)
    {
        return OPENDAQ_SUCCESS;
    });
    ErrCode err = funcObj->call(nullptr, nullptr);
    ASSERT_EQ(err, OPENDAQ_ERR_ARGUMENT_NULL);

    IBaseObject* result;
    err = funcObj->call(nullptr, &result);
    ASSERT_EQ(err, OPENDAQ_SUCCESS);
}

TEST_F(CustomFunctionTest, BasicSumFunc)
{
    auto funcObj = CustomFunction([](IBaseObject* params, IBaseObject** result)
    {
        auto list = ListObjectPtr<IList, IBaseObject>(params);

        Int a = list.getItemAt(0);
        Int b = list.getItemAt(1);

        *result = Integer_Create(a + b);
        return OPENDAQ_SUCCESS;
    });

    ObjectPtr<IInteger> a = 5;
    ObjectPtr<IInteger> b = 4;
    ObjectPtr<IList> list(List_Create());
    list->pushBack(a);
    list->pushBack(b);

    ObjectPtr<IBaseObject> result;
    ErrCode err = funcObj->call(list, &result);
    ASSERT_EQ((Int)result, 9);
    ASSERT_EQ(err, OPENDAQ_SUCCESS);
}

TEST_F(CustomFunctionTest, SmartPtr)
{
    auto funcObj = CustomFunction([](IBaseObject*, IBaseObject**)
    {
        return OPENDAQ_SUCCESS;
    });

    auto result = funcObj.call();
    ASSERT_FALSE(result.assigned());

    result = funcObj.call(BaseObjectPtr(nullptr));
    ASSERT_FALSE(result.assigned());
}

TEST_F(CustomFunctionTest, BaseObjectPtr)
{
    BaseObjectPtr funcObj = CustomFunction([](IBaseObject*, IBaseObject**)
    {
        return OPENDAQ_SUCCESS;
    });

    auto result = funcObj.call();
    ASSERT_FALSE(result.assigned());

    result = funcObj.call(BaseObjectPtr(nullptr));
    ASSERT_FALSE(result.assigned());
}

TEST_F(CustomFunctionTest, Params)
{
    auto funcObj = CustomFunction([](IBaseObject* obj, IBaseObject** result)
    {
        Int a = BaseObjectPtr::Borrow(obj);
        *result = Integer_Create(a + 2);

        return a == 3
            ? OPENDAQ_SUCCESS
            :  OPENDAQ_ERR_INVALIDPARAMETER;
    });

    auto result = funcObj.call((Int) 3);

    ASSERT_EQ((Int) result, 5);
}

TEST_F(CustomFunctionTest, NullFuncAssignedTrue)
{
    ObjectPtr<IFunction> nullFunc = CustomFunction(nullptr);

    ASSERT_TRUE(nullFunc.assigned());
}

TEST_F(CustomFunctionTest, NullFuncExecuteErrorCode)
{
    auto nullFunc = CustomFunction(nullptr);

    ObjectPtr<IBaseObject> result;
    ErrCode err = nullFunc->call(nullptr, &result);
    ASSERT_EQ(err, OPENDAQ_ERR_NOTASSIGNED);
}

TEST_F(CustomFunctionTest, NullFuncExecuteThrows)
{
    auto nullFunc = CustomFunction(nullptr);

    ASSERT_ANY_THROW(nullFunc.call());
}

TEST_F(CustomFunctionTest, OneStandardParams)
{
    auto funcObj = CustomFunction([](IBaseObject* params, IBaseObject** result)
    {
        Int a = BaseObjectPtr(params);
        *result = Integer_Create(a + 2);

        return a == 3
            ? OPENDAQ_SUCCESS
            :  OPENDAQ_ERR_INVALIDPARAMETER;
    });

    auto result = funcObj.call((Int) 3);

    ASSERT_EQ((Int) result, 5);
}

TEST_F(CustomFunctionTest, TwoStandardParams)
{
    auto funcObj = CustomFunction([](IBaseObject* params, IBaseObject** result)
    {
        auto list = ListObjectPtr<IList, IBaseObject>(params);

        Int a = list.getItemAt(0);
        Int b = list.getItemAt(1);
        *result = Integer_Create(a + b);

        return a == 3 && b == 2
                   ? OPENDAQ_SUCCESS
                   :  OPENDAQ_ERR_INVALIDPARAMETER;
    });

    auto result = funcObj.call((Int) 3, Int(2));

    ASSERT_EQ((Int) result, 5);
}

TEST_F(CustomFunctionTest, NoStandardParams)
{
    auto funcObj = CustomFunction([](IBaseObject* params, IBaseObject**)
    {
        return params == nullptr
                   ? OPENDAQ_SUCCESS
                   :  OPENDAQ_ERR_INVALIDPARAMETER;
    });

    auto result = funcObj.call();
    ASSERT_FALSE(result.assigned());
}

TEST_F(CustomFunctionTest, CoreType)
{
    auto funcObj = CustomFunction(nullptr);
    enum CoreType coreType = funcObj.getCoreType();

    ASSERT_EQ(coreType, ctFunc);
}

TEST_F(CustomFunctionTest, Inspectable)
{
    auto obj = CustomFunction(nullptr);

    auto ids = obj.asPtr<IInspectable>(true).getInterfaceIds();
    ASSERT_EQ(ids[0], IFunction::Id);
}

TEST_F(CustomFunctionTest, ImplementationName)
{
    auto obj = CustomFunction(nullptr);
    std::string className = obj.asPtr<IInspectable>(true).getRuntimeClassName();

    auto prefix = className.find("daq::CustomFunctionImpl<");
    ASSERT_EQ(prefix, 0u);
}
