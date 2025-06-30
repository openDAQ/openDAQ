#include <gtest/gtest.h>
#include <testutils/testutils.h>
#include <coretypes/common.h>
#include <coretypes/baseobject.h>
#include <coretypes/intfs.h>
#include <coretypes/objectptr.h>
#include <coretypes/inspectable_ptr.h>
#include <coretypes/validation.h>
#include <coretypes/ctutils.h>
#include <coretypes/errorinfo_factory.h>
#include <coretypes/listobject_factory.h>
#include <coretypes/json_serializer_factory.h>
#include <coretypes/json_deserializer_factory.h>

using namespace daq;

BEGIN_NAMESPACE_OPENDAQ

DECLARE_OPENDAQ_INTERFACE(IErrorTest, IBaseObject)
{
    virtual ErrCode INTERFACE_FUNC test(Bool returnError) = 0;
    virtual ErrCode INTERFACE_FUNC newMakeErrorInfoTest() = 0;
    virtual ErrCode INTERFACE_FUNC multipleErrorInfoTest() = 0;
    virtual ErrCode INTERFACE_FUNC argumentNotNullTest(IBaseObject* obj) = 0;
    virtual ErrCode INTERFACE_FUNC throwExceptionTest() = 0;
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

        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_GENERALERROR, "Test failed");
    }

    ErrCode INTERFACE_FUNC newMakeErrorInfoTest() override
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_GENERALERROR, "newMakeErrorInfoTest failed");
    }

    ErrCode INTERFACE_FUNC multipleErrorInfoTest() override
    {
        const ErrCode errCode = newMakeErrorInfoTest();
        OPENDAQ_RETURN_IF_FAILED(errCode, "multipleErrorInfoTest failed twice");

        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC argumentNotNullTest(IBaseObject* obj) override
    {
        OPENDAQ_PARAM_NOT_NULL(obj);
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC throwExceptionTest() override
    {
        return daqTry([] 
        {
            DAQ_THROW_EXCEPTION(GeneralErrorException, "Test failed");
        });
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
    DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_GENERALERROR, "General error0");
    DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_GENERALERROR, "General error1");

    ObjectPtr<IErrorInfo> lastError;
    daqGetErrorInfo(&lastError);
    ASSERT_TRUE(lastError.assigned());

    ListPtr<IErrorInfo> errorInfoList;
    daqGetErrorInfoList(&errorInfoList);
    ASSERT_TRUE(errorInfoList.assigned());
    ASSERT_EQ(errorInfoList.getCount(), 2);

    for (SizeT i = 0; i < errorInfoList.getCount(); ++i)
    {
        auto errorInfoObject = errorInfoList[i];
        ASSERT_TRUE(errorInfoObject.assigned());

        StringPtr message;
        errorInfoObject->getMessage(&message);
        ASSERT_TRUE(message.assigned());

        std::string expectedMsg = "General error" + std::to_string(i);
        ASSERT_STREQ(message.getCharPtr(), expectedMsg.c_str());

#ifndef NDEBUG
        ConstCharPtr fileName;
        errorInfoObject->getFileName(&fileName);
        ASSERT_TRUE(fileName != nullptr);

        Int line;
        errorInfoObject->getFileLine(&line);
        ASSERT_NE(line, -1);
#endif
    }
}

std::string getErrorPostfix([[maybe_unused]] Int fileLine)
{
#ifdef NDEBUG
    return "";
#else
    return " [ " + std::string(__FILE__) + ":" + std::to_string(fileLine) + " ]";
#endif
}

TEST_F(ErrorInfoTest, ErrorWithFileNameAndLine)
{
    auto obj = CreateTestObject();

    std::string expected = "newMakeErrorInfoTest failed" + getErrorPostfix(47);
    ASSERT_THROW_MSG(checkErrorInfo(obj->newMakeErrorInfoTest()), GeneralErrorException, expected);
}

TEST_F(ErrorInfoTest, MultipleErrorWithFileNameAndLine)
{
    auto obj = CreateTestObject();

    std::string expected = "newMakeErrorInfoTest failed" + getErrorPostfix(47) + "\n";
    expected += " - Cause by: multipleErrorInfoTest failed twice" + getErrorPostfix(53);
    ASSERT_THROW_MSG(checkErrorInfo(obj->multipleErrorInfoTest()), GeneralErrorException, expected);
}

