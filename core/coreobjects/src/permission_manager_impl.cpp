#include <coreobjects/permission_manager_impl.h>
#include <coretypes/impl.h>
#include <coretypes/string_ptr.h>
#include <coreobjects/user_ptr.h>
#include <coretypes/validation.h>
#include <coreobjects/permissions_builder_factory.h>

BEGIN_NAMESPACE_OPENDAQ

PermissionManagerImpl::PermissionManagerImpl(const PermissionManagerPtr& parent)
    : children(Dict<IPermissionManager, Bool>())
    , permissions(PermissionsBuilder().inherit(true).build())
    , localPermissions(PermissionsBuilder().inherit(true).build())
{
    setParent(parent);
}

ErrCode INTERFACE_FUNC PermissionManagerImpl::setPermissions(IPermissions* permissions)
{
    localPermissions = permissions;
    auto builder = PermissionsBuilder();

    if (localPermissions.getInherited() && parent.assigned())
    {
        const auto parent = getParentManager();
        const auto parentConfig = parent.getPermissions();
        builder.inherit(true).extend(parentConfig);
    }

    builder.extend(localPermissions);
    this->permissions = builder.build();

    updateChildPermissions();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PermissionManagerImpl::isAuthorized(IUser* user, Permission permission, Bool* authorizedOut)
{
    OPENDAQ_PARAM_NOT_NULL(authorizedOut);
    *authorizedOut = false;

    UserPtr userPtr = user;
    const auto& groups = userPtr.getGroups();
    const Int targetPermissionInt = (Int) permission;
    Int permissionMask;

    for (const auto& group : groups)
    {
        permissionMask = permissions.getDenied().hasKey(group) ? permissions.getDenied().get(group) : 0;

        if ((permissionMask & targetPermissionInt) != 0)
        {
            *authorizedOut = false;
            return OPENDAQ_SUCCESS;
        }
    }

    for (const auto& group : groups)
    {
        permissionMask = permissions.getAllowed().hasKey(group) ? permissions.getAllowed().get(group) : 0;

        if ((permissionMask & targetPermissionInt) != 0)
        {
            *authorizedOut = true;
            return OPENDAQ_SUCCESS;
        }
    }
    
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PermissionManagerImpl::setParent(IPermissionManager* parentManager)
{
    const auto self = borrowPtr<PermissionManagerPtr>();

    if (parent.assigned())
    {
        auto parent = getParentManager();
        parent.removeChildManager(self);
    }

    parent = parentManager;

    if (parent.assigned())
    {
        auto parent = getParentManager();
        parent.addChildManager(self);
    }

    setPermissions(localPermissions);
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PermissionManagerImpl::addChildManager(IPermissionManager* childManager)
{
    children.set(childManager, true);
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PermissionManagerImpl::removeChildManager(IPermissionManager* childManager)
{
    children.remove(childManager);
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PermissionManagerImpl::getPermissions(IPermissions** permisisonConfigOut)
{
    OPENDAQ_PARAM_NOT_NULL(permisisonConfigOut);

    *permisisonConfigOut = permissions.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PermissionManagerImpl::updateInheritedPermissions()
{
    if (localPermissions.getInherited())
        setPermissions(localPermissions);

    return OPENDAQ_SUCCESS;
}

void PermissionManagerImpl::updateChildPermissions()
{
    for (const auto& [child, _] : children)
        child.asPtr<IPermissionManagerInternal>(true)->updateInheritedPermissions();
}

PermissionManagerInternalPtr PermissionManagerImpl::getParentManager()
{
    return parent.getRef().asPtr<IPermissionManagerInternal>();
}

// Factory

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, PermissionManager, IPermissionManager*, parent)

END_NAMESPACE_OPENDAQ
