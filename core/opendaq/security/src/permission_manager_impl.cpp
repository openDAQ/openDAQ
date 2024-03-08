#include <opendaq/permission_manager_impl.h>
#include <coretypes/impl.h>
#include <coretypes/string_ptr.h>
#include <opendaq/user_ptr.h>
#include <coretypes/validation.h>

BEGIN_NAMESPACE_OPENDAQ

PermissionManagerImpl::PermissionManagerImpl()
{
}

ErrCode INTERFACE_FUNC PermissionManagerImpl::inherit(Bool* inherit, IPermissionManager** managerOut)
{
    OPENDAQ_PARAM_NOT_NULL(managerOut);

    return OPENDAQ_ERR_NOTIMPLEMENTED;
}

ErrCode INTERFACE_FUNC PermissionManagerImpl::setPermissions(IString* groupId, Int permissionFlags, IPermissionManager** managerOut)
{
    OPENDAQ_PARAM_NOT_NULL(managerOut);

    StringPtr groupIdStr = groupId;

    if (groupPermissions.count(groupIdStr) == 0)
        groupPermissions[groupIdStr] = 0;

    groupPermissions[groupIdStr] = permissionFlags;

    *managerOut = this->thisInterface<IPermissionManager>();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PermissionManagerImpl::isAuthorized(IUser* user, AccessPermission permission, Bool* authorizedOut)
{
    OPENDAQ_PARAM_NOT_NULL(authorizedOut);

    *authorizedOut = false;
    UserPtr userPtr = user;
    const auto& groups = userPtr.getGroups();
    
    for (const auto& group : groups)
    {
        auto permissionMask = groupPermissions.find(group);

        if (permissionMask == groupPermissions.end())
            continue;

        if ((permissionMask->second & (Int) permission) != 0)
        {
            *authorizedOut = true;
            break;
        }
    }
    
    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, PermissionManager)

END_NAMESPACE_OPENDAQ
