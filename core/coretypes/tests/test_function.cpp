#include <gtest/gtest.h>
#include <coretypes/string_ptr.h>
#include <coretypes/function_factory.h>
#include <coretypes/list_factory.h>
#include <coretypes/baseobject_factory.h>
#include <coretypes/inspectable_ptr.h>
#include <functional>

using namespace daq;

using FunctionTest = testing::Test;

TEST_F(FunctionTest, Basic)
{
    auto funcObj = Function([]()
    {
        return nullptr;
    });

    ErrCode err = funcObj->call(nullptr, nullptr);
    ASSERT_EQ(err, OPENDAQ_ERR_ARGUMENT_NULL);

    IBaseObject* result;
    err = funcObj->call(nullptr, &result);
    ASSERT_EQ(err, OPENDAQ_SUCCESS);
}

static Int test()
{
    return 5;
}

TEST_F(FunctionTest, IntFunc)
{
    auto func = Function(test);

    auto result = func();
    ASSERT_EQ(result, 5);
}

TEST_F(FunctionTest, BasicSumFunc)
{
    auto funcObj = Function([](ListPtr<IInteger> list)
    {
        Int a = list.getItemAt(0);
        Int b = list.getItemAt(1);

        return a + b;
    });

    auto list = List<Int>(5, 4);

    ObjectPtr<IBaseObject> result;
    ErrCode err = funcObj->call(list, &result);
    ASSERT_EQ(err, OPENDAQ_SUCCESS);

    ASSERT_EQ((Int)result, list[0] + list[1]);
}

TEST_F(FunctionTest, SmartPtr)
{
    auto funcObj = Function([]() {
        return nullptr;
    });

    auto result = funcObj.call();
    ASSERT_FALSE(result.assigned());

    result = funcObj.call(BaseObjectPtr(nullptr));
    ASSERT_FALSE(result.assigned());
}

TEST_F(FunctionTest, BaseObjectPtr)
{
    BaseObjectPtr funcObj = Function([]() {
        return nullptr;
    });

    auto result = funcObj.call();
    ASSERT_FALSE(result.assigned());

    result = funcObj.call(BaseObjectPtr(nullptr));
    ASSERT_FALSE(result.assigned());
}

TEST_F(FunctionTest, NullFuncAssignedTrue)
{
    ObjectPtr<IFunction> nullFunc = Function(nullptr);

    ASSERT_TRUE(nullFunc.assigned());
}

TEST_F(FunctionTest, NullFuncExecuteErrorCode)
{
    auto nullFunc = Function(nullptr);

    ObjectPtr<IBaseObject> result;
    ErrCode err = nullFunc->call(nullptr, &result);
    ASSERT_EQ(err, OPENDAQ_ERR_NOTASSIGNED);
}

TEST_F(FunctionTest, NullFuncExecuteThrows)
{
    auto nullFunc = Function(nullptr);

    ASSERT_ANY_THROW(nullFunc.call());
}

TEST_F(FunctionTest, OneStandardParams)
{
    auto funcObj = Function([](Int a)
    {
        return a + 2;
    });

    auto result = funcObj.call((Int) 3);
    ASSERT_EQ((Int) result, 5);
}

TEST_F(FunctionTest, TwoStandardParams)
{
    auto funcObj = Function([](Int a, Int b)
    {
        return a + b;
    });

    auto result = funcObj.call((Int) 3, Int(2));
    ASSERT_EQ((Int) result, 5);
}

TEST_F(FunctionTest, ThreeStandardParams)
{
    auto funcObj = Function([](Int a, Int b, Int c)
    {
        return a + b + c;
    });

    auto result = funcObj.call(3, 2, 1);
    ASSERT_EQ((Int) result, 6);
}

TEST_F(FunctionTest, CoreType)
{
    auto funcObj = Function(nullptr);
    enum CoreType coreType = funcObj.getCoreType();

    ASSERT_EQ(coreType, ctFunc);
}

TEST_F(FunctionTest, RawSignature)
{
    auto funcObj = Function([](IBaseObject* /*params*/, IBaseObject** /*result*/)
    {
        return OPENDAQ_SUCCESS;
    });
}

TEST_F(FunctionTest, AutoUnpackNoArgsErrCode)
{
    bool called = false;
    auto func = Function([&called]()
    {
        called = true;
        return false;
    });

    auto result = func();
    ASSERT_FALSE(result);
    ASSERT_TRUE(called);    
}

