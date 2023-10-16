#include <testutils/testutils.h>
#include <coretypes/callback.h>

using CallbackTest = testing::Test;

using namespace daq;

///////////////////////////
////  Procedure Lambda
//////////////////////////

TEST_F(CallbackTest, ProcedureLambdaZero)
{
    bool called = false;
    auto proc = Callback([&called](){ called = true; });
    ASSERT_TRUE((std::is_same_v<ProcedurePtr, decltype(proc)>));

    ASSERT_NO_THROW(proc());
    ASSERT_EQ(called, true);
}

TEST_F(CallbackTest, ProcedureLambdaOne)
{
    Int result{0};
    auto proc = Callback([&result](Int a){ result = a; });
    ASSERT_TRUE((std::is_same_v<ProcedurePtr, decltype(proc)>));

    ASSERT_NO_THROW(proc(1));
    ASSERT_EQ(result, 1);
}

TEST_F(CallbackTest, ProcedureLambdaTwo)
{
    Float result;
    auto proc = Callback([&result](Int a, Float b) { result = a + b; });
    ASSERT_TRUE((std::is_same_v<ProcedurePtr, decltype(proc)>));

    ASSERT_NO_THROW(proc(1, 2.5, false));
    ASSERT_EQ(result, 1 + 2.5);
}

TEST_F(CallbackTest, ProcedureLambdaThree)
{
    Float result{0};
    auto proc = Callback([&result](Int a, Float b, bool /*c*/) { result = Float(a) + b; });
    ASSERT_TRUE((std::is_same_v<ProcedurePtr, decltype(proc)>));

    ASSERT_NO_THROW(proc(1, 2.5, false));
    ASSERT_EQ(result, 1 + 2.5);
}

TEST_F(CallbackTest, ProcedureLambdaRaw)
{
    bool called{false};
    auto proc = Callback([&called](IBaseObject*)
    {
        called = true;
        return OPENDAQ_SUCCESS;
    });
    ASSERT_TRUE((std::is_same_v<ProcedurePtr, decltype(proc)>));

    ASSERT_NO_THROW(proc());
    ASSERT_TRUE(called);
}

///////////////////////////
////  Procedure Function
//////////////////////////

static Float globalValue{0};

static ErrCode testingPRaw(IBaseObject* /*params*/)
{
    return OPENDAQ_SUCCESS;
}

[[maybe_unused]] static ErrCode testingPRawE(IBaseObject* /*params*/)
{
    return OPENDAQ_ERR_GENERALERROR;
}

static void testingP0()
{
    globalValue = 5;
}

[[maybe_unused]] static ErrCode testingP0E()
{
    return OPENDAQ_ERR_GENERALERROR;
}

static void testingP1(Int val)
{
    globalValue = val;
}

[[maybe_unused]] static ErrCode testingP1E(Int /*val*/)
{
    return OPENDAQ_ERR_GENERALERROR;
}

static void testingP2(Int val, Float val2)
{
    globalValue = val + val2;
}

[[maybe_unused]] static void testingP2E(Int /*val*/, Int /*val2*/)
{
    throwExceptionFromErrorCode(OPENDAQ_ERR_GENERALERROR);
}

static void testingP3(Int val, Float val2, bool /*val3*/)
{
    globalValue = val + val2;
}

TEST_F(CallbackTest, ProcedureFunctionZero)
{
    auto proc = Callback(testingP0);
    ASSERT_TRUE((std::is_same_v<ProcedurePtr, decltype(proc)>));

    ASSERT_NO_THROW(proc());
}

TEST_F(CallbackTest, ProcedureFunctionOne)
{
    auto proc = Callback(testingP1);
    ASSERT_TRUE((std::is_same_v<ProcedurePtr, decltype(proc)>));

    ASSERT_NO_THROW(proc(1));
    ASSERT_EQ(globalValue, 1);
}

TEST_F(CallbackTest, ProcedureFunctionTwo)
{
    auto proc = Callback(testingP2);
    ASSERT_TRUE((std::is_same_v<ProcedurePtr, decltype(proc)>));

    ASSERT_NO_THROW(proc(1, 2.5));
    ASSERT_EQ(globalValue, 1 + 2.5);
}

TEST_F(CallbackTest, ProcedureFunctionThree)
{
    auto proc = Callback(testingP3);
    ASSERT_TRUE((std::is_same_v<ProcedurePtr, decltype(proc)>));

    ASSERT_NO_THROW(proc(1, 2.5, true));
    ASSERT_EQ(globalValue, 1 + 2.5);
}

TEST_F(CallbackTest, ProcedureFunctionRaw)
{
    auto proc = Callback(testingPRaw);
    ASSERT_TRUE((std::is_same_v<ProcedurePtr, decltype(proc)>));

    ASSERT_NO_THROW(proc());
}

//////////////////////////
////  Function Lambda
//////////////////////////

