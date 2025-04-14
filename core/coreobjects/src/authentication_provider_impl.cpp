#include <coreobjects/authentication_provider_impl.h>
#include <fstream>
#include <coreobjects/user_internal_ptr.h>
#include <coretypes/filesystem.h>
#include <coreobjects/errors.h>
#include <coreobjects/user_factory.h>
#include <bcrypt/BCrypt.hpp>
#include <coretypes/validation.h>

BEGIN_NAMESPACE_OPENDAQ


// ^\$(2[ayb]?)\$[0-9]+\$[a-zA-Z0-9\.\/]{53}$
static const std::regex BcryptRegex("^\\$(2[ayb]?)\\$[0-9]+\\$[a-zA-Z0-9\\.\\/]{53}$");


// AuthenticationProviderImpl

AuthenticationProviderImpl::AuthenticationProviderImpl(bool allowAnonymous)
    : allowAnonymous(allowAnonymous)
    , users(Dict<IString, IUser>())
    , anonymousUser(User("", ""))
{
}

ErrCode INTERFACE_FUNC AuthenticationProviderImpl::authenticate(IString* username, IString* password, IUser** userOut)
{
    const auto user = findUserInternal(username);
    if (!user.assigned())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_AUTHENTICATION_FAILED, "user not found");

    const auto hash = user.asPtr<IUserInternal>(true).getPasswordHash();
    if (!isPasswordValid(hash, password))
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_AUTHENTICATION_FAILED, "password for user is invalid");

    *userOut = user.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC AuthenticationProviderImpl::isAnonymousAllowed(Bool* allowedOut)
{
    OPENDAQ_PARAM_NOT_NULL(allowedOut);

    *allowedOut = this->allowAnonymous;
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC AuthenticationProviderImpl::authenticateAnonymous(IUser** userOut)
{
    OPENDAQ_PARAM_NOT_NULL(userOut);

    if (!this->allowAnonymous)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_AUTHENTICATION_FAILED, "Anonymous authentication is not allowed");

    *userOut = this->anonymousUser.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC AuthenticationProviderImpl::findUser(IString* username, IUser** userOut)
{
    OPENDAQ_PARAM_NOT_NULL(userOut);

    const UserPtr userTmp = findUserInternal(username);
    *userOut = userTmp.addRefAndReturn();
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

UserPtr AuthenticationProviderImpl::findUserInternal(const StringPtr& username)
{
    return users.getOrDefault(username);
}

bool AuthenticationProviderImpl::isPasswordValid(const std::string& hash, const StringPtr& password)
{
    if (std::regex_match(hash, BcryptRegex))
        return BCrypt::validatePassword(password, hash);

    return hash == password;
}


// StaticAuthenticationProviderImpl

StaticAuthenticationProviderImpl::StaticAuthenticationProviderImpl(bool allowAnonymous, const ListPtr<IUser>& users)
    : AuthenticationProviderImpl(allowAnonymous)
{
    loadUserList(users);
}


// JsonStringAuthenticationProviderImpl

JsonStringAuthenticationProviderImpl::JsonStringAuthenticationProviderImpl(const StringPtr& jsonString)
    : AuthenticationProviderImpl(false)
{
    loadJsonString(jsonString);
}

void JsonStringAuthenticationProviderImpl::loadJsonString(const StringPtr& jsonString)
{
    rapidjson::Document document;
    document.Parse(jsonString);

    if (document.HasParseError())
        DAQ_THROW_EXCEPTION(ParseFailedException);

    this->allowAnonymous = parseAllowAnonymous(document);
    this->users = parseUsers(document);
}

bool JsonStringAuthenticationProviderImpl::parseAllowAnonymous(rapidjson::Document& document)
{
    if (!document.HasMember("allowAnonymous") || !document["allowAnonymous"].IsBool())
        DAQ_THROW_EXCEPTION(ParseFailedException);

    return document["allowAnonymous"].GetBool();
}

DictPtr<IString, IUser> JsonStringAuthenticationProviderImpl::parseUsers(rapidjson::Document& document)
{
    auto users = Dict<IString, IUser>();

    if (!document.HasMember("users") || !document["users"].IsArray())
        return users;

    if (!document["users"].IsArray())
        DAQ_THROW_EXCEPTION(ParseFailedException);

    const auto userArray = document["users"].GetArray();

    for (size_t i = 0; i < userArray.Size(); i++)
    {
        if (!userArray[i].IsObject())
            DAQ_THROW_EXCEPTION(ParseFailedException);

        const auto user = parseUser(userArray[i].GetObject());
        users.set(user.getUsername(), user);
    }

    return users;
}

UserPtr JsonStringAuthenticationProviderImpl::parseUser(const rapidjson::Value& userObject)
{
    if (!userObject.HasMember("username") || !userObject["username"].IsString())
        DAQ_THROW_EXCEPTION(ParseFailedException);
    if (!userObject.HasMember("passwordHash") || !userObject["passwordHash"].IsString())
        DAQ_THROW_EXCEPTION(ParseFailedException);

    const auto username = userObject["username"].GetString();
    const auto passwordHash = userObject["passwordHash"].GetString();
    const auto groups = parseUserGroups(userObject);
    return User(username, passwordHash, groups);
}

ListPtr<IString> JsonStringAuthenticationProviderImpl::parseUserGroups(const rapidjson::Value& userObject)
{
    auto groups = List<IString>();

    if (!userObject.HasMember("groups"))
        return groups;
    if (!userObject["groups"].IsArray())
        DAQ_THROW_EXCEPTION(ParseFailedException);

    auto groupArray = userObject["groups"].GetArray();

    for (size_t i = 0; i < groupArray.Size(); i++)
    {
        if (!groupArray[i].IsString())
            DAQ_THROW_EXCEPTION(ParseFailedException);

        const auto groupName = groupArray[i].GetString();
        groups.pushBack(groupName);
    }

    return groups;
}


// JsonFileAuthenticationProviderImpl


JsonFileAuthenticationProviderImpl::JsonFileAuthenticationProviderImpl(const StringPtr& filename)
    : JsonStringAuthenticationProviderImpl(readJsonFile(filename))
{
}

std::string JsonFileAuthenticationProviderImpl::readJsonFile(const StringPtr& filename)
{
    const std::string filenameStr = filename.toStdString();
    const auto exists = fs::exists(filenameStr);

    if (!exists)
        DAQ_THROW_EXCEPTION(NotFoundException, "Json authentication file does not exist");

    std::ifstream file;
    file.open(filenameStr);

    std::stringstream ss;
    ss << file.rdbuf();
    const auto jsonStr = ss.str();
    return jsonStr;
}


// Factories

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(LIBRARY_FACTORY,
                                                               AuthenticationProviderImpl,
                                                               IAuthenticationProvider,
                                                               createAuthenticationProvider,
                                                               Bool,
                                                               allowAnonymous)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(LIBRARY_FACTORY,
                                                               StaticAuthenticationProviderImpl,
                                                               IAuthenticationProvider,
                                                               createStaticAuthenticationProvider,
                                                               Bool,
                                                               allowAnonymous,
                                                               IList*,
                                                               userList)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(LIBRARY_FACTORY,
                                                               JsonStringAuthenticationProviderImpl,
                                                               IAuthenticationProvider,
                                                               createJsonStringAuthenticationProvider,
                                                               IString*,
                                                               jsonString)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(LIBRARY_FACTORY,
                                                               JsonFileAuthenticationProviderImpl,
                                                               IAuthenticationProvider,
                                                               createJsonFileAuthenticationProvider,
                                                               IString*,
                                                               filename)


END_NAMESPACE_OPENDAQ
