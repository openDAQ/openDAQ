#include <gtest/gtest.h>
#include <coretypes/stringobject_factory.h>
#include <opendaq/module_check_dependencies.h>
#include <testutils/testutils.h>
#include <coretypes/exceptions.h>
#include <opendaq/opendaq_config.h>

#include <opendaq/version.h>
#include <coretypes/version.h>
#include <coreobjects/version.h>

using namespace daq;

using CheckDependenciesTest = testing::Test;

TEST_F(CheckDependenciesTest, ObsoleteGetVersionFailure)
{
    unsigned int dummy;
    ASSERT_THROW_MSG(
        daqCoreTypesGetVersion(&dummy, &dummy, &dummy),
        NotCompatibleVersionException,
        "does not support obsolete mechanism for checking core dependencies version"
    );
    ASSERT_THROW_MSG(
        daqCoreObjectsGetVersion(&dummy, &dummy, &dummy),
        NotCompatibleVersionException,
        "does not support obsolete mechanism for checking core dependencies version"
    );
    ASSERT_THROW_MSG(
        daqOpenDaqGetVersion(&dummy, &dummy, &dummy),
        NotCompatibleVersionException,
        "does not support obsolete mechanism for checking core dependencies version"
    );
}

static void ExpectLogMessageContain(const std::string& message, const std::string& substring)
{
    ASSERT_TRUE(std::string(message).find(substring) != std::string::npos)
        << "Expected log message contains \"" << substring << "\"" << std::endl
        << "Actual message is \"" << message << "\".";
}

static ErrCode getCoreVersionMetadataDefault(EnumerateMetadataFieldFunc enumerateFieldFunc, void* userData)
{
    enumerateFieldFunc("major", OPENDAQ_OPENDAQ_MAJOR_VERSION_STR, userData);
    enumerateFieldFunc("minor", OPENDAQ_OPENDAQ_MINOR_VERSION_STR, userData);
    enumerateFieldFunc("patch", OPENDAQ_OPENDAQ_PATCH_VERSION_STR, userData);
    enumerateFieldFunc("branch", OPENDAQ_OPENDAQ_BRANCH_NAME, userData);
    enumerateFieldFunc("sha", OPENDAQ_OPENDAQ_REVISION_HASH, userData);
    return OPENDAQ_SUCCESS;
}

TEST_F(CheckDependenciesTest, InvalidParamsFailure)
{
    ASSERT_THROW(checkErrorInfo(daqCoreValidateVersionMetadata(&getCoreVersionMetadataDefault, nullptr)), ArgumentNullException);

    StringPtr logMessage;
    ASSERT_THROW(checkErrorInfo(daqCoreValidateVersionMetadata(nullptr, &logMessage)), ArgumentNullException);
}

TEST_F(CheckDependenciesTest, DefaultMetadataSuccess)
{
    StringPtr logMessage;
    ErrCode errCode = daqCoreValidateVersionMetadata(&getCoreVersionMetadataDefault, &logMessage);
    ASSERT_EQ(errCode, OPENDAQ_SUCCESS);
    ASSERT_TRUE(logMessage.assigned());
}

static ErrCode getCoreVersionMetadataFailure(EnumerateMetadataFieldFunc enumerateFieldFunc, void* userData)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_GENERALERROR, "In-module failure");
}

TEST_F(CheckDependenciesTest, GetMetadataFailure)
{
    StringPtr logMessage;
    ASSERT_THROW_MSG(checkErrorInfo(daqCoreValidateVersionMetadata(&getCoreVersionMetadataFailure, &logMessage)),
                     GeneralErrorException,
                     "In-module failure");
}

static ErrCode getCoreVersionMetadataMinimalMissingFields(EnumerateMetadataFieldFunc enumerateFieldFunc, void* userData)
{
    enumerateFieldFunc("major", OPENDAQ_OPENDAQ_MAJOR_VERSION_STR, userData);
    enumerateFieldFunc("minor", OPENDAQ_OPENDAQ_MINOR_VERSION_STR, userData);
    return OPENDAQ_SUCCESS;
}

TEST_F(CheckDependenciesTest, MinimalMetadataMissingFields)
{
    StringPtr logMessage;
    ErrCode errCode = daqCoreValidateVersionMetadata(&getCoreVersionMetadataMinimalMissingFields, &logMessage);
    ASSERT_EQ(errCode, OPENDAQ_PARTIAL_SUCCESS);
    ASSERT_TRUE(logMessage.assigned());

    auto logMessageStr = logMessage.toStdString();
    ExpectLogMessageContain(logMessageStr, "The module has not provided the version patch number");
    ExpectLogMessageContain(logMessageStr, "The module has not provided the git branch name");
    ExpectLogMessageContain(logMessageStr, "The module has not provided the git commit sha");
}

