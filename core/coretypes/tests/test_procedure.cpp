#include <gtest/gtest.h>
#include <coretypes/procedure_factory.h>
#include <coretypes/baseobject_factory.h>
#include <coretypes/listptr.h>
#include <coretypes/inspectable_ptr.h>

#include <memory>

using namespace daq;

using ProcedureTest = testing::Test;

TEST_F(ProcedureTest, Basic)
{
    auto proc = Procedure([](IBaseObject*) { return OPENDAQ_SUCCESS; });
    ErrCode err = proc->dispatch(nullptr);
    ASSERT_EQ(err, OPENDAQ_SUCCESS);
}

TEST_F(ProcedureTest, SmartPtrCallOperator)
{
    auto proc = Procedure([](IBaseObject*) { return OPENDAQ_SUCCESS; });
    ASSERT_NO_THROW(proc());
    ASSERT_NO_THROW(proc(BaseObjectPtr(nullptr)));
}

TEST_F(ProcedureTest, BaseObjectPtr)
{
    BaseObjectPtr proc = Procedure([](IBaseObject*) { return OPENDAQ_SUCCESS; });
    ASSERT_NO_THROW(proc.dispatch());
    ASSERT_NO_THROW(proc.dispatch(BaseObjectPtr(nullptr)));
}

TEST_F(ProcedureTest, Params)
{
    auto proc = Procedure([](IBaseObject* obj)
    {
        Int a = BaseObjectPtr::Borrow(obj);
        return a == 3
            ? OPENDAQ_SUCCESS
            :  OPENDAQ_ERR_INVALIDPARAMETER;
    });

    ASSERT_NO_THROW(proc.dispatch((Int) 3));
    ASSERT_ANY_THROW(proc.dispatch((Int) 2));
}

TEST_F(ProcedureTest, ParamsCallOperator)
{
    auto proc = Procedure([](IBaseObject* obj)
    {
        Int a = BaseObjectPtr::Borrow(obj);
        return a == 3
            ? OPENDAQ_SUCCESS
            :  OPENDAQ_ERR_INVALIDPARAMETER;
    });
    ASSERT_NO_THROW(proc((Int) 3));
    ASSERT_ANY_THROW(proc((Int) 2));
}

TEST_F(ProcedureTest, StandardParams)
{
    auto proc = Procedure([](IBaseObject* obj)
    {
        Int a = BaseObjectPtr::Borrow(obj);
        return a == 3
            ? OPENDAQ_SUCCESS
            :  OPENDAQ_ERR_INVALIDPARAMETER;
    });

    ASSERT_NO_THROW(proc.execute((Int) 3));
    ASSERT_ANY_THROW(proc.execute((Int) 2));
}

TEST_F(ProcedureTest, NullFuncAssignedTrue)
{
    ObjectPtr<IProcedure> nullFunc = Procedure(nullptr);

    ASSERT_TRUE(nullFunc.assigned());
}

TEST_F(ProcedureTest, TwoStandardParams)
{
    auto proc = Procedure([](IBaseObject* params)
    {
        auto list = ListObjectPtr<IList, IBaseObject>(params);

        Int a = list.getItemAt(0);
        Int b = list.getItemAt(1);
        return a == 3 && b == 2 
                   ? OPENDAQ_SUCCESS
                   :  OPENDAQ_ERR_INVALIDPARAMETER;
    });

    ASSERT_NO_THROW(proc.execute((Int) 3, Int(2)));
    ASSERT_ANY_THROW(proc.execute((Int) 2));

    ASSERT_NO_THROW(proc((Int) 3, (Int) 2));
}

TEST_F(ProcedureTest, NoStandardParams)
{
    auto proc = Procedure([](IBaseObject* params)
    {
        return params == nullptr
                   ? OPENDAQ_SUCCESS
                   :  OPENDAQ_ERR_INVALIDPARAMETER;
    });

    ASSERT_NO_THROW(proc.execute());
}

TEST_F(ProcedureTest, CoreType)
{
    auto proc = Procedure([]() { });
    enum CoreType coreType = proc.getCoreType();

    ASSERT_EQ(coreType, ctProc);
}

TEST_F(ProcedureTest, CoreTypeNull)
{
    auto proc = Procedure(nullptr);
    enum CoreType coreType = proc.getCoreType();

    ASSERT_EQ(coreType, ctProc);
}

TEST_F(ProcedureTest, AutoUnpackNoArgsErrCode)
{
    bool called = false;
    auto proc = Procedure([&called]()
    {
        called = true;
        return OPENDAQ_SUCCESS;
    });
    ASSERT_NO_THROW(proc());
    ASSERT_TRUE(called);
}

