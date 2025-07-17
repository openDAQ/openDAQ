#include <coretypes/errorinfo_impl.h>
#include <coretypes/impl.h>
#include <coretypes/freezable.h>

BEGIN_NAMESPACE_OPENDAQ

thread_local ErrorInfoHolder errorInfoHolder;

// ErrorInfoHolder

// mingw has a bug which causes segmentation fault on thread_local destructor
// disabling produces memory leak on mingw on thread exit if errorInfo object is assigned
#ifndef __MINGW32__
ErrorInfoHolder::~ErrorInfoHolder()
{
    releaseRefIfNotNull(errorInfoList);
}
#endif

void ErrorInfoHolder::setErrorInfo(IErrorInfo* errorInfo)
{
    if (errorInfo == nullptr)
    {
        releaseRefIfNotNull(errorInfoList);
        errorInfoList = nullptr;
        return;
    }

    if (errorInfoList == nullptr)
        errorInfoList = ListWithElementType_Create(IErrorInfo::Id);
    errorInfoList->pushBack(errorInfo);
}

IErrorInfo* ErrorInfoHolder::getErrorInfo() const
{
    if (errorInfoList == nullptr)
        return nullptr;
    
    SizeT count = 0;
    errorInfoList->getCount(&count);
    if (count == 0)
        return nullptr;
    
    IBaseObject* errorInfoObject;
    errorInfoList->getItemAt(count - 1, &errorInfoObject);

    if (errorInfoObject == nullptr)
        return nullptr;

    IErrorInfo* errorInfo;
    errorInfoObject->borrowInterface(IErrorInfo::Id, reinterpret_cast<void**>(&errorInfo));

    return errorInfo;
}

IList* ErrorInfoHolder::getErrorInfoList()
{
    IList* tmp = errorInfoList;
    errorInfoList = nullptr;
    return tmp;
}

// ErrorInfoImpl

ErrorInfoImpl::ErrorInfoImpl()
    : message(nullptr)
    , source(nullptr)
    , fileName(nullptr)
    , line(-1)
    , frozen(False)
    , fileNameObject(nullptr)
{
}

ErrorInfoImpl::~ErrorInfoImpl()
{
    releaseRefIfNotNull(message);
    releaseRefIfNotNull(source);
    releaseRefIfNotNull(fileNameObject);
}

