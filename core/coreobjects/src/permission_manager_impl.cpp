#include <coreobjects/permission_manager_impl.h>
#include <coretypes/impl.h>
#include <coretypes/string_ptr.h>
#include <coreobjects/user_ptr.h>
#include <coretypes/validation.h>
#include <coreobjects/permissions_builder_factory.h>
#include <coreobjects/permission_manager_factory.h>
#include <iostream>

BEGIN_NAMESPACE_OPENDAQ

namespace detail
{
    static const auto DefaultPermissions = PermissionsBuilder().inherit(true).build();
}

// PermissionManagerImpl

PermissionManagerImpl::PermissionManagerImpl(const PermissionManagerPtr& parent)
    : permissions(PermissionsBuilder().inherit(true).build())
    , localPermissions(PermissionsBuilder().inherit(true).build())
{
    if (parent.assigned())
        setParent(parent);
}

PermissionManagerImpl::~PermissionManagerImpl()
{
    if (const auto parent = getParentManager(); parent.assigned())
    {
        const auto self = borrowPtr<PermissionManagerPtr>();
        parent.removeChildManager(self);
    }
}

ErrCode INTERFACE_FUNC PermissionManagerImpl::setPermissions(IPermissions* permissions)
{
    localPermissions = permissions;

    if (localPermissions.getInherited())
    {
        auto builder = PermissionsBuilder();
        if (const auto parent = getParentManager(); parent.assigned())
        {
            const auto parentConfig = parent.getPermissions();
            builder.inherit(true).extend(parentConfig);
        }

        builder.extend(localPermissions);
        this->permissions = builder.build();
    }
    else
    {
        this->permissions = localPermissions;
    }

    updateChildPermissions();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PermissionManagerImpl::isAuthorized(IUser* user, Permission permission, Bool* authorizedOut)
{
    OPENDAQ_PARAM_NOT_NULL(authorizedOut);
    *authorizedOut = false;

    const UserPtr userPtr = UserPtr::Borrow(user);
    const auto& groups = userPtr.getGroups();
    const Int targetPermissionInt = (Int) permission;
    Int permissionMask;

    for (const auto& group : groups)
    {
        permissionMask = (Int) permissions.getDenied().getOrDefault(group, Int(0));

        if ((permissionMask & targetPermissionInt) != 0)
        {
            *authorizedOut = false;
            return OPENDAQ_SUCCESS;
        }
    }

    for (const auto& group : groups)
    {
        permissionMask = (Int) permissions.getAllowed().getOrDefault(group, Int(0));

        if ((permissionMask & targetPermissionInt) != 0)
        {
            *authorizedOut = true;
            return OPENDAQ_SUCCESS;
        }
    }
    
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PermissionManagerImpl::clone(IBaseObject** cloneOut)
{
    PermissionManagerPtr cloneParent = getParentManager();
    auto manager = PermissionManager(cloneParent);
    manager.setPermissions(localPermissions);
    *cloneOut = manager.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PermissionManagerImpl::setParent(IPermissionManager* parentManager)
{
    const auto self = borrowPtr<PermissionManagerPtr>();

    if (const auto parent = getParentManager(); parent.assigned())
        parent.removeChildManager(self);

    parent = parentManager;

    if (const auto parent = getParentManager(); parent.assigned())
        parent.addChildManager(self);

    setPermissions(localPermissions);
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PermissionManagerImpl::addChildManager(IPermissionManager* childManager)
{
    children.insert(childManager);
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PermissionManagerImpl::removeChildManager(IPermissionManager* childManager)
{
    children.erase(childManager);
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
    for (auto child : children)
    {
        PermissionManagerPtr childPtr = child;
        childPtr.asPtr<IPermissionManagerInternal>(true)->updateInheritedPermissions();
    }
}

PermissionManagerInternalPtr PermissionManagerImpl::getParentManager()
{
    if (parent.assigned()) 
    {
        const auto parentPtr = parent.getRef();
        if (parentPtr.assigned())
            return parentPtr.asPtr<IPermissionManagerInternal>().detach();
    }
    return nullptr;
}

// DisabledPermissionManagerImpl

DisabledPermissionManagerImpl::DisabledPermissionManagerImpl()
{
}

ErrCode DisabledPermissionManagerImpl::setPermissions(IPermissions* /*permissions*/)
{
    return OPENDAQ_SUCCESS;
}

ErrCode DisabledPermissionManagerImpl::isAuthorized(IUser* /*user*/, Permission /*permission*/, Bool* authorizedOut)
{
    OPENDAQ_PARAM_NOT_NULL(authorizedOut);
    *authorizedOut = true;
    return OPENDAQ_SUCCESS;
}

ErrCode DisabledPermissionManagerImpl::clone(IBaseObject** cloneOut)
{
    auto manager = DisabledPermissionManager();
    *cloneOut = manager.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode DisabledPermissionManagerImpl::setParent(IPermissionManager* /*parentManager*/)
{
    return OPENDAQ_SUCCESS;
}

ErrCode DisabledPermissionManagerImpl::addChildManager(IPermissionManager* /*childManager*/)
{
    return OPENDAQ_SUCCESS;
}

ErrCode DisabledPermissionManagerImpl::removeChildManager(IPermissionManager* /*childManager*/)
{
    return OPENDAQ_SUCCESS;
}

ErrCode DisabledPermissionManagerImpl::getPermissions(IPermissions** /*permisisonConfigOut*/)
{
    return OPENDAQ_SUCCESS;
}

ErrCode DisabledPermissionManagerImpl::updateInheritedPermissions()
{
    return OPENDAQ_SUCCESS;
}

// Factories

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, PermissionManager, IPermissionManager*, parent)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(LIBRARY_FACTORY,
                                                               DisabledPermissionManagerImpl,
                                                               IPermissionManager,
                                                               createDisabledPermissionManager)

END_NAMESPACE_OPENDAQ
