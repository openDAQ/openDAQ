#include <opendaq/log_file_info_builder_impl.h>
#include <opendaq/log_file_info_builder_ptr.h>
#include <opendaq/log_file_info_impl.h>
#include <coretypes/validation.h>

BEGIN_NAMESPACE_OPENDAQ

LogFileInfoBuilderImpl::LogFileInfoBuilderImpl()
    : encoding(LogFileEncodingType::Unknown)
    , size(0)
{
}

ErrCode LogFileInfoBuilderImpl::build(ILogFileInfo** logFileInfo)
{
    OPENDAQ_PARAM_NOT_NULL(logFileInfo);

    const auto builderPtr = this->borrowPtr<LogFileInfoBuilderPtr>();

    return daqTry([&]
    {
        *logFileInfo = createWithImplementation<ILogFileInfo, LogFileInfoImpl>(builderPtr).detach();
        return OPENDAQ_SUCCESS;
    });
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

ErrCode LogFileInfoBuilderImpl::getId(IString** id)
{
    OPENDAQ_PARAM_NOT_NULL(id);
    *id = this->id.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode LogFileInfoBuilderImpl::setId(IString* id)
{
    this->id = id;
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

ErrCode LogFileInfoBuilderImpl::getSize(SizeT* size)
{
    OPENDAQ_PARAM_NOT_NULL(size);
    *size = this->size;
    return OPENDAQ_SUCCESS;
}

ErrCode LogFileInfoBuilderImpl::setSize(SizeT size)
{
    this->size = size;
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

ErrCode LogFileInfoBuilderImpl::getLastModified(IString** lastModified)
{
    OPENDAQ_PARAM_NOT_NULL(lastModified);
    *lastModified = this->lastModified.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode LogFileInfoBuilderImpl::setLastModified(IString* lastModified)
{
    this->lastModified = lastModified;
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
