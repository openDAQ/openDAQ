#include <opendaq/permission_config_builder_impl.h>
#include <coretypes/validation.h>
#include <opendaq/permission_config_impl.h>

BEGIN_NAMESPACE_OPENDAQ

PermissionConfigBuilderImpl::PermissionConfigBuilderImpl()
    : inherited(false)
    , allowed(Dict<IString, Int>())
    , denied(Dict<IString, Int>())
{
}

ErrCode INTERFACE_FUNC PermissionConfigBuilderImpl::inherit(Bool inherit)
{
    inherited = inherit;
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PermissionConfigBuilderImpl::set(IString* groupId, Int permissionFlags)
{
    allowed.set(groupId, permissionFlags);
    denied.set(groupId, 0);
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PermissionConfigBuilderImpl::allow(IString* groupId, Int permissionFlags)
{
    Int allowMask = allowed.hasKey(groupId) ? (Int) allowed.get(groupId) : 0;
    Int denyMask = denied.hasKey(groupId) ? (Int) denied.get(groupId) : 0;

    allowMask |= permissionFlags;
    denyMask &= ~permissionFlags;

    allowed.set(groupId, allowMask);
    denied.set(groupId, denyMask);
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PermissionConfigBuilderImpl::deny(IString* groupId, Int permissionFlags)
{
    Int denyMask = denied.hasKey(groupId) ? (Int) denied.get(groupId) : 0;
    Int allowMask = allowed.hasKey(groupId) ? (Int) allowed.get(groupId) : 0;

    denyMask |= permissionFlags;
    allowMask &= ~permissionFlags;

    denied.set(groupId, denyMask);
    allowed.set(groupId, allowMask);
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PermissionConfigBuilderImpl::extend(IPermissionConfig* config)
{
    PermissionConfigPtr configPtr = config;

    for (const auto& [groupId, permissionMask] : configPtr.getAllowed())
        allow(groupId, permissionMask);

    for (const auto& [groupId, permissionMask] : configPtr.getDenied())
        deny(groupId, permissionMask);

    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PermissionConfigBuilderImpl::build(IPermissionConfig** configOut)
{
    OPENDAQ_PARAM_NOT_NULL(configOut);

    PermissionConfigPtr config(createWithImplementation<IPermissionConfig, PermissionConfigImpl>(inherited, allowed, denied));
    *configOut = config.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

END_NAMESPACE_OPENDAQ
