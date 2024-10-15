#include <opendaq/log_file_info_builder_impl.h>
#include <coretypes/validation.h>

BEGIN_NAMESPACE_OPENDAQ

LogFileInfoBuilderImpl::LogFileInfoBuilderImpl()
    : encoding(LogFileEncodingType::Utf8)
{
}

ErrCode LogFileInfoBuilderImpl::getLocalPath(IString** localPath)
{
    OPENDAQ_PARAM_NOT_NULL(localPath);
    *localPath = this->localPath.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode LogFileInfoBuilderImpl::setLocalPath(IString* localPath)
{
    this->localPath = localPath;
    return OPENDAQ_SUCCESS;
}

ErrCode LogFileInfoBuilderImpl::getName(IString** name)
{
    OPENDAQ_PARAM_NOT_NULL(name);
    *name = this->name.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode LogFileInfoBuilderImpl::setName(IString* name)
{
    this->name = name;
    return OPENDAQ_SUCCESS;
}

ErrCode LogFileInfoBuilderImpl::getDescription(IString** description)
{
    OPENDAQ_PARAM_NOT_NULL(description);
    *description = this->description.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode LogFileInfoBuilderImpl::setDescription(IString* description)
{
    this->description = description;
    return OPENDAQ_SUCCESS;
}

ErrCode LogFileInfoBuilderImpl::getEncoding(LogFileEncodingType* encoding)
{
    OPENDAQ_PARAM_NOT_NULL(encoding);
    *encoding = this->encoding;
    return OPENDAQ_SUCCESS;
}

ErrCode LogFileInfoBuilderImpl::setEncoding(LogFileEncodingType encoding)
{
    this->encoding = encoding;
    return OPENDAQ_SUCCESS;
}

#if !defined(BUILDING_STATIC_LIBRARY)

extern "C"
ErrCode PUBLIC_EXPORT createLogFileInfoBuilder(ILogFileInfoBuilder** objTmp)
{
    return daq::createObject<ILogFileInfoBuilder, LogFileInfoBuilderImpl>(objTmp);
}

#endif

END_NAMESPACE_OPENDAQ
