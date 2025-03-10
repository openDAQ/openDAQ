#include <coreobjects/permissions_impl.h>
#include <coretypes/validation.h>

BEGIN_NAMESPACE_OPENDAQ

PermissionsImpl::PermissionsImpl()
    : inherited(false)
    , allowed(Dict<IString, Int>())
    , denied(Dict<IString, Int>())
{
}

PermissionsImpl::PermissionsImpl(Bool inherited,
                                 const PermissionsImpl::PermissionTable& allowed,
                                 const PermissionsImpl::PermissionTable& denied,
                                 const PermissionsImpl::PermissionTable& assigned)
    : inherited(inherited)
    , allowed(CopyPermissionTable(allowed))
    , denied(CopyPermissionTable(denied))
    , assigned(CopyPermissionTable(assigned))
{
}

ErrCode INTERFACE_FUNC PermissionsImpl::getInherited(Bool* inherited)
{
    OPENDAQ_PARAM_NOT_NULL(inherited);

    *inherited = this->inherited;
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PermissionsImpl::getAllowed(IDict** permissions)
{
    OPENDAQ_PARAM_NOT_NULL(permissions);

    *permissions = allowed.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PermissionsImpl::getDenied(IDict** permissions)
{
    OPENDAQ_PARAM_NOT_NULL(permissions);

    *permissions = denied.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PermissionsImpl::getAssigned(IDict** permissions)
{
    OPENDAQ_PARAM_NOT_NULL(permissions);

    *permissions = assigned.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

DictPtr<IString, Int> PermissionsImpl::CopyPermissionTable(const PermissionsImpl::PermissionTable& dict){
    DictPtr<IString, Int> target = Dict<IString, Int>();
    for (const auto& [key, val] : dict)
        target.set(key, val);
    return target;
}

END_NAMESPACE_OPENDAQ
