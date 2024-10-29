#include <coretypes/impl.h>
#include <opendaq/module_info_impl.h>

BEGIN_NAMESPACE_OPENDAQ

ModuleInfoImpl::ModuleInfoImpl(const VersionInfoPtr& versionInfo, const StringPtr& name, const StringPtr& id)
    : versionInfo(versionInfo)
    , name(name)
    , id(id)
{
}

ErrCode INTERFACE_FUNC ModuleInfoImpl::getVersionInfo(IVersionInfo** versionInfo)
{
    if (versionInfo == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *versionInfo = this->versionInfo.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC ModuleInfoImpl::getName(IString** name)
{
    if (name == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *name = this->name.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC ModuleInfoImpl::getId(IString** id)
{
    if (id == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *id = this->id.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, ModuleInfo, IVersionInfo*, versionInfo, IString*, name, IString*, id)

END_NAMESPACE_OPENDAQ
