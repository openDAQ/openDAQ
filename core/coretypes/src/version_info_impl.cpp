#include <coretypes/version_info_impl.h>
#include <coretypes/impl.h>

BEGIN_NAMESPACE_OPENDAQ

#undef major
#undef minor
#undef patch

VersionInfoImpl::VersionInfoImpl(SizeT major, SizeT minor, SizeT patch)
    : major(major)
    , minor(minor)
    , patch(patch)
{
}

ErrCode VersionInfoImpl::getMajor(SizeT* major)
{
    if (major == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *major = this->major;
    return OPENDAQ_SUCCESS;
}

ErrCode VersionInfoImpl::getMinor(SizeT* minor)
{
    if (minor == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *minor = this->minor;
    return OPENDAQ_SUCCESS;
}

ErrCode VersionInfoImpl::getPatch(SizeT* patch)
{
    if (patch == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *patch = this->patch;
    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, VersionInfo, SizeT, major, SizeT, minor, SizeT, patch)

END_NAMESPACE_OPENDAQ
