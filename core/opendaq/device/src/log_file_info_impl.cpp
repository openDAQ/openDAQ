

#include <opendaq/log_file_info_impl.h>
#include <coretypes/validation.h>
#include <opendaq/log_file_info_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

LogFileInfoImpl::LogFileInfoImpl(const LogFileInfoBuilderPtr& builder)
{
    if (builder == nullptr)
        throw InvalidParameterException();

    localPath = builder.getLocalPath();
    name = builder.getName();
    id = builder.getId();
    description = builder.getDescription();
    encoding = builder.getEncoding();
    size = builder.getSize();
    lastModified = builder.getLastModified();

    if (!name.assigned() || name.getLength() == 0)
        throw InvalidParameterException("Log file name is not assigned or empty.");
    
    if (!lastModified.assigned() || lastModified.getLength() == 0)
        throw InvalidParameterException("Last modified date is not assigned or empty.");

    if (id.assigned() && id.getLength() > 0)
        return;

    if (localPath.assigned())
        id = localPath + "/" + name;
    else
        id = name;
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

ErrCode LogFileInfoImpl::getEncoding(LogFileEncodingType* encoding)
{
    OPENDAQ_PARAM_NOT_NULL(encoding);
    *encoding = this->encoding;
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
    serializer->writeInt(static_cast<Int>(encoding));

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
    Int encoding;
    Int size;
    StringPtr lastModified;

    errCode = serialized->readString(String("name"), &name);
    if (OPENDAQ_FAILED(errCode))
        return errCode;
    errCode = serialized->readInt(String("encoding"), &encoding);
    if (OPENDAQ_FAILED(errCode))
        return errCode;

    Bool hasKey;

    errCode = serialized->hasKey(String("localPath"), &hasKey);
    if (OPENDAQ_FAILED(errCode))
        return errCode;

    if (hasKey)
    {
        errCode = serialized->readString(String("localPath"), &localPath);
        if (OPENDAQ_FAILED(errCode))
            return errCode;
    }

    errCode = serialized->readString(String("id"), &id);
    if (OPENDAQ_FAILED(errCode))
        return errCode;

    errCode = serialized->hasKey(String("description"), &hasKey);
    if (OPENDAQ_FAILED(errCode))
        return errCode;
    
    if (hasKey)
    {
        errCode = serialized->readString(String("description"), &description);
        if (OPENDAQ_FAILED(errCode))
            return errCode;
    }

    errCode = serialized->readInt(String("size"), &size);
    if (OPENDAQ_FAILED(errCode))
        return errCode;

    errCode = serialized->readString(String("lastModified"), &lastModified);
    if (OPENDAQ_FAILED(errCode))
        return errCode;

    LogFileInfoBuilderPtr infoBuilder = LogFileInfoBuilder_Create();
    LogFileInfoPtr info = infoBuilder.setName(name)
                                     .setLocalPath(localPath)
                                     .setId(id)
                                     .setDescription(description)
                                     .setEncoding(static_cast<LogFileEncodingType>(encoding))
                                     .setSize(size)
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

