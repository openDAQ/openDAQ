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

    assigned[groupIdPtr] = permissionFlags;
    allowed[groupIdPtr] = permissionFlags;
    denied[groupIdPtr] = 0;
}

void PermissionsBuilderImpl::allow(IString* groupId, Int permissionFlags)
{
    StringPtr groupIdPtr = groupId;
    Int& allowMask = allowed[groupIdPtr];
    Int& denyMask = denied[groupIdPtr];

    allowMask |= permissionFlags;
    denyMask &= ~permissionFlags;
}

void PermissionsBuilderImpl::deny(IString* groupId, Int permissionFlags)
{
    StringPtr groupIdPtr = groupId;
    Int& allowMask = allowed[groupIdPtr];
    Int& denyMask = denied[groupIdPtr];

    denyMask |= permissionFlags;
    allowMask &= ~permissionFlags;
}

// Factory

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, PermissionsBuilder)

END_NAMESPACE_OPENDAQ
