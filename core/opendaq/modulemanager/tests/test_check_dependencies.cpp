#include <gtest/gtest.h>
#include <coretypes/stringobject_factory.h>
#include <opendaq/module_manager_check_dependencies.h>
#include <opendaq/module_check_dependencies.h>
#include <testutils/testutils.h>
#include <coretypes/exceptions.h>
#include <opendaq/opendaq_config.h>

#include <opendaq/version.h>
#include <coretypes/version.h>
#include <coreobjects/version.h>
#include <coretypes/dictobject_factory.h>

using namespace daq;

using CheckDependenciesTest = testing::Test;

TEST_F(CheckDependenciesTest, GetVersion)
{
    unsigned int major;
    unsigned int minor;
    unsigned int revision;
    daqOpenDaqGetVersion(&major, &minor, &revision);

    ASSERT_EQ(major, OPENDAQ_OPENDAQ_MAJOR_VERSION);
    ASSERT_EQ(minor, OPENDAQ_OPENDAQ_MINOR_VERSION);
    ASSERT_EQ(revision, OPENDAQ_OPENDAQ_PATCH_VERSION);
}

TEST_F(CheckDependenciesTest, GetVersionMetadata)
{
    unsigned int major, minor, patch;
    StringPtr branch, sha;

    daq::ErrCode errCode = getSdkCoreVersionMetadata(&major, &minor, &patch, &branch, &sha, nullptr);
    ASSERT_EQ(errCode, OPENDAQ_SUCCESS);

    ASSERT_TRUE(branch.assigned());
    ASSERT_TRUE(sha.assigned());

    ASSERT_EQ(major, OPENDAQ_OPENDAQ_MAJOR_VERSION);
    ASSERT_EQ(minor, OPENDAQ_OPENDAQ_MINOR_VERSION);
    ASSERT_EQ(patch, OPENDAQ_OPENDAQ_PATCH_VERSION);
    ASSERT_EQ(branch, OPENDAQ_OPENDAQ_BRANCH_NAME);
    ASSERT_EQ(sha, OPENDAQ_OPENDAQ_REVISION_HASH);
}

TEST_F(CheckDependenciesTest, TestVersions)
{
    auto linkedVersion = [](unsigned int* major, unsigned int* minor, unsigned int* revision)
    {
        *major = 1;
        *minor = 1;
        *revision = 0;
    };
    StringPtr errMsg;

    LibraryVersion matchingVersion{1, 1, 0};
    ASSERT_TRUE(isCompatibleVersion("CoreTypes", linkedVersion, matchingVersion, &errMsg));

    LibraryVersion compatibleVersion{1, 1, 1};
    ASSERT_TRUE(isCompatibleVersion("CoreTypes", linkedVersion, compatibleVersion, &errMsg));

    LibraryVersion incompatibleMajorVersion{2, 1, 0};
    ASSERT_FALSE(isCompatibleVersion("CoreTypes", linkedVersion, incompatibleMajorVersion, &errMsg));

    LibraryVersion incompatibleMinorVersion{1, 2, 0};
    ASSERT_FALSE(isCompatibleVersion("CoreTypes", linkedVersion, incompatibleMinorVersion, &errMsg));
}

static void ExpectLogMessageContain(const std::string& message, const std::string& substring)
{
    ASSERT_TRUE(std::string(message).find(substring) != std::string::npos)
        << "Expected log message contains \"" << substring << "\"" << std::endl
        << "Actual message is \"" << message << "\".";
}

static ErrCode getCoreVersionMetadataDefault(unsigned int* major, unsigned int* minor, unsigned int* patch, daq::IString** branch, daq::IString** sha, daq::IString** /*fork*/)
{
    if (major != nullptr)
        *major = OPENDAQ_OPENDAQ_MAJOR_VERSION;
    if (minor != nullptr)
        *minor = OPENDAQ_OPENDAQ_MINOR_VERSION;
    if (patch != nullptr)
        *patch = OPENDAQ_OPENDAQ_PATCH_VERSION;
    if (branch != nullptr)
        *branch = daq::String(OPENDAQ_OPENDAQ_BRANCH_NAME).detach();
    if (sha != nullptr)
        *sha = daq::String(OPENDAQ_OPENDAQ_REVISION_HASH).detach();
    return OPENDAQ_SUCCESS;
}

TEST_F(CheckDependenciesTest, InvalidParamsFailure)
{
    StringPtr dummy = String("");
    ASSERT_THROW(
        checkErrorInfo(checkModuleVersionCompatibility(OPENDAQ_OPENDAQ_MAJOR_VERSION, OPENDAQ_OPENDAQ_MINOR_VERSION, OPENDAQ_OPENDAQ_PATCH_VERSION, dummy, dummy, dummy, nullptr)),
        ArgumentNullException
    );

    StringPtr logMessage;
    ASSERT_THROW(
        checkErrorInfo(checkModuleVersionCompatibility(OPENDAQ_OPENDAQ_MAJOR_VERSION, OPENDAQ_OPENDAQ_MINOR_VERSION, OPENDAQ_OPENDAQ_PATCH_VERSION, nullptr, nullptr, nullptr, &logMessage)),
        ArgumentNullException
    );
}

