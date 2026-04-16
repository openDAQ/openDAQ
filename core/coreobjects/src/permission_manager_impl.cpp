#include <coreobjects/permission_manager_impl.h>
#include <coretypes/impl.h>
#include <coretypes/string_ptr.h>
#include <coreobjects/user_ptr.h>
#include <coretypes/validation.h>
#include <coreobjects/permissions_builder_factory.h>
#include <coreobjects/permission_manager_factory.h>

BEGIN_NAMESPACE_OPENDAQ

#ifdef OPENDAQ_ENABLE_ACCESS_CONTROL

namespace detail
{
    static const auto DefaultPermissions = []()
    {
        daqDisableObjectTracking();
        auto permissions = PermissionsBuilder().inherit(true).build();
        daqEnableObjectTracking();
        return permissions;
    }();
}

PermissionManagerImpl::PermissionManagerImpl(const PermissionManagerPtr& parent)
    : permissions(detail::DefaultPermissions)
    , localPermissions(detail::DefaultPermissions)
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
            builder.inherit(true);

            if (parentConfig.assigned())
                builder.extend(parentConfig);
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

#else

// permission manager which never restricts any access to any object.

PermissionManagerImpl::PermissionManagerImpl(const PermissionManagerPtr& /*parent*/)
{
}

ErrCode PermissionManagerImpl::setPermissions(IPermissions* /*permissions*/)
{
    return OPENDAQ_SUCCESS;
}

ErrCode PermissionManagerImpl::isAuthorized(IUser* /*user*/, Permission /*permission*/, Bool* authorizedOut)
{
    OPENDAQ_PARAM_NOT_NULL(authorizedOut);
    *authorizedOut = true;
    return OPENDAQ_SUCCESS;
}

ErrCode PermissionManagerImpl::clone(IBaseObject** cloneOut)
{
    auto manager = PermissionManager();
    *cloneOut = manager.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode PermissionManagerImpl::setParent(IPermissionManager* /*parentManager*/)
{
    return OPENDAQ_SUCCESS;
}

ErrCode PermissionManagerImpl::addChildManager(IPermissionManager* /*childManager*/)
{
    return OPENDAQ_SUCCESS;
}

ErrCode PermissionManagerImpl::removeChildManager(IPermissionManager* /*childManager*/)
{
    return OPENDAQ_SUCCESS;
}

ErrCode PermissionManagerImpl::getPermissions(IPermissions** /*permisisonConfigOut*/)
{
    return OPENDAQ_SUCCESS;
}

ErrCode PermissionManagerImpl::updateInheritedPermissions()
{
    return OPENDAQ_SUCCESS;
}

#endif

// Factories

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, PermissionManager, IPermissionManager*, parent)

END_NAMESPACE_OPENDAQ
