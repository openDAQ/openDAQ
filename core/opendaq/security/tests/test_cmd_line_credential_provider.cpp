#include <gtest/gtest.h>
#include <opendaq/credential_provider_factory.h>
#include <opendaq/credential_request_factory.h>
#include <opendaq/credential_field_factory.h>
#include <opendaq/authentication_method_factory.h>
#include <opendaq/device_type_factory.h>
#include <opendaq/security_exceptions.h>
#include <sstream>
#include <iostream>
#include <cstdio>
#include <fstream>

// `CmdLineCredentialProviderImpl` prompts via `std::cin`/`std::cout` for `Text`/`FilePath` fields using plain
// `std::getline`, so redirecting `std::cin`'s buffer to a canned input stream exercises the real interactive
// code path deterministically, no actual terminal needed. `Secret` fields, however, go through a raw,
// per-character read that (on POSIX) manipulates `STDIN_FILENO` directly via `tcgetattr`/`tcsetattr`, so `Secret`-kind
// fields are deliberately not covered here; `StandardPin`/`StandardUserNamePassword` (both carry a `Secret`
// field) are out of scope for this reason. Every method built below uses `Text`/`FilePath` fields only.

using CmdLineCredentialProviderTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

class CmdLineTestStdinRedirect
{
public:
    explicit CmdLineTestStdinRedirect(const std::string& input)
        : inputStream(input)
        , oldBuf(std::cin.rdbuf(inputStream.rdbuf()))
    {
    }

    ~CmdLineTestStdinRedirect()
    {
        std::cin.rdbuf(oldBuf);
        std::cin.clear();
    }

private:
    std::istringstream inputStream;
    std::streambuf* oldBuf;
};

class CmdLineTestStdoutSuppress
{
public:
    CmdLineTestStdoutSuppress()
        : oldBuf(std::cout.rdbuf(capture.rdbuf()))
    {
    }

    ~CmdLineTestStdoutSuppress()
    {
        std::cout.rdbuf(oldBuf);
    }

private:
    std::ostringstream capture;
    std::streambuf* oldBuf;
};

// A temporary file that exists for the lifetime of the object - used to supply a `FilePath` field with a
// path `CmdLineCredentialProviderImpl::isFileAccessible` accepts.
class CmdLineTestTempFile
{
public:
    explicit CmdLineTestTempFile(const std::string& path)
        : path(path)
    {
        std::ofstream(path) << "test";
    }

    ~CmdLineTestTempFile()
    {
        std::remove(path.c_str());
    }

    const std::string path;
};

static ComponentTypePtr CmdLineTestComponentType()
{
    return DeviceType("TestDeviceType", "Test device", "Test device type", "test");
}

static CredentialRequestPtr CmdLineTestCredentialRequest(const AuthenticationMethodPtr& method)
{
    return CredentialRequestBuilder()
        .setComponentType(CmdLineTestComponentType())
        .setConnectionString("daq://device1")
        .setAuthenticationMethod(method)
        .build();
}

TEST_F(CmdLineCredentialProviderTest, GetDescription)
{
    const auto provider = CmdLineCredentialProvider();

    ASSERT_EQ(provider.getDescription(), "Prompts for credentials interactively via the command line.");
}

TEST_F(CmdLineCredentialProviderTest, RequestCredentialsReadsEachTextField)
{
    const auto fields = Dict<IString, ICredentialField>(
        {{"UserName", CredentialField("UserName", CredentialFieldKind::Text, "User name")},
         {"Nickname", CredentialField("Nickname", CredentialFieldKind::Text, "Nickname")}});
    const auto method = AuthenticationMethod("TwoTextFieldsTest", fields, "Two text fields");

    const auto provider = CmdLineCredentialProvider();

    CmdLineTestStdoutSuppress noOutput;
    CmdLineTestStdinRedirect input("john\njohnny\n");

    const auto credentials = provider.requestCredentials(CmdLineTestCredentialRequest(method));
    const auto expectedCredentials = Dict<IString, IString>({{"UserName", "john"}, {"Nickname", "johnny"}});

    ASSERT_EQ(credentials, expectedCredentials);
}

TEST_F(CmdLineCredentialProviderTest, RequestCredentialsAcceptsAccessibleFilePath)
{
    const CmdLineTestTempFile keyFile("cmd_line_credential_provider_test_file.txt");

    const auto fields = Dict<IString, ICredentialField>(
        {{"KeyPath", CredentialField("KeyPath", CredentialFieldKind::FilePath, "Key file path")}});
    const auto method = AuthenticationMethod("FilePathTest", fields, "A single file-path field");

    const auto provider = CmdLineCredentialProvider();

    CmdLineTestStdoutSuppress noOutput;
    CmdLineTestStdinRedirect input(keyFile.path + "\n");

    const auto credentials = provider.requestCredentials(CmdLineTestCredentialRequest(method));
    const auto expectedCredentials = Dict<IString, IString>({{"KeyPath", keyFile.path}});

    ASSERT_EQ(credentials, expectedCredentials);
}

TEST_F(CmdLineCredentialProviderTest, RequestCredentialsThrowsOnInaccessibleFilePath)
{
    const auto fields = Dict<IString, ICredentialField>(
        {{"KeyPath", CredentialField("KeyPath", CredentialFieldKind::FilePath, "Key file path")}});
    const auto method = AuthenticationMethod("FilePathTest", fields, "A single file-path field");

    const auto provider = CmdLineCredentialProvider();

    CmdLineTestStdoutSuppress noOutput;
    CmdLineTestStdinRedirect input("/no/such/file/path\n");

    ASSERT_THROW(provider.requestCredentials(CmdLineTestCredentialRequest(method)), NotFoundException);
}

TEST_F(CmdLineCredentialProviderTest, RequestCredentialsThrowsProviderCancelledOnEof)
{
    const auto fields = Dict<IString, ICredentialField>(
        {{"UserName", CredentialField("UserName", CredentialFieldKind::Text, "User name")}});
    const auto method = AuthenticationMethod("TextFieldTest", fields, "A single text field");

    const auto provider = CmdLineCredentialProvider();

    CmdLineTestStdoutSuppress noOutput;
    CmdLineTestStdinRedirect input(""); // Already at EOF - simulates Ctrl+D/Ctrl+Z being pressed immediately.

    ASSERT_THROW(provider.requestCredentials(CmdLineTestCredentialRequest(method)), ProviderCancelledException);
}

TEST_F(CmdLineCredentialProviderTest, RequestCredentialsThrowsWhenRequestUnassigned)
{
    const auto provider = CmdLineCredentialProvider();

    ASSERT_THROW(provider.requestCredentials(nullptr), ArgumentNullException);
}

END_NAMESPACE_OPENDAQ