TEST_F(CheckDependenciesTest, DefaultMetadataSuccess)
{
    unsigned int major, minor, patch;
    StringPtr branch, sha;
    getCoreVersionMetadataDefault(&major, &minor, &patch, &branch, &sha, nullptr);

    StringPtr logMessage;
    ErrCode errCode = checkModuleVersionCompatibility(major, minor, patch, branch, sha, nullptr, &logMessage);
    ASSERT_EQ(errCode, OPENDAQ_SUCCESS);
    ASSERT_TRUE(logMessage.assigned());
}

static ErrCode getCoreVersionMetadataMinimalMismatchFields(unsigned int* major, unsigned int* minor, unsigned int* patch, daq::IString** branch, daq::IString** sha, daq::IString** /*fork*/)
{
    if (major != nullptr)
        *major = OPENDAQ_OPENDAQ_MAJOR_VERSION;
    if (minor != nullptr)
        *minor = OPENDAQ_OPENDAQ_MINOR_VERSION;
    if (patch != nullptr)
        *patch = OPENDAQ_OPENDAQ_PATCH_VERSION + 1;
    if (branch != nullptr)
        *branch = daq::String(" .. ").detach(); // uses not allowed symbols so the comparison vs real or empty name always fails
    if (sha != nullptr)
        *sha = daq::String("it-is-not-hex").detach(); // uses not allowed symbols so comparison vs real or empty sha always fails
    return OPENDAQ_SUCCESS;
}

TEST_F(CheckDependenciesTest, MinimalMetadataMismatchFields)
{
    unsigned int major, minor, patch;
    StringPtr branch, sha;
    getCoreVersionMetadataMinimalMismatchFields(&major, &minor, &patch, &branch, &sha, nullptr);

    StringPtr logMessage;
    ErrCode errCode = checkModuleVersionCompatibility(major, minor, patch, branch, sha, nullptr, &logMessage);
    ASSERT_EQ(errCode, OPENDAQ_PARTIAL_SUCCESS);
    ASSERT_TRUE(logMessage.assigned());

    auto logMessageStr = logMessage.toStdString();
    ExpectLogMessageContain(logMessageStr, "the patch number mismatches");
    ExpectLogMessageContain(logMessageStr, "the git branch name mismatches");
    ExpectLogMessageContain(logMessageStr, "the git commit sha mismatches");
}

static ErrCode getCoreVersionMetadataMajorZero(unsigned int* major, unsigned int* minor, unsigned int* patch, daq::IString** branch, daq::IString** sha, daq::IString** /*fork*/)
{
    if (major != nullptr)
        *major = 0;
    if (minor != nullptr)
        *minor = OPENDAQ_OPENDAQ_MINOR_VERSION;
    if (patch != nullptr)
        *patch = OPENDAQ_OPENDAQ_PATCH_VERSION;
    if (branch != nullptr)
        *branch = daq::String(OPENDAQ_OPENDAQ_BRANCH_NAME).detach();
    if (sha != nullptr)
        *sha = daq::String(OPENDAQ_OPENDAQ_REVISION_HASH).detach();
    return OPENDAQ_SUCCESS;
}

TEST_F(CheckDependenciesTest, MajorNumberFailure)
{
    unsigned int major, minor, patch;
    StringPtr branch, sha;
    getCoreVersionMetadataMajorZero(&major, &minor, &patch, &branch, &sha, nullptr);

    StringPtr logMessage;

    ASSERT_THROW_MSG(checkErrorInfo(checkModuleVersionCompatibility(major, minor, patch, branch, sha, nullptr, &logMessage)),
                     NotCompatibleVersionException,
                     "the major number mismatches");
}

static ErrCode getCoreVersionMetadataMinorMismatch(unsigned int* major, unsigned int* minor, unsigned int* patch, daq::IString** branch, daq::IString** sha, daq::IString** /*fork*/)
{
    if (major != nullptr)
        *major = OPENDAQ_OPENDAQ_MAJOR_VERSION;
    if (minor != nullptr)
        *minor = OPENDAQ_OPENDAQ_MINOR_VERSION + 1;
    if (patch != nullptr)
        *patch = OPENDAQ_OPENDAQ_PATCH_VERSION;
    if (branch != nullptr)
        *branch = daq::String(OPENDAQ_OPENDAQ_BRANCH_NAME).detach();
    if (sha != nullptr)
        *sha = daq::String(OPENDAQ_OPENDAQ_REVISION_HASH).detach();
    return OPENDAQ_SUCCESS;
}

TEST_F(CheckDependenciesTest, MinorNumberFailure)
{
    unsigned int major, minor, patch;
    StringPtr branch, sha;
    getCoreVersionMetadataMinorMismatch(&major, &minor, &patch, &branch, &sha, nullptr);

    StringPtr logMessage;

    ASSERT_THROW_MSG(checkErrorInfo(checkModuleVersionCompatibility(major, minor, patch, branch, sha, nullptr, &logMessage)),
                     NotCompatibleVersionException,
                     "the minor number mismatches");
}
