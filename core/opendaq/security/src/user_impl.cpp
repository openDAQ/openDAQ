#include <opendaq/user_impl.h>
#include <coretypes/impl.h>
#include <coretypes/validation.h>

BEGIN_NAMESPACE_OPENDAQ

UserImpl::UserImpl(const StringPtr& username, const StringPtr& passwordHash, const ListPtr<IString> groups)
    : username(username)
    , passwordHash(passwordHash)
    , groups(groups)
{
    this->groups = groups.assigned() ? groups : ListPtr<IString>();
}

ErrCode INTERFACE_FUNC UserImpl::getUsername(IString** username)
{
    OPENDAQ_PARAM_NOT_NULL(username);

    *username = this->username.detach();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC UserImpl::getPasswordHash(IString** passwordHash)
{
    OPENDAQ_PARAM_NOT_NULL(passwordHash);

    *passwordHash = this->passwordHash.detach();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC UserImpl::getGroups(IList** groups)
{
    OPENDAQ_PARAM_NOT_NULL(groups);

    *groups = this->groups.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, User, IString*, username, IString*, passwordHash, IList*, groups)

END_NAMESPACE_OPENDAQ
