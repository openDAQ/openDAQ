#include <opendaq/permissions_impl.h>
#include <coretypes/validation.h>

BEGIN_NAMESPACE_OPENDAQ

PermissionsImpl::PermissionsImpl()
    : inherited(false)
    , allowed(Dict<IString, Int>())
    , denied(Dict<IString, Int>())
{
}

PermissionsImpl::PermissionsImpl(Bool inherited, const DictPtr<IString, Int>& allowed, const DictPtr<IString, Int>& denied)
    : inherited(inherited)
    , allowed(cloneDict(allowed))
    , denied(cloneDict(denied))
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

DictPtr<IString, Int> PermissionsImpl::cloneDict(const DictPtr<IString, Int>& dict)
{
    auto clone = Dict<IString, Int>();

    for (const auto& [key, val] : dict)
        clone.set(key, val);

    return clone;
}

END_NAMESPACE_OPENDAQ
