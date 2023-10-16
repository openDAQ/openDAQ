#include <gtest/gtest.h>
#include <testutils/testutils.h>
#include <coretypes/common.h>
#include <coretypes/baseobject.h>
#include <coretypes/intfs.h>
#include <coretypes/objectptr.h>
#include <coretypes/inspectable_ptr.h>

using namespace daq;

BEGIN_NAMESPACE_OPENDAQ

DECLARE_OPENDAQ_INTERFACE(IErrorTest, IBaseObject)
{
    virtual ErrCode INTERFACE_FUNC test(Bool returnError) = 0;
};

END_NAMESPACE_OPENDAQ

class ErrorTestImpl : public ImplementationOf<IErrorTest>
{
public:
    ErrorTestImpl()
    {
    }

    ErrCode INTERFACE_FUNC test(Bool returnError) override
    {
        if (!returnError)
            return OPENDAQ_SUCCESS;

        return makeErrorInfo(OPENDAQ_ERR_GENERALERROR, "Test failed");
    }
};

static ObjectPtr<IErrorTest> CreateTestObject()
{
    IErrorTest* testObject = new ErrorTestImpl();
    testObject->addRef();
    return testObject;
}

using ErrorInfoTest = testing::Test;

TEST_F(ErrorInfoTest, CheckObject)
{
    auto obj = CreateTestObject();
}

TEST_F(ErrorInfoTest, NoError)
{
    auto obj = CreateTestObject();
    ASSERT_EQ(obj->test(false), OPENDAQ_SUCCESS);
}

TEST_F(ErrorInfoTest, Error)
{
    auto obj = CreateTestObject();

    ASSERT_THROW_MSG(checkErrorInfo(obj->test(true)), GeneralErrorException, "Test failed");
}

TEST_F(ErrorInfoTest, Inspectable)
{
    auto obj = CreateTestObject();

    auto ids = obj.asPtr<IInspectable>(true).getInterfaceIds();
    ASSERT_EQ(ids[0], IErrorTest::Id);
}

TEST_F(ErrorInfoTest, ImplementationName)
{
    auto obj = CreateTestObject();

    StringPtr className = obj.asPtr<IInspectable>(true).getRuntimeClassName();
    ASSERT_EQ(className, "ErrorTestImpl");
}