TEST_F(FunctionTest, AutoUnpackNoArgsErrCodeException)
{
    bool called = false;
    auto func = Function([&called]() -> bool
    {
        called = true;
        throwExceptionFromErrorCode(OPENDAQ_ERR_GENERALERROR);
    });

    ASSERT_ANY_THROW(func());
    ASSERT_TRUE(called);
}

TEST_F(FunctionTest, AutoUnpackNoArgs)
{
    bool called = false;
    auto func = Function([&called]()
    {
        called = true;
        return false;
    });

    auto result = func();
    ASSERT_FALSE(result);
    ASSERT_TRUE(called);
}

TEST_F(FunctionTest, AutoUnpackOneArg)
{
    auto func = Function([](Int r)
    {
        return r;
    });

    auto res = func(3);
    ASSERT_EQ(res, 3);
}

TEST_F(FunctionTest, AutoUnpackTwoArg)
{
    Int res1 = 0;
    Float res2 = 0.0;
    auto func = Function([&res1, &res2](Int r1, Float r2)
    {
        res1 = r1;
        res2 = r2;

        return r1 + r2;
    });

    auto result = func(3, 2.0);
    ASSERT_EQ(result, 3 + 2.0);

    ASSERT_EQ(res1, 3);
    ASSERT_DOUBLE_EQ(res2, 2.0);
}

TEST_F(FunctionTest, AutoUnpackThrowException)
{
    bool called = false;
    auto func = Function([&called]() -> bool
    {
        called = true;
        throw InvalidStateException();
    });

    ASSERT_THROW(func(), InvalidStateException);
    ASSERT_TRUE(called);
}

TEST_F(FunctionTest, AutoUnpackThrowExceptionCheckCode)
{
    bool called = false;
    auto func = Function([&called]() -> bool
    {
        called = true;
        throw InvalidStateException();
    });

    BaseObjectPtr result;
    auto errCode = func->call(nullptr, &result);
    ASSERT_EQ(errCode, OPENDAQ_ERR_INVALIDSTATE);
    ASSERT_TRUE(called);
}

class TestBind
{
public:
    bool called = false;

    ErrCode test(IBaseObject* /*obj*/)
    {
        called = true;
        return OPENDAQ_SUCCESS;
    }

    BaseObjectPtr createNull() const
    {
        return nullptr;
    }

    BaseObjectPtr createFromType(StringPtr type)
    {
        if (type == "test1" || type == "test2")
        {
            return BaseObject();
        }
        return nullptr;
    }
};

TEST_F(FunctionTest, BindWrapped)
{
    using namespace std::placeholders;

    auto createGroupNull = Function(std::bind(&::TestBind::createNull, this));
    auto createGroup = Function(std::bind(&::TestBind::createFromType, this, _1));
}

TEST_F(FunctionTest, Bind)
{
    auto testBind = std::make_shared<TestBind>();
    auto handler = std::bind(&TestBind::test, testBind, std::placeholders::_1);

    auto func = Function(handler);
    ASSERT_NO_THROW(func(nullptr));
    ASSERT_TRUE(testBind->called);
}

static auto takesFunctionPtr(FunctionPtr func)
{
    return func();
}

static ErrCode rawFunction(IBaseObject* /*params*/, IBaseObject** result)
{
    *result = Integer_Create(5);
    return OPENDAQ_SUCCESS;
}

TEST_F(FunctionTest, ImplicitConvertFreeFunctionPtr)
{
    auto funcResult = takesFunctionPtr(rawFunction);
    ASSERT_EQ(funcResult, 5);
}

TEST_F(FunctionTest, ImplicitConvertLambda)
{
    auto funcResult = takesFunctionPtr([]()
    {
        return 5;
    });
    ASSERT_EQ(funcResult, 5);
}

TEST_F(FunctionTest, ImplicitConvertRawLambda)
{
    auto funcResult = takesFunctionPtr([](IBaseObject* /*params*/, IBaseObject** result)
    {
        *result = Integer_Create(5);
        return OPENDAQ_SUCCESS;
    });
    ASSERT_EQ(funcResult, 5);
}

TEST_F(FunctionTest, ImplicitSmartPtrLambdaRaw)
{
    FunctionPtr ptr = [](IBaseObject* /*params*/, IBaseObject** /*result*/)
    {
        return OPENDAQ_SUCCESS;
    };
    ASSERT_NO_THROW(ptr());

    BaseObjectPtr result;
    ASSERT_EQ(ptr->call(nullptr, &result), OPENDAQ_SUCCESS);
}

