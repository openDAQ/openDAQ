#include <coreobjects/permissions_builder_impl.h>
#include <coretypes/validation.h>
#include <coreobjects/permissions_impl.h>
#include <coreobjects/permission_mask_builder_ptr.h>
#include <coreobjects/permissions_internal_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

PermissionsBuilderImpl::PermissionsBuilderImpl()
    : inherited(false)
{
}

ErrCode INTERFACE_FUNC PermissionsBuilderImpl::inherit(Bool inherit)
{
    inherited = inherit;
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PermissionsBuilderImpl::assign(IString* groupId, IPermissionMaskBuilder* permissions)
{
    OPENDAQ_PARAM_NOT_NULL(permissions);
    OPENDAQ_PARAM_NOT_NULL(groupId);

    Int permissionFlags;
    ErrCode err = permissions->build(&permissionFlags);
    if (OPENDAQ_FAILED(err))
        return err;

    assign(groupId, permissionFlags);
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PermissionsBuilderImpl::allow(IString* groupId, IPermissionMaskBuilder* permissions)
{
    OPENDAQ_PARAM_NOT_NULL(permissions);
    OPENDAQ_PARAM_NOT_NULL(groupId);

    Int permissionFlags;
    ErrCode err = permissions->build(&permissionFlags);
    if (OPENDAQ_FAILED(err))
        return err;

    allow(groupId, permissionFlags);
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PermissionsBuilderImpl::deny(IString* groupId, IPermissionMaskBuilder* permissions)
{
    OPENDAQ_PARAM_NOT_NULL(permissions);
    OPENDAQ_PARAM_NOT_NULL(groupId);

    Int permissionFlags;
    ErrCode err = permissions->build(&permissionFlags);
    if (OPENDAQ_FAILED(err))
        return err;

    deny(groupId, permissionFlags);
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PermissionsBuilderImpl::extend(IPermissions* config)
{
    OPENDAQ_PARAM_NOT_NULL(config);
    const PermissionsPtr configPtr = config;

    for (const auto& [groupId, permissionMask] : configPtr.asPtr<IPermissionsInternal>().getAssigned())
        assign(groupId, permissionMask);

    for (const auto& [groupId, permissionMask] : configPtr.getAllowed())
        allow(groupId, permissionMask);

    for (const auto& [groupId, permissionMask] : configPtr.getDenied())
        deny(groupId, permissionMask);

    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PermissionsBuilderImpl::build(IPermissions** configOut)
{
    OPENDAQ_PARAM_NOT_NULL(configOut);

    PermissionsPtr config(createWithImplementation<IPermissions, PermissionsImpl>(inherited, allowed, denied, assigned));
    *configOut = config.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

void PermissionsBuilderImpl::assign(IString* groupId, Int permissionFlags)
{
    StringPtr groupIdPtr = groupId;
    assigned.insert(std::make_pair(groupIdPtr, permissionFlags));
    allowed.insert(std::make_pair(groupIdPtr, permissionFlags));
    denied.insert(std::make_pair(groupIdPtr, permissionFlags));
}

void PermissionsBuilderImpl::allow(IString* groupId, Int permissionFlags)
{
    StringPtr groupIdPtr = groupId;
    Int allowMask = allowed.count(groupId) ? allowed.at(groupId) : 0;
    Int denyMask = denied.count(groupId) ? denied.at(groupId) : 0;

    allowMask |= permissionFlags;
    denyMask &= ~permissionFlags;

    allowed.insert(std::make_pair(groupIdPtr, allowMask));
    denied.insert(std::make_pair(groupIdPtr, denyMask));
}

void PermissionsBuilderImpl::deny(IString* groupId, Int permissionFlags)
{
    StringPtr groupIdPtr = groupId;
    Int denyMask = denied.count(groupId) ? denied.at(groupId) : 0;
    Int allowMask = allowed.count(groupId) ? allowed.at(groupId) : 0;

    denyMask |= permissionFlags;
    allowMask &= ~permissionFlags;
    
    denied.insert(std::make_pair(groupIdPtr, denyMask));
    allowed.insert(std::make_pair(groupIdPtr, allowMask));
}

// Factory

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, PermissionsBuilder)

END_NAMESPACE_OPENDAQ