TEST_F(CallbackTest, FunctionLambdaZero)
{
    auto func = Callback([]() { return true; });
    ASSERT_TRUE((std::is_same_v<FunctionPtr, decltype(func)>));

    bool result;
    ASSERT_NO_THROW(result = func());
    ASSERT_EQ(result, true);
}

TEST_F(CallbackTest, FunctionLambdaOne)
{
    auto func = Callback([](Int a) { return a; });
    ASSERT_TRUE((std::is_same_v<FunctionPtr, decltype(func)>));

    Int result;
    ASSERT_NO_THROW(result = func(1));
    ASSERT_EQ(result, 1);
}

TEST_F(CallbackTest, FunctionLambdaTwo)
{
    auto func = Callback([](Int a, Float b) { return a + b; });
    ASSERT_TRUE((std::is_same_v<FunctionPtr, decltype(func)>));

    Float result;
    ASSERT_NO_THROW(result = func(1, 2.5, false));
    ASSERT_EQ(result, 1 + 2.5);
}

TEST_F(CallbackTest, FunctionLambdaThree)
{
    auto func = Callback([](Int a, Float b, bool /*c*/) { return a + b; });
    ASSERT_TRUE((std::is_same_v<FunctionPtr, decltype(func)>));

    Float result;
    ASSERT_NO_THROW(result = func(1, 2.5, false));
    ASSERT_EQ(result, 1 + 2.5);
}

TEST_F(CallbackTest, FunctionLambdaRaw)
{
    auto func = Callback([](IBaseObject* /*params*/, IBaseObject** /*result*/) { return OPENDAQ_SUCCESS;});
    ASSERT_TRUE((std::is_same_v<FunctionPtr, decltype(func)>));

    BaseObjectPtr result;
    ASSERT_NO_THROW(func());
    ASSERT_TRUE(!result.assigned());
}

//////////////////////////
////  Function pointer
//////////////////////////

static Int testingF0()
{
    return -1;
}

static ErrCode testingFRaw(IBaseObject* /*params*/, IBaseObject** /*result*/)
{
    return OPENDAQ_SUCCESS;
}

[[maybe_unused]] static ErrCode testingFRawE(IBaseObject* /*params*/, IBaseObject** /*result*/)
{
    return OPENDAQ_ERR_GENERALERROR;
}

[[maybe_unused]] static std::string testingF0E()
{
    throwExceptionFromErrorCode(OPENDAQ_ERR_GENERALERROR);
}

static Int testingF1(Int val)
{
    return val;
}

[[maybe_unused]] static Float testingF1E(Int /*val*/)
{
    throwExceptionFromErrorCode(OPENDAQ_ERR_GENERALERROR);
}

static Float testingF2(Int val, Float val2)
{
    return val + val2;
}

[[maybe_unused]] static Int testingF2E(Int /*val*/, Int /*val2*/)
{
    throwExceptionFromErrorCode(OPENDAQ_ERR_GENERALERROR);
}

static Float testingF3(Int val, Float val2, bool /*val3*/)
{
    return val + val2;
}

[[maybe_unused]] static Int testingF3E(Int /*val*/, Int /*val2*/, bool /*val3*/)
{
    throwExceptionFromErrorCode(OPENDAQ_ERR_GENERALERROR);
}

TEST_F(CallbackTest, FunctionPtrZero)
{
    auto func = Callback(testingF0);
    ASSERT_TRUE((std::is_same_v<FunctionPtr, decltype(func)>));

    bool result;
    ASSERT_NO_THROW(result = func());
    ASSERT_EQ(result, true);
}

TEST_F(CallbackTest, FunctionPtrOne)
{
    auto func = Callback(testingF1);
    ASSERT_TRUE((std::is_same_v<FunctionPtr, decltype(func)>));

    Int result;
    ASSERT_NO_THROW(result = func(1));
    ASSERT_EQ(result, 1);
}

TEST_F(CallbackTest, FunctionPtrTwo)
{
    auto func = Callback(testingF2);
    ASSERT_TRUE((std::is_same_v<FunctionPtr, decltype(func)>));

    Float result;
    ASSERT_NO_THROW(result = func(1, 2.5, false));
    ASSERT_EQ(result, 1 + 2.5);
}

TEST_F(CallbackTest, FunctionPtrThree)
{
    auto func = Callback(testingF3);
    ASSERT_TRUE((std::is_same_v<FunctionPtr, decltype(func)>));

    Float result;
    ASSERT_NO_THROW(result = func(1, 2.5, false));
    ASSERT_EQ(result, 1 + 2.5);
}

TEST_F(CallbackTest, FunctionPtrRaw)
{
    auto func = Callback(testingFRaw);
    ASSERT_TRUE((std::is_same_v<FunctionPtr, decltype(func)>));

    BaseObjectPtr result;
    ASSERT_NO_THROW(func());
    ASSERT_TRUE(!result.assigned());
}
