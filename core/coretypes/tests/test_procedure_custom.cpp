#include <gtest/gtest.h>
#include <coretypes/procedure_factory.h>
#include <coretypes/baseobject_factory.h>
#include <coretypes/inspectable_ptr.h>

using namespace daq;

using CustomProcedureTest = testing::Test;

TEST_F(CustomProcedureTest, Basic)
{
    auto procObj = CustomProcedure([](IBaseObject*)
    {
        return OPENDAQ_SUCCESS;
    });

    ErrCode err = procObj->dispatch(nullptr);
    ASSERT_EQ(err, OPENDAQ_SUCCESS);
}

TEST_F(CustomProcedureTest, SmartPtr)
{
    auto procObj = CustomProcedure([](IBaseObject*) { return OPENDAQ_SUCCESS; });
    procObj.dispatch();
    procObj.dispatch(BaseObjectPtr(nullptr));
}

TEST_F(CustomProcedureTest, SmartPtrCallOperator)
{
    auto procObj = CustomProcedure([](IBaseObject*) { return OPENDAQ_SUCCESS; });
    procObj();
    procObj(BaseObjectPtr(nullptr));
}

TEST_F(CustomProcedureTest, Params)
{
    auto procObj = CustomProcedure([](IBaseObject* obj)
    {
        Int a = BaseObjectPtr::Borrow(obj);
        return a == 3
            ? OPENDAQ_SUCCESS
            :  OPENDAQ_ERR_INVALIDPARAMETER;
    });
    procObj.dispatch((Int) 3);

    ASSERT_ANY_THROW(procObj.dispatch((Int) 2));
}

TEST_F(CustomProcedureTest, ParamsCallOperator)
{
    auto procObj = CustomProcedure([](IBaseObject* obj)
    {
        Int a = BaseObjectPtr::Borrow(obj);
        return a == 3
            ? OPENDAQ_SUCCESS
            :  OPENDAQ_ERR_INVALIDPARAMETER;
    });
    procObj((Int) 3);

    ASSERT_ANY_THROW(procObj((Int) 2));
}

TEST_F(CustomProcedureTest, StandardParams)
{
    auto procObj = CustomProcedure([](IBaseObject* obj)
    {
        Int a = BaseObjectPtr::Borrow(obj);
        return a == 3 ? OPENDAQ_SUCCESS :  OPENDAQ_ERR_INVALIDPARAMETER;
    });

    procObj.execute((Int) 3);
    ASSERT_ANY_THROW(procObj.execute((Int) 2));
}

TEST_F(CustomProcedureTest, NullFuncExecuteThrows)
{
    auto nullFunc = CustomProcedure(nullptr);

    ASSERT_ANY_THROW(nullFunc.execute());
}

TEST_F(CustomProcedureTest, TwoStandardParams)
{
    auto procObj = CustomProcedure([](IBaseObject* params)
    {
        auto list = ListObjectPtr<IList, IBaseObject>(params);

        Int a = list.getItemAt(0);
        Int b = list.getItemAt(1);
        return a == 3 && b == 2 ? OPENDAQ_SUCCESS :  OPENDAQ_ERR_INVALIDPARAMETER;
    });

    procObj.execute((Int) 3, Int(2));
    ASSERT_ANY_THROW(procObj.execute((Int) 2));

    procObj((Int) 3, (Int) 2);
}

TEST_F(CustomProcedureTest, NoStandardParams)
{
    auto procObj = CustomProcedure([](IBaseObject* params)
    {
        return params == nullptr
                   ? OPENDAQ_SUCCESS
                   :  OPENDAQ_ERR_INVALIDPARAMETER;
    });

    procObj.execute();
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

TEST_F(CustomProcedureTest, CoreType)
{
    auto procObj = CustomProcedure([](IBaseObject* /*params*/) { return OPENDAQ_SUCCESS; });
    enum CoreType coreType = procObj.getCoreType();

    ASSERT_EQ(coreType, ctProc);
}

TEST_F(CustomProcedureTest, CoreTypeNull)
{
    auto procObj = CustomProcedure(nullptr);
    enum CoreType coreType = procObj.getCoreType();

    ASSERT_EQ(coreType, ctProc);
}

TEST_F(CustomProcedureTest, Inspectable)
{
    auto obj = CustomProcedure(nullptr);

    auto ids = obj.asPtr<IInspectable>(true).getInterfaceIds();
    ASSERT_EQ(ids[0], IProcedure::Id);
}

TEST_F(CustomProcedureTest, ImplementationName)
{
    auto obj = CustomProcedure(nullptr);

    std::string className = obj.asPtr<IInspectable>(true).getRuntimeClassName();
    auto prefix = className.find("daq::CustomProcedureImpl<");
    ASSERT_EQ(prefix, 0u);
}
