#include <coreobjects/permissions_builder_impl.h>
#include <coretypes/validation.h>
#include <coreobjects/permissions_impl.h>

BEGIN_NAMESPACE_OPENDAQ

PermissionsBuilderImpl::PermissionsBuilderImpl()
    : inherited(false)
    , allowed(Dict<IString, Int>())
    , denied(Dict<IString, Int>())
{
}

ErrCode INTERFACE_FUNC PermissionsBuilderImpl::inherit(Bool inherit)
{
    inherited = inherit;
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PermissionsBuilderImpl::set(IString* groupId, Int permissionFlags)
{
    allowed.set(groupId, permissionFlags);
    denied.set(groupId, 0);
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PermissionsBuilderImpl::allow(IString* groupId, Int permissionFlags)
{
    Int allowMask = allowed.hasKey(groupId) ? (Int) allowed.get(groupId) : 0;
    Int denyMask = denied.hasKey(groupId) ? (Int) denied.get(groupId) : 0;

    allowMask |= permissionFlags;
    denyMask &= ~permissionFlags;

    allowed.set(groupId, allowMask);
    denied.set(groupId, denyMask);
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PermissionsBuilderImpl::deny(IString* groupId, Int permissionFlags)
{
    Int denyMask = denied.hasKey(groupId) ? (Int) denied.get(groupId) : 0;
    Int allowMask = allowed.hasKey(groupId) ? (Int) allowed.get(groupId) : 0;

    denyMask |= permissionFlags;
    allowMask &= ~permissionFlags;

    denied.set(groupId, denyMask);
    allowed.set(groupId, allowMask);
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PermissionsBuilderImpl::extend(IPermissions* config)
{
    PermissionsPtr configPtr = config;

    for (const auto& [groupId, permissionMask] : configPtr.getAllowed())
        allow(groupId, permissionMask);

    for (const auto& [groupId, permissionMask] : configPtr.getDenied())
        deny(groupId, permissionMask);

    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PermissionsBuilderImpl::build(IPermissions** configOut)
{
    OPENDAQ_PARAM_NOT_NULL(configOut);

    PermissionsPtr config(createWithImplementation<IPermissions, PermissionsImpl>(inherited, allowed, denied));
    *configOut = config.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

// Factory

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, PermissionsBuilder)

END_NAMESPACE_OPENDAQ