TEST_F(FunctionTest, ImplicitSmartPtrLambdaRawThrow)
{
    FunctionPtr ptr = [](IBaseObject* /*params*/, IBaseObject** /*result*/)
    {
        return OPENDAQ_ERR_GENERALERROR;
    };
    ASSERT_THROW(ptr(), GeneralErrorException);

    BaseObjectPtr result;
    ASSERT_EQ(ptr->call(nullptr, &result), OPENDAQ_ERR_GENERALERROR);
}

TEST_F(FunctionTest, ImplicitSmartPtrLambdaZeroParam)
{
    FunctionPtr ptr = []()
    {
        return 5;
    };
    ASSERT_EQ(ptr(), 5);

    BaseObjectPtr result;
    ASSERT_EQ(ptr->call(nullptr, &result), OPENDAQ_SUCCESS);
    ASSERT_EQ(result, 5);
}

TEST_F(FunctionTest, ImplicitSmartPtrLambdaZeroParamThrow)
{
    bool called = false;
    FunctionPtr ptr = [&called]() -> Int
    {
        called = true;
        throw GeneralErrorException();
    };
    ASSERT_THROW(ptr(), GeneralErrorException);
    ASSERT_TRUE(called);

    called = false;
    BaseObjectPtr result;
    ASSERT_EQ(ptr->call(nullptr, &result), OPENDAQ_ERR_GENERALERROR);
    ASSERT_TRUE(called);
}

TEST_F(FunctionTest, ImplicitSmartPtrLambdaOneParam)
{
    FunctionPtr ptr = [](Int param)
    {
        return param;
    };
    ASSERT_EQ(ptr(5), 5);

    BaseObjectPtr result;
    ObjectPtr<IInteger> intA = Integer_Create(5);
    ASSERT_EQ(ptr->call(intA, &result), OPENDAQ_SUCCESS);
    ASSERT_EQ(result, 5);
}

TEST_F(FunctionTest, ImplicitSmartPtrLambdaOneParamThrow)
{
    FunctionPtr ptr = [](Int /*param*/) -> Int
    {
        throw GeneralErrorException();
    };
    ASSERT_THROW(ptr(5), GeneralErrorException);

    BaseObjectPtr result;
    ObjectPtr<IInteger> intA = Integer_Create(5);
    ASSERT_EQ(ptr->call(intA, &result), OPENDAQ_ERR_GENERALERROR);
}

TEST_F(FunctionTest, ImplicitSmartPtrLambdaTwoParam)
{
    FunctionPtr ptr = [](Int a, Int b)
    {
        return a + b;
    };
    ASSERT_EQ(ptr(3, 2), 3 + 2);

    BaseObjectPtr result;
    auto params = List<Int>(5, 3);
    ASSERT_EQ(ptr->call(params, &result), OPENDAQ_SUCCESS);
    ASSERT_EQ(result, 5 + 3);
}

TEST_F(FunctionTest, ImplicitSmartPtrLambdaTwoThrow)
{
    FunctionPtr ptr = [](Int /*a*/, Int /*b*/) -> Int
    {
        throw GeneralErrorException();
    };
    ASSERT_THROW(ptr(5, 3), GeneralErrorException);

    BaseObjectPtr result;
    auto params = List<Int>(5, 3);
    ASSERT_EQ(ptr->call(params, &result), OPENDAQ_ERR_GENERALERROR);
}

static Int testing0()
{
    return -1;
}

static ErrCode testingRaw(IBaseObject* /*params*/, IBaseObject** /*result*/)
{
    return OPENDAQ_SUCCESS;
}

static ErrCode testingRawE(IBaseObject* /*params*/, IBaseObject** /*result*/)
{
    return OPENDAQ_ERR_GENERALERROR;
}

static std::string testing0E()
{
    throwExceptionFromErrorCode(OPENDAQ_ERR_GENERALERROR);
}

static Int testing1(Int val)
{
    return val;
}

static Float testing1E(Int /*val*/)
{
    throwExceptionFromErrorCode(OPENDAQ_ERR_GENERALERROR);
}

static Int testing2(Int val, Int val2)
{
    return val + val2;
}

static Int testing2E(Int /*val*/, Int /*val2*/)
{
    throwExceptionFromErrorCode(OPENDAQ_ERR_GENERALERROR);
}

[[maybe_unused]] static void VoidFunc()
{
}

//TEST_F(FunctionTest, VoidFunction)
//{
//    // Should not compile
//    auto func = Function(VoidFunc);
//    FAIL();
//}

//TEST_F(FunctionTest, VoidLambda)
//{
//    // Should not compile
//    auto func = Function([](){ });
//    FAIL();
//}

