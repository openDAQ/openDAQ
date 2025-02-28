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
    virtual ErrCode INTERFACE_FUNC test2() = 0;
    virtual ErrCode INTERFACE_FUNC test3() = 0;
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

        return this->makeErrorInfo(OPENDAQ_ERR_GENERALERROR, "Test failed");
    }

    ErrCode INTERFACE_FUNC test2() override
    {
        return this->MakeErrorInfo(OPENDAQ_ERR_GENERALERROR, "Test2 failed");
    }

    ErrCode INTERFACE_FUNC test3() override
    {
        this->MakeErrorInfo(OPENDAQ_ERR_GENERALERROR, "Test3 failed once");
        return this->MakeErrorInfo(OPENDAQ_ERR_GENERALERROR, "Test3 failed twice");
    }
};

static ObjectPtr<IErrorTest> CreateTestObject()
{
    IErrorTest* testObject = new ErrorTestImpl();
    testObject->addRef();
    return testObject;
}

using ErrorInfoTest = testing::Test;

static constexpr auto INTERFACE_ID = FromTemplatedTypeName("IErrorInfo", "daq");

TEST_F(ErrorInfoTest, InterfaceId)
{
    ASSERT_EQ(INTERFACE_ID, IErrorInfo::Id);
}

TEST_F(ErrorInfoTest, InterfaceIdString)
{
    ASSERT_EQ(daqInterfaceIdString<IErrorInfo>(), "{483B3446-8F45-53CE-B4EE-EC2B03CF6A4C}");
}

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

TEST_F(ErrorInfoTest, MultipleMessages)
{
    MakeErrorInfoForSource(OPENDAQ_ERR_GENERALERROR, nullptr, "General error0");
    MakeErrorInfoForSource(OPENDAQ_ERR_GENERALERROR, nullptr, "General error1");

    IErrorInfo* lastError;
    daqGetErrorInfo(&lastError);

    ASSERT_TRUE(lastError != nullptr);
    Finally finally([&]
    {
        if (lastError != nullptr)
            lastError->releaseRef();
        daqClearErrorInfo();
    });

    IList* errorInfoList;
    daqGetErrorInfoList(&errorInfoList);

    ASSERT_TRUE(errorInfoList != nullptr);
    Finally finally3([&]
    {
        if (errorInfoList != nullptr)
            errorInfoList->releaseRef();
    });

    SizeT count = 0;
    errorInfoList->getCount(&count);
    ASSERT_EQ(count, 2);

    for (SizeT i = 0; i < count; ++i)
    {
        IBaseObject* errorInfoObject;
        errorInfoList->getItemAt(i, &errorInfoObject);

        ASSERT_TRUE(errorInfoObject != nullptr);
        Finally finally4([&]
        {
            if (errorInfoObject != nullptr)
                errorInfoObject->releaseRef();
        });

        IErrorInfo* errorInfo;
        errorInfoObject->borrowInterface(IErrorInfo::Id, reinterpret_cast<void**>(&errorInfo));

        if (i == count -1)
        {
            ASSERT_EQ(errorInfo, lastError);
        }

        IString* message;
        errorInfo->getMessage(&message);

        ASSERT_TRUE(message != nullptr);
        Finally finally5([&]
        {
            if (message != nullptr)
                message->releaseRef();
        });

        ConstCharPtr msgCharPtr;
        message->getCharPtr(&msgCharPtr);

        std::string expectedMsg = "General error" + std::to_string(i);
        ASSERT_STREQ(msgCharPtr, expectedMsg.c_str());

// #ifndef NDEBUG
        ConstCharPtr fileName;
        errorInfo->getFileName(&fileName);
        ASSERT_TRUE(fileName != nullptr);

        Int line;
        errorInfo->getFileLine(&line);
        ASSERT_NE(line, -1);
// #endif
    }
}

TEST_F(ErrorInfoTest, ErrorWithFileNameAndLine)
{
    auto obj = CreateTestObject();

    std::string expected = "[ " + std::string(__FILE__) + ":39 ] : Test2 failed"; 
    ASSERT_THROW_MSG(checkErrorInfo(obj->test2()), GeneralErrorException, expected);
}

TEST_F(ErrorInfoTest, MultipleErrorWithFileNameAndLine)
{
    auto obj = CreateTestObject();

    std::string expected = "[ " + std::string(__FILE__) + ":44 ] : Test3 failed once";
    expected += "\n[ " + std::string(__FILE__) + ":45 ] : Test3 failed twice"; 
    ASSERT_THROW_MSG(checkErrorInfo(obj->test3()), GeneralErrorException, expected);
}