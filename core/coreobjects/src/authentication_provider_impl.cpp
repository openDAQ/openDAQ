#include <coreobjects/authentication_provider_impl.h>
#include <fstream>
#include <coreobjects/user_internal_ptr.h>
#include <coretypes/filesystem.h>
#include <coreobjects/errors.h>
#include <coreobjects/user_factory.h>
#include <bcrypt/BCrypt.hpp>

BEGIN_NAMESPACE_OPENDAQ


// ^\$(2[ayb]?)\$[0-9]+\$[a-zA-Z0-9\.\/]{53}$
static const std::regex BcryptRegex("^\\$(2[ayb]?)\\$[0-9]+\\$[a-zA-Z0-9\\.\\/]{53}$");


// AuthenticationProviderImpl

AuthenticationProviderImpl::AuthenticationProviderImpl(bool allowAnonymous)
    : allowAnonymous(allowAnonymous)
    , users(Dict<IString, IUser>())
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

ErrCode INTERFACE_FUNC AuthenticationProviderImpl::isAnonymousAllowed(Bool* allowedOut)
{
    *allowedOut = this->allowAnonymous;
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
        throw ParseFailedException();

    this->allowAnonymous = parseAllowAnonymous(document);
    this->users = parseUsers(document);
}

bool JsonStringAuthenticationProviderImpl::parseAllowAnonymous(rapidjson::Document& document)
{
    if (!document.HasMember("allowAnonymous") || !document["allowAnonymous"].IsBool())
        throw ParseFailedException();

    return document["allowAnonymous"].GetBool();
}

DictPtr<IString, IUser> JsonStringAuthenticationProviderImpl::parseUsers(rapidjson::Document& document)
{
    auto users = Dict<IString, IUser>();

    if (!document.HasMember("users") || !document["users"].IsArray())
        return users;

    if (!document["users"].IsArray())
        throw ParseFailedException();

    const auto userArray = document["users"].GetArray();

    for (size_t i = 0; i < userArray.Size(); i++)
    {
        if (!userArray[i].IsObject())
            throw ParseFailedException();

        const auto user = parseUser(userArray[i].GetObject());
        users.set(user.getUsername(), user);
    }

    return users;
}

UserPtr JsonStringAuthenticationProviderImpl::parseUser(const rapidjson::Value& userObject)
{
    if (!userObject.HasMember("username") || !userObject["username"].IsString())
        throw ParseFailedException();
    if (!userObject.HasMember("passwordHash") || !userObject["passwordHash"].IsString())
        throw ParseFailedException();

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
        throw ParseFailedException();

    auto groupArray = userObject["groups"].GetArray();

    for (size_t i = 0; i < groupArray.Size(); i++)
    {
        if (!groupArray[i].IsString())
            throw ParseFailedException();

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
        throw NotFoundException("Json authentication file does not exist");

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