TEST_F(ErrorInfoTest, ArgumentNotNull)
{
    auto obj = CreateTestObject();
    
#ifdef NDEBUG
    std::string expected = "Parameter obj must not be null in the function \"";
#else
    std::string expected = "Parameter obj must not be null" + getErrorPostfix(60);
#endif
    ASSERT_THROW_MSG(checkErrorInfo(obj->argumentNotNullTest(nullptr)), ArgumentNullException, expected);
}

TEST_F(ErrorInfoTest, ThrowExceptionInDaqTry)
{
    auto obj = CreateTestObject();

    std::string expected = "Test failed" + getErrorPostfix(68);
    ASSERT_THROW_MSG(checkErrorInfo(obj->throwExceptionTest()), GeneralErrorException, expected);
}

TEST_F(ErrorInfoTest, SerializeDeserializeEmpty)
{
    ObjectPtr<IErrorInfo> errorInfo;
    ASSERT_EQ(createErrorInfo(&errorInfo), OPENDAQ_SUCCESS);
    ASSERT_TRUE(errorInfo.assigned());

    const auto serializer = JsonSerializer();
    errorInfo.serialize(serializer);
    const auto jsonStr = serializer.getOutput();

    const auto deserializer = JsonDeserializer();
    const ObjectPtr<IErrorInfo> newErrorInfo = deserializer.deserialize(jsonStr);
    ASSERT_TRUE(newErrorInfo.assigned());

    serializer.reset();
    newErrorInfo.serialize(serializer);
    const auto jsonStrNew = serializer.getOutput();

    ASSERT_EQ(jsonStr, jsonStrNew);
}

TEST_F(ErrorInfoTest, SerializeDeserialize)
{
    const auto serializer = JsonSerializer();
    {
        ObjectPtr<IErrorInfo> errorInfo;
        ASSERT_EQ(createErrorInfo(&errorInfo), OPENDAQ_SUCCESS);
        ASSERT_TRUE(errorInfo.assigned());

        ASSERT_EQ(errorInfo->setMessage(String("Error info message")), OPENDAQ_SUCCESS);
        ASSERT_EQ(errorInfo->setSource(String("Error info source")), OPENDAQ_SUCCESS);
        ASSERT_EQ(errorInfo->setFileName("filepath/filename.cpp"), OPENDAQ_SUCCESS);
        ASSERT_EQ(errorInfo->setFileLine(1234), OPENDAQ_SUCCESS);

        errorInfo.serialize(serializer);
    }
    const auto jsonStr = serializer.getOutput();

    const auto deserializer = JsonDeserializer();
    const ObjectPtr<IErrorInfo> newErrorInfo = deserializer.deserialize(jsonStr);
    ASSERT_TRUE(newErrorInfo.assigned());

    StringPtr message;
    newErrorInfo->getMessage(&message);
    ASSERT_EQ(message, "Error info message");

    StringPtr source;
    newErrorInfo->getSource(&source);
    ASSERT_EQ(source, "Error info source");

    ConstCharPtr fileName;
    newErrorInfo->getFileName(&fileName);
    ASSERT_TRUE(fileName != nullptr);
    ASSERT_STREQ(fileName, "filepath/filename.cpp");

    Int fileLine;
    newErrorInfo->getFileLine(&fileLine);
    ASSERT_EQ(fileLine, 1234);

    serializer.reset();
    newErrorInfo.serialize(serializer);
    const auto jsonStrNew = serializer.getOutput();

    ASSERT_EQ(jsonStr, jsonStrNew);
}

TEST_F(ErrorInfoTest, errorGuardClearing)
{
    const auto trackedObjectCount = daqGetTrackedObjectCount();
    // create error scope and error here
    {
        auto errorGuard = DAQ_ERROR_GUARD();
        {
            auto errorGuard2 = DAQ_ERROR_GUARD();
            DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_GENERALERROR, "Error in error guard2");

            ListPtr<IErrorInfo> errorInfoList;
            errorGuard2->getErrorInfos(&errorInfoList);
            ASSERT_TRUE(errorInfoList.assigned());
            ASSERT_EQ(errorInfoList.getCount(), 1);
        }

        DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_GENERALERROR, "Error in error guard");
        ListPtr<IErrorInfo> errorInfoList;
        errorGuard->getErrorInfos(&errorInfoList);
        ASSERT_TRUE(errorInfoList.assigned());
        ASSERT_EQ(errorInfoList.getCount(), 1);
    }
    // scope is cleared, error info should be cleared as well
    ObjectPtr<IErrorInfo> lastError;
    daqGetErrorInfo(&lastError, OPENDAQ_ERR_GENERALERROR);
    ASSERT_FALSE(lastError.assigned());

    ASSERT_EQ(daqGetTrackedObjectCount(), trackedObjectCount);
}