TEST_F(ProcedureTest, AutoUnpackNoArgsErrCodeException)
{
    bool called = false;
    auto proc = Procedure([&called]()
    {
        called = true;
        return OPENDAQ_ERR_GENERALERROR;
    });

    ASSERT_ANY_THROW(proc());
}

TEST_F(ProcedureTest, AutoUnpackNoArgs)
{
    bool called = false;
    auto proc = Procedure([&called]() { called = true; });
    ASSERT_NO_THROW(proc());
    ASSERT_TRUE(called);
}

TEST_F(ProcedureTest, AutoUnpackOneArg)
{
    Int res = 0;
    auto proc = Procedure([&res](Int r) { res = r; });
    ASSERT_NO_THROW(proc(3));
    ASSERT_EQ(res, 3);
}

TEST_F(ProcedureTest, AutoUnpackTwoArg)
{
    Int res1 = 0;
    Float res2 = 0.0;
    auto proc = Procedure([&res1, &res2](Int r1, Float r2)
    {
        res1 = r1;
        res2 = r2;
    });
    ASSERT_NO_THROW(proc(3, 2.0));
    ASSERT_EQ(res1, 3);
    ASSERT_DOUBLE_EQ(res2, 2.0);
}

TEST_F(ProcedureTest, AutoUnpackThrowException)
{
    bool called = false;
    auto proc = Procedure([&called]()
    {
        called = true;
        throw InvalidStateException();
    });
    ASSERT_THROW(proc(), InvalidStateException);
    ASSERT_TRUE(called);
}

TEST_F(ProcedureTest, AutoUnpackThrowExceptionCheckCode)
{
    bool called = false;
    auto proc = Procedure(
        [&called]()
        {
            called = true;
            throw InvalidStateException();
        });
    auto errCode = proc->dispatch(nullptr);
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
};

TEST_F(ProcedureTest, Bind)
{
    auto testBind = std::make_shared<TestBind>();
    auto handler = std::bind(&TestBind::test, testBind, std::placeholders::_1);

    auto proc = Procedure(handler);
    ASSERT_NO_THROW(proc(nullptr));
    ASSERT_TRUE(testBind->called);
}

static auto takesProcedurePtr(ProcedurePtr proc)
{
    return proc();
}

static int callCount{0};
static ErrCode rawFunction(IBaseObject* /*params*/)
{
    callCount++;
    return OPENDAQ_SUCCESS;
}

TEST_F(ProcedureTest, ImplicitConvertFreeFunctionPtr)
{
    callCount = 0;
    ASSERT_NO_THROW(takesProcedurePtr(rawFunction));
    ASSERT_EQ(callCount, 1);
}

TEST_F(ProcedureTest, ImplicitConvertLambda)
{
    bool called = false;
    ASSERT_NO_THROW(takesProcedurePtr([&called]() { called = true; }));
    ASSERT_TRUE(called);
}

TEST_F(ProcedureTest, ImplicitConvertRawLambda)
{
    bool called = false;
    ASSERT_NO_THROW(takesProcedurePtr([&called](IBaseObject* /*params*/)
    {
        called = true;
        return OPENDAQ_SUCCESS;
    }));

    ASSERT_TRUE(called);
}

TEST_F(ProcedureTest, ImplicitSmartPtrLambdaRaw)
{
    ProcedurePtr ptr = [](IBaseObject* /*params*/)
    {
        return OPENDAQ_SUCCESS;
    };
    ASSERT_NO_THROW(ptr());
}

TEST_F(ProcedureTest, ImplicitSmartPtrLambdaRawThrow)
{
    ProcedurePtr ptr = [](IBaseObject* /*params*/)
    {
        return OPENDAQ_ERR_GENERALERROR;
    };
    ASSERT_THROW(ptr(), GeneralErrorException);
    ASSERT_EQ(ptr->dispatch(nullptr), OPENDAQ_ERR_GENERALERROR);
}

TEST_F(ProcedureTest, ImplicitSmartPtrLambdaZeroParam)
{
    Int value{0};
    ProcedurePtr ptr = [&value]()
    {
        value = 5;
    };
    ASSERT_NO_THROW(ptr());
    ASSERT_EQ(value, 5);

    value = 0;
    ASSERT_EQ(ptr->dispatch(nullptr), OPENDAQ_SUCCESS);
    ASSERT_EQ(value, 5);
}

