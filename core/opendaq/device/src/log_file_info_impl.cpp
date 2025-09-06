

#include <opendaq/log_file_info_impl.h>
#include <coretypes/validation.h>
#include <opendaq/log_file_info_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

LogFileInfoImpl::LogFileInfoImpl(const LogFileInfoBuilderPtr& builder)
{
    if (builder == nullptr)
        DAQ_THROW_EXCEPTION(InvalidParameterException);

    localPath = builder.getLocalPath();
    name = builder.getName();
    id = builder.getId();
    description = builder.getDescription();
    encoding = builder.getEncoding();
    size = builder.getSize();
    lastModified = builder.getLastModified();

    if (!name.assigned() || name.getLength() == 0)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Log file name is not assigned or empty.");
    
    if (!lastModified.assigned() || lastModified.getLength() == 0)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Last modified date is not assigned or empty.");

    if (id.assigned() && id.getLength() > 0)
        return;

    if (localPath.assigned())
        id = localPath + "/" + name;
    else
        id = name;

    if (!encoding.assigned())
        encoding = "";
}

ErrCode LogFileInfoImpl::getId(IString** id)
{
    OPENDAQ_PARAM_NOT_NULL(id);
    *id = this->id.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode LogFileInfoImpl::getLocalPath(IString** localPath)
{
    OPENDAQ_PARAM_NOT_NULL(localPath);
    *localPath = this->localPath.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode LogFileInfoImpl::getName(IString** name)
{
    OPENDAQ_PARAM_NOT_NULL(name);
    *name = this->name.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode LogFileInfoImpl::getDescription(IString** description)
{
    OPENDAQ_PARAM_NOT_NULL(description);
    *description = this->description.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode LogFileInfoImpl::getSize(SizeT* size)
{
    OPENDAQ_PARAM_NOT_NULL(size);
    *size = this->size;
    return OPENDAQ_SUCCESS;
}

ErrCode LogFileInfoImpl::getEncoding(IString** encoding)
{
    OPENDAQ_PARAM_NOT_NULL(encoding);
    *encoding = this->encoding.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode LogFileInfoImpl::getLastModified(IString** lastModified)
{
    OPENDAQ_PARAM_NOT_NULL(lastModified);
    *lastModified = this->lastModified.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode LogFileInfoImpl::getSerializeId(ConstCharPtr* id) const
{
    *id = SerializeId();
    return OPENDAQ_SUCCESS;
}

ConstCharPtr LogFileInfoImpl::SerializeId()
{
    return "LogFileInfo";
}

ErrCode LogFileInfoImpl::serialize(ISerializer* serializer)
{
    serializer->startTaggedObject(this);

    serializer->key("name");
    serializer->writeString(name.getCharPtr(), name.getLength());

    if (localPath.assigned())
    {
        serializer->key("localPath");
        serializer->writeString(localPath.getCharPtr(), localPath.getLength());
    }

    serializer->key("id");
    serializer->writeString(id.getCharPtr(), id.getLength());

    serializer->key("encoding");
    serializer->writeString(encoding.getCharPtr(), encoding.getLength());

    if (description.assigned())
    {
        serializer->key("description");
        serializer->writeString(description.getCharPtr(), description.getLength());
    }

    serializer->key("size");
    serializer->writeInt(size);

    serializer->key("lastModified");
    serializer->writeString(lastModified.getCharPtr(), lastModified.getLength());

    serializer->endObject();
    return OPENDAQ_SUCCESS;

}

ErrCode LogFileInfoImpl::Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj)
{
    ErrCode errCode;
    StringPtr name;
    StringPtr localPath;
    StringPtr id;
    StringPtr description;
    StringPtr encoding;
    Int size;
    StringPtr lastModified;

    errCode = serialized->readString(String("name"), &name);
    OPENDAQ_RETURN_IF_FAILED(errCode);
    errCode = serialized->readString(String("encoding"), &encoding);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    Bool hasKey;

    errCode = serialized->hasKey(String("localPath"), &hasKey);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    if (hasKey)
    {
        errCode = serialized->readString(String("localPath"), &localPath);
        OPENDAQ_RETURN_IF_FAILED(errCode);
    }

    errCode = serialized->readString(String("id"), &id);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    errCode = serialized->hasKey(String("description"), &hasKey);
    OPENDAQ_RETURN_IF_FAILED(errCode);
    
    if (hasKey)
    {
        errCode = serialized->readString(String("description"), &description);
        OPENDAQ_RETURN_IF_FAILED(errCode);
    }

    errCode = serialized->readInt(String("size"), &size);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    errCode = serialized->readString(String("lastModified"), &lastModified);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    LogFileInfoBuilderPtr infoBuilder = LogFileInfoBuilder_Create();
    LogFileInfoPtr info = infoBuilder.setName(name)
                                     .setLocalPath(localPath)
                                     .setId(id)
                                     .setDescription(description)
                                     .setEncoding(encoding)
                                     .setSize(static_cast<SizeT>(size))
                                     .setLastModified(lastModified)
                                     .build();
    *obj = info.detach();
    return OPENDAQ_SUCCESS;
}

#if !defined(BUILDING_STATIC_LIBRARY)

extern "C"
ErrCode PUBLIC_EXPORT createLogFileInfoFromBuilder(ILogFileInfo** objTmp, ILogFileInfoBuilder* builder)
{
    return daq::createObject<ILogFileInfo, LogFileInfoImpl>(objTmp, builder);
}

#endif

END_NAMESPACE_OPENDAQ

