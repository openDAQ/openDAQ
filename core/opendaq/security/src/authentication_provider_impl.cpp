#include <opendaq/authentication_provider_impl.h>
#include <opendaq/security_errors.h>
#include <opendaq/security_exceptions.h>
#include <fstream>
#include <opendaq/user_internal_ptr.h>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

BEGIN_NAMESPACE_OPENDAQ

// AuthenticationProviderImpl

AuthenticationProviderImpl::AuthenticationProviderImpl()
    : users(Dict<IString, IUser>())
{
}

ErrCode INTERFACE_FUNC AuthenticationProviderImpl::authenticate(IString* username, IString* password, IUser** userOut)
{
    const auto user = findUser(username);
    if (!user.assigned())
        return OPENDAQ_ERR_AUTHENTICATION_FAILED;

    const auto hash = user.asPtr<IUserInternal>(true).getPasswordHash();
    if (!isPasswordValid(hash, password))
        return OPENDAQ_ERR_AUTHENTICATION_FAILED;

    *userOut = user.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

void AuthenticationProviderImpl::loadUserList(const ListPtr<IUser>& userList)
{
    users.clear();

    if (userList.assigned())
    {
        for (const auto& user : userList)
            users.set(user.getUsername(), user);
    }
}

UserPtr AuthenticationProviderImpl::findUser(const StringPtr& username)
{
    if (users.hasKey(username))
        return users.get(username);

    return nullptr;
}

bool AuthenticationProviderImpl::isPasswordValid(const StringPtr& hash, const StringPtr& password)
{
    // return libsodium.passwordVerify(hash, password);
    return hash == password;
}


// StaticAuthenticationProviderImpl

StaticAuthenticationProviderImpl::StaticAuthenticationProviderImpl(const ListPtr<IUser>& users)
    : AuthenticationProviderImpl()
{
    loadUserList(users);
}


// JsonStringAuthenticationProviderImpl

JsonStringAuthenticationProviderImpl::JsonStringAuthenticationProviderImpl(const StringPtr& jsonString)
    : AuthenticationProviderImpl()
{
    loadJsonString(jsonString);
}

void JsonStringAuthenticationProviderImpl::loadJsonString(const StringPtr& jsonString)
{
    std::string stdJsonString = jsonString;
    boost::trim_left(stdJsonString);
    boost::trim_right(stdJsonString);

    if (stdJsonString.empty())
        return;

    auto serializer = JsonDeserializer();
    ListPtr<IUser> userList = serializer.deserialize(stdJsonString);
    loadUserList(userList);
}


// JsonFileAuthenticationProviderImpl

JsonFileAuthenticationProviderImpl::JsonFileAuthenticationProviderImpl(const StringPtr& filename)
    : JsonStringAuthenticationProviderImpl(readJsonFile(filename))
{
}

std::string JsonFileAuthenticationProviderImpl::readJsonFile(const StringPtr& filename)
{
    const std::string filenameStr = filename.toStdString();
    const auto exists = boost::filesystem::exists(filenameStr);

    if (!exists)
        throw NotFoundException("Json authentication file does not exist");

    std::ifstream file;
    file.open(filenameStr);

    std::stringstream ss;
    ss << file.rdbuf();
    const auto jsonStr = ss.str();
    return jsonStr;
}


// Factories

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(
    LIBRARY_FACTORY, StaticAuthenticationProviderImpl, IAuthenticationProvider, createStaticAuthenticationProvider, IList*, userList)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(LIBRARY_FACTORY,
                                                               JsonStringAuthenticationProviderImpl,
                                                               IAuthenticationProvider,
                                                               createJsonStringAuthenticationProvider,
                                                               IString*,
                                                               jsonString)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(
    LIBRARY_FACTORY, JsonFileAuthenticationProviderImpl, IAuthenticationProvider, createJsonFileAuthenticationProvider, IString*, filename)


END_NAMESPACE_OPENDAQ