TEST_F(ProcedureTest, ImplicitSmartPtrLambdaZeroParamThrow)
{
    bool called = false;
    ProcedurePtr ptr = [&called]()
    {
        called = true;
        throw GeneralErrorException();
    };
    ASSERT_THROW(ptr(), GeneralErrorException);
    ASSERT_TRUE(called);

    called = false;
    ASSERT_EQ(ptr->dispatch(nullptr), OPENDAQ_ERR_GENERALERROR);
    ASSERT_TRUE(called);
}

TEST_F(ProcedureTest, ImplicitSmartPtrLambdaOneParam)
{
    Int value{0};
    ProcedurePtr ptr = [&value](Int a)
    {
        value = a;
    };
    ASSERT_NO_THROW(ptr(5));
    ASSERT_EQ(value, 5);

    value = 0;
    ObjectPtr<IInteger> intA = Integer_Create(5);
    ASSERT_EQ(ptr->dispatch(intA), OPENDAQ_SUCCESS);
    ASSERT_EQ(value, 5);
}

TEST_F(ProcedureTest, ImplicitSmartPtrLambdaOneParamThrow)
{
    ProcedurePtr ptr = [](Int /*param*/) -> Int
    {
        throw GeneralErrorException();
    };
    ASSERT_THROW(ptr(5), GeneralErrorException);

    ObjectPtr<IInteger> intA = Integer_Create(5);
    ASSERT_EQ(ptr->dispatch(intA), OPENDAQ_ERR_GENERALERROR);
}

TEST_F(ProcedureTest, ImplicitSmartPtrLambdaTwoParam)
{
    Int value{0};
    ProcedurePtr ptr = [&value](Int a, Int b)
    {
        value = a + b;
    };
    ASSERT_NO_THROW(ptr(3, 2));
    ASSERT_EQ(value, 3 + 2);

    value = 0;
    auto params = List<Int>(5, 3);
    ASSERT_EQ(ptr->dispatch(params), OPENDAQ_SUCCESS);
    ASSERT_EQ(value, 5 + 3);
}

TEST_F(ProcedureTest, ImplicitSmartPtrLambdaTwoThrow)
{
    ProcedurePtr ptr = [](Int /*a*/, Int /*b*/) -> Int
    {
        throw GeneralErrorException();
    };
    ASSERT_THROW(ptr(5, 3), GeneralErrorException);

    auto params = List<Int>(5, 3);
    ASSERT_EQ(ptr->dispatch(params), OPENDAQ_ERR_GENERALERROR);
}

static Int globalValue{0};

static ErrCode testingRaw(IBaseObject* /*params*/)
{
    return OPENDAQ_SUCCESS;
}

static ErrCode testingRawE(IBaseObject* /*params*/)
{
    return OPENDAQ_ERR_GENERALERROR;
}

static void testing0()
{
    globalValue = 5;
}

static ErrCode testing0E()
{
    return OPENDAQ_ERR_GENERALERROR;
}

static void testing1(Int val)
{
    globalValue = val;
}

static ErrCode testing1E(Int /*val*/)
{
    return OPENDAQ_ERR_GENERALERROR;
}

static void testing2(Int val, Int val2)
{
    globalValue = val + val2;
}

static void testing2E(Int /*val*/, Int /*val2*/)
{
    throwExceptionFromErrorCode(OPENDAQ_ERR_GENERALERROR);
}

TEST_F(ProcedureTest, Test)
{
    auto free1 = Procedure(testing1);
    free1(5);

    auto free1E = Procedure(testing1E);
    ASSERT_ANY_THROW(free1E(5));

    auto free2 = Procedure(testing2);
    free2(5, 10);

    auto free0 = Procedure(testing0);
    free0();

    auto free0E = Procedure(testing0E);

    ASSERT_ANY_THROW(free0E());

    // lambdas

    auto lambda0 = Procedure([]() { ASSERT_EQ(5, 5); });
    lambda0();

    auto lambda1 = Procedure([](int /*inVal*/) { throw OutOfRangeException(); });

    ASSERT_ANY_THROW(lambda1(5));

    auto lambda2 = Procedure([](int /*inVal*/, int /*inVal2*/) {});
    lambda2(5, 10);

    auto lambda3E = Procedure([](int /*inVal*/, int /*inVal2*/, int /*inVal3*/) { throw GeneralErrorException("test"); });

    ASSERT_ANY_THROW(lambda3E(1, 2, 3));

    auto lambda1E = Procedure([](int /*inVal*/) { throw GeneralErrorException("test"); });

    ASSERT_ANY_THROW(lambda1E(1));

    // bind

    auto testBind = std::make_shared<TestBind>();
    auto handler = std::bind(&TestBind::test, testBind, std::placeholders::_1);

    auto bind1 = Procedure(handler);
    bind1(nullptr);
}

