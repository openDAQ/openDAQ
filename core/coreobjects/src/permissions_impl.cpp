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
                                 const std::unordered_map<StringPtr, Int, StringHash, StringEqualTo>& allowed,
                                 const std::unordered_map<StringPtr, Int, StringHash, StringEqualTo>& denied,
                                 const std::unordered_map<StringPtr, Int, StringHash, StringEqualTo>& assigned)
    : inherited(inherited)
    , allowed(Dict<IString, Int>())
    , denied(Dict<IString, Int>())
    , assigned(Dict<IString, Int>())
{
    CopyToTarget(allowed, this->allowed);
    CopyToTarget(denied, this->denied);
    CopyToTarget(assigned, this->assigned);
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

void PermissionsImpl::CopyToTarget(const std::unordered_map<StringPtr, Int, StringHash, StringEqualTo>& dict, DictPtr<IString, Int>& target)
{
    for (const auto& [key, val] : dict)
        target.set(key, val);
}

END_NAMESPACE_OPENDAQ