static ErrCode getCoreVersionMetadataMinimalMismatchFields(EnumerateMetadataFieldFunc enumerateFieldFunc, void* userData)
{
    enumerateFieldFunc("major", OPENDAQ_OPENDAQ_MAJOR_VERSION_STR, userData);
    enumerateFieldFunc("minor", OPENDAQ_OPENDAQ_MINOR_VERSION_STR, userData);
    enumerateFieldFunc("patch", "not-a-number", userData); // comparison vs any number will fail
    enumerateFieldFunc("branch", " .. ", userData); // uses not allowed symbols so the comparison vs real or empty name always fails
    enumerateFieldFunc("sha", "it-is-not-hex", userData); // uses not allowed symbols so comparison vs real or empty sha always fails
    return OPENDAQ_SUCCESS;
}

TEST_F(CheckDependenciesTest, MinimalMetadataMismatchFields)
{
    StringPtr logMessage;
    ErrCode errCode = daqCoreValidateVersionMetadata(&getCoreVersionMetadataMinimalMismatchFields, &logMessage);
    ASSERT_EQ(errCode, OPENDAQ_PARTIAL_SUCCESS);
    ASSERT_TRUE(logMessage.assigned());

    auto logMessageStr = logMessage.toStdString();
    ExpectLogMessageContain(logMessageStr, "the patch number mismatches");
    ExpectLogMessageContain(logMessageStr, "the git branch name mismatches");
    ExpectLogMessageContain(logMessageStr, "the git commit sha mismatches");
}

static ErrCode getCoreVersionMetadataMajorEmpty(EnumerateMetadataFieldFunc enumerateFieldFunc, void* userData)
{
    enumerateFieldFunc("major", "not-a-number", userData); // comparison vs any number will fail
    enumerateFieldFunc("minor", OPENDAQ_OPENDAQ_MINOR_VERSION_STR, userData);
    enumerateFieldFunc("patch", OPENDAQ_OPENDAQ_PATCH_VERSION_STR, userData);
    enumerateFieldFunc("branch", OPENDAQ_OPENDAQ_BRANCH_NAME, userData);
    enumerateFieldFunc("sha", OPENDAQ_OPENDAQ_REVISION_HASH, userData);
    return OPENDAQ_SUCCESS;
}

static ErrCode getCoreVersionMetadataMajorMissing(EnumerateMetadataFieldFunc enumerateFieldFunc, void* userData)
{
    enumerateFieldFunc("minor", OPENDAQ_OPENDAQ_MINOR_VERSION_STR, userData);
    enumerateFieldFunc("patch", OPENDAQ_OPENDAQ_PATCH_VERSION_STR, userData);
    enumerateFieldFunc("branch", OPENDAQ_OPENDAQ_BRANCH_NAME, userData);
    enumerateFieldFunc("sha", OPENDAQ_OPENDAQ_REVISION_HASH, userData);
    return OPENDAQ_SUCCESS;
}

TEST_F(CheckDependenciesTest, MajorNumberFailure)
{
    StringPtr logMessage;

    ASSERT_THROW_MSG(checkErrorInfo(daqCoreValidateVersionMetadata(&getCoreVersionMetadataMajorEmpty, &logMessage)),
                     NotCompatibleVersionException,
                     "the major number mismatches");

    ASSERT_THROW_MSG(checkErrorInfo(daqCoreValidateVersionMetadata(&getCoreVersionMetadataMajorMissing, &logMessage)),
                     NotCompatibleVersionException,
                     "The module has not provided the version major number");
}

static ErrCode getCoreVersionMetadataMinorEmpty(EnumerateMetadataFieldFunc enumerateFieldFunc, void* userData)
{
    enumerateFieldFunc("major", OPENDAQ_OPENDAQ_MAJOR_VERSION_STR, userData);
    enumerateFieldFunc("minor", "not-a-number", userData); // comparison vs any number will fail
    enumerateFieldFunc("patch", OPENDAQ_OPENDAQ_PATCH_VERSION_STR, userData);
    enumerateFieldFunc("branch", OPENDAQ_OPENDAQ_BRANCH_NAME, userData);
    enumerateFieldFunc("sha", OPENDAQ_OPENDAQ_REVISION_HASH, userData);
    return OPENDAQ_SUCCESS;
}

static ErrCode getCoreVersionMetadataMinorMissing(EnumerateMetadataFieldFunc enumerateFieldFunc, void* userData)
{
    enumerateFieldFunc("major", OPENDAQ_OPENDAQ_MAJOR_VERSION_STR, userData);
    enumerateFieldFunc("patch", OPENDAQ_OPENDAQ_PATCH_VERSION_STR, userData);
    enumerateFieldFunc("branch", OPENDAQ_OPENDAQ_BRANCH_NAME, userData);
    enumerateFieldFunc("sha", OPENDAQ_OPENDAQ_REVISION_HASH, userData);
    return OPENDAQ_SUCCESS;
}

TEST_F(CheckDependenciesTest, MinorNumberFailure)
{
    StringPtr logMessage;

    ASSERT_THROW_MSG(checkErrorInfo(daqCoreValidateVersionMetadata(&getCoreVersionMetadataMinorEmpty, &logMessage)),
                     NotCompatibleVersionException,
                     "the minor number mismatches");

    ASSERT_THROW_MSG(checkErrorInfo(daqCoreValidateVersionMetadata(&getCoreVersionMetadataMinorMissing, &logMessage)),
                     NotCompatibleVersionException,
                     "The module has not provided the version minor number");
}