ErrCode ErrorInfoImpl::getMessage(IString** message)
{
    if (message == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *message = this->message;
    addRefIfNotNull(*message);

    return OPENDAQ_SUCCESS;
}

ErrCode ErrorInfoImpl::setMessage(IString* message)
{
    if (frozen)
        return OPENDAQ_ERR_FROZEN;

    releaseRefIfNotNull(this->message);
    this->message = message;
    addRefIfNotNull(this->message);

    return OPENDAQ_SUCCESS;
}

ErrCode ErrorInfoImpl::getSource(IString** source)
{
    if (source == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *source = this->source;
    addRefIfNotNull(*source);

    return OPENDAQ_SUCCESS;
}

ErrCode ErrorInfoImpl::setFileName(ConstCharPtr fileName)
{
    if (frozen)
        return OPENDAQ_ERR_FROZEN;

    this->fileName = fileName;

    return OPENDAQ_SUCCESS;
}

ErrCode ErrorInfoImpl::getFileName(ConstCharPtr* fileName)
{
    if (fileName == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *fileName = this->fileName;

    return OPENDAQ_SUCCESS;
}

ErrCode ErrorInfoImpl::setFileLine(Int line)
{
    if (frozen)
        return OPENDAQ_ERR_FROZEN;

    this->line = line;

    return OPENDAQ_SUCCESS;
}

ErrCode ErrorInfoImpl::getFileLine(Int* line)
{
    if (line == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *line = this->line;

    return OPENDAQ_SUCCESS;
}

ErrCode ErrorInfoImpl::setSource(IString* source)
{
    if (frozen)
        return OPENDAQ_ERR_FROZEN;

    releaseRefIfNotNull(this->source);
    this->source = source;
    addRefIfNotNull(this->source);

    return OPENDAQ_SUCCESS;
}

ErrCode ErrorInfoImpl::isFrozen(Bool* frozen) const
{
    if (frozen == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *frozen = this->frozen;

    return OPENDAQ_SUCCESS;
}

ErrCode ErrorInfoImpl::freeze()
{
    if (frozen)
        return OPENDAQ_IGNORED;

    this->frozen = true;

    return OPENDAQ_SUCCESS;
}

ErrCode ErrorInfoImpl::serialize(ISerializer* serializer)
{
    OPENDAQ_PARAM_NOT_NULL(serializer);

    serializer->startTaggedObject(this);
    if (message)
    {
        ConstCharPtr messageCharPtr;
        SizeT messageLenght;
        ErrCode errCode = message->getCharPtr(&messageCharPtr);
        OPENDAQ_RETURN_IF_FAILED(errCode);
        OPENDAQ_PARAM_NOT_NULL(messageCharPtr);
        errCode = message->getLength(&messageLenght);
        OPENDAQ_RETURN_IF_FAILED(errCode);
        serializer->key("message");
        serializer->writeString(messageCharPtr, messageLenght);
    }
    if (source)
    {
        ConstCharPtr sourceCharPtr;
        SizeT sourceLenght;
        ErrCode errCode = source->getCharPtr(&sourceCharPtr);
        OPENDAQ_RETURN_IF_FAILED(errCode);
        OPENDAQ_PARAM_NOT_NULL(sourceCharPtr);
        errCode = source->getLength(&sourceLenght);
        OPENDAQ_RETURN_IF_FAILED(errCode);
        serializer->key("source");
        serializer->writeString(sourceCharPtr, sourceLenght);
    }
    if (fileName && std::strlen(fileName))
    {
        serializer->key("fileName");
        serializer->writeString(fileName, std::strlen(fileName));
    }
    if (line != -1)
    {
        serializer->key("fileLine");
        serializer->writeInt(line);
    }
    serializer->endObject();

    return OPENDAQ_SUCCESS;
}

ErrCode ErrorInfoImpl::getSerializeId(ConstCharPtr* id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = SerializeId();

    return OPENDAQ_SUCCESS;
}

ConstCharPtr ErrorInfoImpl::SerializeId()
{
    return "ErrorInfo";
}

ErrCode ErrorInfoImpl::Deserialize(ISerializedObject* serialized, IBaseObject* /*context*/, IFunction* /* factoryCallback*/, IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(serialized);
    OPENDAQ_PARAM_NOT_NULL(obj);

    IErrorInfo* errorInfo = nullptr;
    IString* message = nullptr;
    IString* source = nullptr;
    IString* fileName = nullptr;
    IString* messageKey = nullptr;
    IString* sourceKey = nullptr;
    IString* fileNameKey = nullptr;
    IString* fileLineKey = nullptr;
    Finally final([&errorInfo, &message, &source, &fileName, &messageKey, &sourceKey, &fileNameKey, &fileLineKey]
                  {
                      if (errorInfo != nullptr)
                          errorInfo->releaseRef();
                      if (message != nullptr)
                          message->releaseRef();
                      if (source != nullptr)
                          source->releaseRef();
                      if (fileName != nullptr)
                          fileName->releaseRef();
                      if (messageKey != nullptr)
                          messageKey->releaseRef();
                      if (sourceKey != nullptr)
                          sourceKey->releaseRef();
                      if (fileNameKey != nullptr)
                          fileNameKey->releaseRef();
                      if (fileLineKey != nullptr)
                          fileLineKey->releaseRef();
                  });

    ErrCode errCode = createErrorInfo(&errorInfo);
    OPENDAQ_RETURN_IF_FAILED(errCode);
    OPENDAQ_PARAM_NOT_NULL(errorInfo);

    Bool hasKey = False;

    errCode = createString(&messageKey, "message");
    OPENDAQ_RETURN_IF_FAILED(errCode);
    OPENDAQ_PARAM_NOT_NULL(messageKey);
    errCode = serialized->hasKey(messageKey, &hasKey);
    OPENDAQ_RETURN_IF_FAILED(errCode);
    if (hasKey)
    {
        errCode = serialized->readString(messageKey, &message);
        OPENDAQ_RETURN_IF_FAILED(errCode);
        errCode = errorInfo->setMessage(message);
        OPENDAQ_RETURN_IF_FAILED(errCode);
    }

    errCode = createString(&sourceKey, "source");
    OPENDAQ_RETURN_IF_FAILED(errCode);
    OPENDAQ_PARAM_NOT_NULL(sourceKey);
    errCode = serialized->hasKey(sourceKey, &hasKey);
    OPENDAQ_RETURN_IF_FAILED(errCode);
    if (hasKey)
    {
        errCode = serialized->readString(sourceKey, &source);
        OPENDAQ_RETURN_IF_FAILED(errCode);
        errCode = errorInfo->setSource(source);
        OPENDAQ_RETURN_IF_FAILED(errCode);
    }

    errCode = createString(&fileNameKey, "fileName");
    OPENDAQ_RETURN_IF_FAILED(errCode);
    OPENDAQ_PARAM_NOT_NULL(fileNameKey);
    errCode = serialized->hasKey(fileNameKey, &hasKey);
    OPENDAQ_RETURN_IF_FAILED(errCode);
    if (hasKey)
    {
        errCode = serialized->readString(fileNameKey, &fileName);
        OPENDAQ_RETURN_IF_FAILED(errCode);
        OPENDAQ_PARAM_NOT_NULL(fileName);
        ConstCharPtr fileNameCharPtr;
        errCode = fileName->getCharPtr(&fileNameCharPtr);
        OPENDAQ_RETURN_IF_FAILED(errCode);
        errCode = errorInfo->setFileName(fileNameCharPtr);
        OPENDAQ_RETURN_IF_FAILED(errCode);
        auto implPtr = static_cast<ErrorInfoImpl*>(errorInfo);
        implPtr->setFileNameObject(fileName);
    }

    errCode = createString(&fileLineKey, "fileLine");
    OPENDAQ_RETURN_IF_FAILED(errCode);
    OPENDAQ_PARAM_NOT_NULL(fileLineKey);
    errCode = serialized->hasKey(fileLineKey, &hasKey);
    OPENDAQ_RETURN_IF_FAILED(errCode);
    if (hasKey)
    {
        Int fileLine;
        errCode = serialized->readInt(fileLineKey, &fileLine);
        OPENDAQ_RETURN_IF_FAILED(errCode);
        errCode = errorInfo->setFileLine(fileLine);
        OPENDAQ_RETURN_IF_FAILED(errCode);
    }

    errorInfo->addRef();
    *obj = errorInfo;

    return OPENDAQ_SUCCESS;
}

void ErrorInfoImpl::setFileNameObject(IString* fileNameObj)
{
    releaseRefIfNotNull(this->fileNameObject);
    this->fileNameObject = fileNameObj;
    addRefIfNotNull(this->fileNameObject);
}

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(ErrorInfoImpl)
OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, ErrorInfo)

END_NAMESPACE_OPENDAQ

extern "C"
void PUBLIC_EXPORT daqSetErrorInfo(daq::IErrorInfo* errorInfo)
{
    if (errorInfo != nullptr)
    {
        daq::IFreezable* freezable;
        if (OPENDAQ_SUCCEEDED(errorInfo->borrowInterface(daq::IFreezable::Id, reinterpret_cast<void**>(&freezable))))
            freezable->freeze();
    }
    daq::errorInfoHolder.setErrorInfo(errorInfo);
}

extern "C"
void PUBLIC_EXPORT daqGetErrorInfo(daq::IErrorInfo** errorInfo)
{
    if (errorInfo == nullptr)
        return;

    *errorInfo = daq::errorInfoHolder.getErrorInfo();
}

extern "C" 
void PUBLIC_EXPORT daqGetErrorInfoList(daq::IList** errorInfoList)
{
    if (errorInfoList == nullptr)
        return;

    *errorInfoList = daq::errorInfoHolder.getErrorInfoList();
}

extern "C"
void PUBLIC_EXPORT daqClearErrorInfo()
{
    daq::errorInfoHolder.setErrorInfo(nullptr);
}