TEST_F(FunctionTest, Test)
{
    auto free1 = Function(testing1);
    ASSERT_EQ(free1(5), 5);

    auto free1E = Function(testing1E);
    ASSERT_ANY_THROW(free1E(5));

    auto free2 = Function(testing2);
    ASSERT_EQ(free2(5, 10), 5 + 10);

    auto free0 = Function(testing0);
    ASSERT_EQ(free0(), -1);

    auto free0E = Function(testing0E);
    ASSERT_ANY_THROW(free0E());

    // lambdas

    auto lambda0 = Function([]() { return true; });
    ASSERT_TRUE(lambda0());

    auto lambda1 = Function([](int /*inVal*/) -> bool { throw OutOfRangeException(); });
    ASSERT_ANY_THROW(lambda1(5));

    auto lambda2 = Function([](int inVal, int inVal2) {
        return inVal + inVal2;
    });
    ASSERT_EQ(lambda2(5, 10), 5 + 10);

    auto lambda3E = Function([](int /*inVal*/, int /*inVal2*/, int /*inVal3*/) -> int { throw GeneralErrorException("test"); });
    ASSERT_ANY_THROW(lambda3E(1, 2, 3));

    auto lambda1E = Function([](int /*inVal*/) -> int { throw GeneralErrorException("test"); });
    ASSERT_ANY_THROW(lambda1E(1));

    // bind

    auto testBind = std::make_shared<TestBind>();
    auto handler = std::bind(&TestBind::test, testBind, std::placeholders::_1);

    auto bind1 = Function(handler);
    bind1(nullptr);
}

////

TEST_F(FunctionTest, ImplicitSmartPtrFreeFuncRaw)
{
    FunctionPtr ptr = testingRaw;
    ASSERT_NO_THROW(ptr());

    BaseObjectPtr result;
    ASSERT_EQ(ptr->call(nullptr, &result), OPENDAQ_SUCCESS);
}

TEST_F(FunctionTest, ImplicitSmartPtrFreeFuncRawThrow)
{
    FunctionPtr ptr = testingRawE;
    ASSERT_THROW(ptr(), GeneralErrorException);

    BaseObjectPtr result;
    ASSERT_EQ(ptr->call(nullptr, &result), OPENDAQ_ERR_GENERALERROR);
}

TEST_F(FunctionTest, ImplicitSmartPtrFreeFuncOneParam)
{
    FunctionPtr ptr = testing1;
    ASSERT_EQ(ptr(5), 5);

    BaseObjectPtr result;
    ObjectPtr<IInteger> intA = Integer_Create(5);
    ASSERT_EQ(ptr->call(intA, &result), OPENDAQ_SUCCESS);
    ASSERT_EQ(result, 5);
}

TEST_F(FunctionTest, ImplicitSmartPtrFreeFuncOneParamThrow)
{
    FunctionPtr ptr = testing1E;
    ASSERT_THROW(ptr(5), GeneralErrorException);

    BaseObjectPtr result;
    ObjectPtr<IInteger> intA = Integer_Create(5);
    ASSERT_EQ(ptr->call(intA, &result), OPENDAQ_ERR_GENERALERROR);
}

TEST_F(FunctionTest, ImplicitSmartPtrFreeFuncTwoParam)
{
    FunctionPtr ptr = testing2;
    ASSERT_EQ(ptr(3, 2), 3 + 2);

    BaseObjectPtr result;
    auto params = List<Int>(5, 3);
    ASSERT_EQ(ptr->call(params, &result), OPENDAQ_SUCCESS);
    ASSERT_EQ(result, 5 + 3);
}

TEST_F(FunctionTest, ImplicitSmartPtrFreeFuncTwoThrow)
{
    FunctionPtr ptr = testing2E;
    ASSERT_THROW(ptr(5, 3), GeneralErrorException);

    BaseObjectPtr result;
    auto params = List<Int>(5, 3);
    ASSERT_EQ(ptr->call(params, &result), OPENDAQ_ERR_GENERALERROR);
}

TEST_F(FunctionTest, Inspectable)
{
    FunctionPtr obj = testing2E;

    auto ids = obj.asPtr<IInspectable>(true).getInterfaceIds();
    ASSERT_EQ(ids[0], IFunction::Id);
}

TEST_F(FunctionTest, ImplementationName)
{
    FunctionPtr obj = testing2E;
    std::string className = obj.asPtr<IInspectable>(true).getRuntimeClassName();

    auto prefix = className.find("daq::FunctionImpl<");
    ASSERT_EQ(prefix, 0u);
}