TEST_F(ProcedureTest, ImplicitSmartPtrFreeFuncRaw)
{
    ProcedurePtr ptr = testingRaw;
    ASSERT_NO_THROW(ptr());
}

TEST_F(ProcedureTest, ImplicitSmartPtrFreeFuncRawThrow)
{
    ProcedurePtr ptr = testingRawE;
    ASSERT_THROW(ptr(), GeneralErrorException);
    ASSERT_EQ(ptr->dispatch(nullptr), OPENDAQ_ERR_GENERALERROR);
}

TEST_F(ProcedureTest, ImplicitSmartPtrFreeFuncZeroParam)
{
    globalValue = 0;

    ProcedurePtr ptr = testing0;
    ASSERT_NO_THROW(ptr());
    ASSERT_EQ(globalValue, 5);

    globalValue = 0;
    ASSERT_EQ(ptr->dispatch(nullptr), OPENDAQ_SUCCESS);
    ASSERT_EQ(globalValue, 5);
}

TEST_F(ProcedureTest, ImplicitSmartPtrFreeFuncZeroParamThrow)
{
    ProcedurePtr ptr = testing0E;
    ASSERT_THROW(ptr(), GeneralErrorException);

    ASSERT_EQ(ptr->dispatch(nullptr), OPENDAQ_ERR_GENERALERROR);
}

TEST_F(ProcedureTest, ImplicitSmartPtrFreeFuncOneParam)
{
    globalValue = 0;

    ProcedurePtr ptr = testing1;
    ASSERT_NO_THROW(ptr(5));
    ASSERT_EQ(globalValue, 5);

    globalValue = 0;
    ObjectPtr<IInteger> intA = Integer_Create(5);
    ASSERT_EQ(ptr->dispatch(intA), OPENDAQ_SUCCESS);
    ASSERT_EQ(globalValue, 5);
}

TEST_F(ProcedureTest, ImplicitSmartPtrFreeFuncOneParamThrow)
{
    ProcedurePtr ptr = testing1E;
    ASSERT_THROW(ptr(5), GeneralErrorException);

    ObjectPtr<IInteger> intA = Integer_Create(5);
    ASSERT_EQ(ptr->dispatch(intA), OPENDAQ_ERR_GENERALERROR);
}

TEST_F(ProcedureTest, ImplicitSmartPtrFreeFuncTwoParam)
{
    globalValue = 0;

    ProcedurePtr ptr = testing2;
    ASSERT_NO_THROW(ptr(3, 2));
    ASSERT_EQ(globalValue, 3 + 2);

    globalValue = 0;
    auto params = List<Int>(5, 3);
    ASSERT_EQ(ptr->dispatch(params), OPENDAQ_SUCCESS);
    ASSERT_EQ(globalValue, 5 + 3);
}

TEST_F(ProcedureTest, ImplicitSmartPtrFreeFuncTwoThrow)
{
    ProcedurePtr ptr = testing2E;
    ASSERT_THROW(ptr(5, 3), GeneralErrorException);

    auto params = List<Int>(5, 3);
    ASSERT_EQ(ptr->dispatch(params), OPENDAQ_ERR_GENERALERROR);
}

TEST_F(ProcedureTest, Inspectable)
{
    ProcedurePtr obj = testing2E;

    auto ids = obj.asPtr<IInspectable>(true).getInterfaceIds();
    ASSERT_EQ(ids[0], IProcedure::Id);
}

TEST_F(ProcedureTest, ImplementationName)
{
    ProcedurePtr obj = testing2E;

    std::string className = obj.asPtr<IInspectable>(true).getRuntimeClassName();
    auto prefix = className.find("daq::ProcedureImpl<");
    ASSERT_EQ(prefix, 0u);
}

static constexpr auto INTERFACE_ID = FromTemplatedTypeName("IProcedure", "daq");

TEST_F(ProcedureTest, InterfaceId)
{
    ASSERT_EQ(INTERFACE_ID, IProcedure::Id);
}

TEST_F(ProcedureTest, InterfaceIdString)
{
    ASSERT_EQ(daqInterfaceIdString<IProcedure>(), "{36247E6D-6BDD-5964-857D-0FD296EEB5C3}");
}
