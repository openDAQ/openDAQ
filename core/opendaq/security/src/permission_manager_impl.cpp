#include <opendaq/permission_manager_impl.h>
#include <coretypes/impl.h>
#include <coretypes/string_ptr.h>
#include <opendaq/user_ptr.h>
#include <coretypes/validation.h>
#include <opendaq/permission_config_builder_factory.h>

BEGIN_NAMESPACE_OPENDAQ

PermissionManagerImpl::PermissionManagerImpl(const PermissionManagerPtr& parent)
    : children(Dict<IPermissionManager, Bool>())
    , config(PermissionConfigBuilder().inherit(true).build())
    , localConfig(PermissionConfigBuilder().inherit(true).build())
{
    setParent(parent);
}

ErrCode INTERFACE_FUNC PermissionManagerImpl::setPermissionConfig(IPermissionConfig* permissionConfig)
{
    localConfig = permissionConfig;
    auto builder = PermissionConfigBuilder();

    if (localConfig.getInherited() && parent.assigned())
    {
        const auto parent = getParentManager();
        const auto parentConfig = parent.getPermissionConfig();
        builder.inherit(true).extend(parentConfig);
    }

    builder.extend(localConfig);
    config = builder.build();

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
        permissionMask = 0;

        if (config.getAllowed().hasKey(group))
            permissionMask |= (Int) config.getAllowed().get(group);
        if (config.getDenied().hasKey(group))
            permissionMask &= ~(Int) config.getDenied().get(group);

        if ((permissionMask & targetPermissionInt) != 0)
        {
            *authorizedOut = true;
            break;
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

    setPermissionConfig(localConfig);
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

ErrCode INTERFACE_FUNC PermissionManagerImpl::getPermissionConfig(IPermissionConfig** permisisonConfigOut)
{
    OPENDAQ_PARAM_NOT_NULL(permisisonConfigOut);

    *permisisonConfigOut = config.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PermissionManagerImpl::updateInheritedPermissions()
{
    if (localConfig.getInherited())
        setPermissionConfig(localConfig);

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

END_NAMESPACE_OPENDAQ
