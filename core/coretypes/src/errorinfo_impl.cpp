#include <coretypes/common.h>
#include <coretypes/errorinfo_impl.h>
#include <coretypes/errors.h>
#include <coretypes/freezable.h>
#include <coretypes/impl.h>
#include <coretypes/string_ptr.h>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <algorithm>

BEGIN_NAMESPACE_OPENDAQ

thread_local ErrorInfoHolder errorInfoHolder;

// InitialErrorGuard
class InitialErrorGuard : public ErrorGuardImpl
{
public:
    using Super = ErrorGuardImpl;

    InitialErrorGuard()
        : ErrorGuardImpl(nullptr, -1)
    {
    }

    ErrCode INTERFACE_FUNC getErrorInfos(IList** errorInfos) override
    {
        const ErrCode errCode = Super::getErrorInfos(errorInfos);
        this->releaseRef();
        return errCode;
    }

    void clearLastErrorInfo(ErrCode errCode) override
    {
        Super::clearLastErrorInfo(errCode);
        if (this->empty())
            this->releaseRef();
    }

    bool isInitial() const override
    {
        return true;
    }
};

// ErrorInfoWrapper

ErrorInfoWrapper::ErrorInfoWrapper(IErrorInfo* errorInfo)
    : errorInfo(errorInfo)
{
    addRefIfNotNull(errorInfo);
}

ErrorInfoWrapper::~ErrorInfoWrapper()
{
    releaseRefIfNotNull(errorInfo);
}

IErrorInfo* ErrorInfoWrapper::getAddRef() const
{
    addRefIfNotNull(errorInfo);
    return errorInfo;
}

IErrorInfo* ErrorInfoWrapper::borrow() const
{
    return errorInfo;
}

// ErrorInfoHolder

ErrorInfoHolder::~ErrorInfoHolder()
{
    if (!errorScopeList || errorScopeList->empty())
        return;

    for (auto& scope : *errorScopeList)
    {
        if (scope->isInitial())
        {
            scope->releaseRef();  // only dummy
            return;
        }
    }
}

ErrorInfoHolder::ContainerT* ErrorInfoHolder::getOrCreateList()
{
    if (!errorScopeList)
    {
        errorScopeList = std::make_unique<ContainerT>();
        auto entry = new InitialErrorGuard();
        entry->addRef();
    }
    return errorScopeList.get();
}

void ErrorInfoHolder::setErrorInfo(IErrorInfo* errorInfo)
{
    if (errorInfo)
        getOrCreateList()->back()->setErrorInfo(errorInfo);
}

void ErrorInfoHolder::extendErrorInfo(IErrorInfo* errorInfo, ErrCode prevErrCode)
{
    if (errorInfo)
        getOrCreateList()->back()->extendErrorInfo(errorInfo, prevErrCode);
}

void ErrorInfoHolder::clearErrorInfo(ErrCode errorCode)
{
    if (errorScopeList)
        errorScopeList->back()->clearLastErrorInfo(errorCode);
}

IErrorInfo* ErrorInfoHolder::getErrorInfo(ErrCode errCode) const
{
    if (errorScopeList)
    {
        IErrorInfo* errorInfo = nullptr;
        errorScopeList->back()->getLastErrorInfo(&errorInfo, errCode);
        return errorInfo;
    }
    return nullptr;
}

IList* ErrorInfoHolder::getErrorInfoList()
{
    if (!errorScopeList)
        return nullptr;

    IList* list = nullptr;
    const ErrCode errCode = errorScopeList->back()->getErrorInfos(&list);
    if (OPENDAQ_FAILED(errCode))
        throw std::runtime_error("Failed to get error info list");
    return list;
}

IString* ErrorInfoHolder::getFormatMessage(ErrCode errCode) const
{
    if (!errorScopeList)
        return nullptr;

    IString* message = nullptr;
    const ErrCode err = errorScopeList->back()->getFormatMessage(&message, errCode);
    if (OPENDAQ_FAILED(err))
        throw std::runtime_error("Failed to get format message");
    return message;
}

void ErrorInfoHolder::setScopeEntry(ErrorGuardImpl* entry)
{
    if (!entry)
        throw std::invalid_argument("ErrorGuardImpl entry must not be null");

    if (!errorScopeList)
        errorScopeList = std::make_unique<ContainerT>();
    errorScopeList->push_back(entry);
}

void ErrorInfoHolder::removeScopeEntry(ErrorGuardImpl* entry)
{
    if (!entry)
    {
        std::cerr << "ErrorGuardImpl entry must not be null" << std::endl;
        return;
    }

    if (!errorScopeList)
    {
        std::cerr << "ErrorGuardImpl scope list is empty. Is order correct?" << std::endl;
        return;
    }

    auto backEntry = errorScopeList->back();
    if (backEntry != entry)
    {
        std::cerr << "ErrorGuard scope entry mismatch. Expected: " << backEntry << ", got: " << entry << std::endl;
        auto it = std::find(errorScopeList->begin(), errorScopeList->end(), entry);
        if (it != errorScopeList->end())
            errorScopeList->erase(it);
    }
    else
    {
        errorScopeList->pop_back();
    }

    if (errorScopeList->empty())
        errorScopeList.reset();
}

// ErrorGuardImpl

ErrorGuardImpl::ErrorGuardImpl(ConstCharPtr filename, int fileLine)
    : filename(filename)
    , fileLine(fileLine)
{
    errorInfoHolder.setScopeEntry(this);
}

ErrorGuardImpl::~ErrorGuardImpl()
{
    errorInfoHolder.removeScopeEntry(this);
}

ErrCode ErrorGuardImpl::getErrorInfos(IList** errorInfos)
{
    if (errorInfos == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *errorInfos = nullptr;
    if (errorInfoList.empty())
        return OPENDAQ_SUCCESS;

    IList* list = nullptr;
    const ErrCode errCode = createListWithElementType(&list, IErrorInfo::Id);
    if (OPENDAQ_FAILED(errCode))
        return errCode;

    for (const auto& errorInfo : errorInfoList)
        list->pushBack(errorInfo.borrow());

    *errorInfos = list;
    return OPENDAQ_SUCCESS;
}

ErrCode ErrorGuardImpl::getFormatMessage(IString** message, ErrCode errCode) const
{
    if (message == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *message = nullptr;
    if (errorInfoList.empty())
        return OPENDAQ_SUCCESS;

    if (errCode != OPENDAQ_LAST_ERROR_INFO)
    {
        IErrorInfo* lastErrorInfo = errorInfoList.back().borrow();
        ErrCode lastErrorInfoCode;
        lastErrorInfo->getErrorCode(&lastErrorInfoCode);
        if (errCode != lastErrorInfoCode)
            return OPENDAQ_SUCCESS;
    }

    std::vector<IErrorInfo*> errorList;
    errorList.reserve(errorInfoList.size());
    for (auto it = errorInfoList.rbegin(); it != errorInfoList.rend(); ++it)
    {
        errorList.push_back(it->borrow());
        Bool causedByPrevious = False;
        it->borrow()->getCausedByPrevious(&causedByPrevious);
        if (!causedByPrevious)
            break;
    }

    if (errorList.empty())
        return OPENDAQ_SUCCESS;

    std::ostringstream ss;
    bool firstMessage = true;
    for (auto it = errorList.rbegin(); it != errorList.rend(); ++it)
    {
        IErrorInfo* errorInfo = *it;
        IString* errorMessage = nullptr;
        errorInfo->getFormatMessage(&errorMessage);
        if (errorMessage == nullptr)
            continue;

        ConstCharPtr messagePtr;
        errorMessage->getCharPtr(&messagePtr);
        if (messagePtr != nullptr)
        {
            ss << (firstMessage ? "" : "\n") << messagePtr;
            firstMessage = false;
        }
        errorMessage->releaseRef();
    }

    if (filename && !firstMessage)
    {
        ss << "\n - Caused by: [ " << filename;
        if (fileLine != -1)
            ss << ":" << fileLine;
        ss << " ]";
    }

    auto str = ss.str();
    return createString(message, str.c_str());
}

ErrCode ErrorGuardImpl::toString(CharPtr* str)
{
    std::ostringstream stream;
    stream << "ErrorGuard";
    if (filename)
    {
        stream << " [ " << filename;
        if (fileLine != -1)
            stream << ":" << fileLine;
        stream << " ]";
    }
    return daqDuplicateCharPtr(stream.str().c_str(), str);
}

ErrCode ErrorGuardImpl::getLastErrorInfo(IErrorInfo** errorInfo, ErrCode errCode) const
{
    if (errorInfo == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;
    *errorInfo = nullptr;

    if (errorInfoList.empty())
        return OPENDAQ_SUCCESS;

    auto lastErrorInfo = errorInfoList.back().borrow();
    ErrCode lastErrorInfoCode;
    lastErrorInfo->getErrorCode(&lastErrorInfoCode);

    if (errCode != lastErrorInfoCode && errCode != OPENDAQ_LAST_ERROR_INFO)
        return OPENDAQ_NOTFOUND;

    Bool causedByPrevious = False;
    lastErrorInfo->getCausedByPrevious(&causedByPrevious);
    if (!causedByPrevious)
    {
        *errorInfo = errorInfoList.back().getAddRef();
        return OPENDAQ_SUCCESS;
    }

    IString* message = nullptr;
    ErrCode err = this->getFormatMessage(&message, errCode);
    if (OPENDAQ_FAILED(err))
        return err;

    err = createErrorInfo(errorInfo);
    if (OPENDAQ_FAILED(err))
        return err;

    (*errorInfo)->setErrorCode(lastErrorInfoCode);
    (*errorInfo)->setMessage(message);
    releaseRefIfNotNull(message);
    return OPENDAQ_SUCCESS;
}

void ErrorGuardImpl::setErrorInfo(IErrorInfo* errorInfo)
{
    if (errorInfo)
        errorInfoList.emplace_back(errorInfo);
}

void ErrorGuardImpl::extendErrorInfo(IErrorInfo* errorInfo, ErrCode prevErrCode)
{
    if (errorInfo == nullptr)
        return;

    if (!errorInfoList.empty())
    {
        ErrCode prevErrorInfoCode;
        errorInfoList.back().borrow()->getErrorCode(&prevErrorInfoCode);
        if (prevErrorInfoCode == prevErrCode)
            errorInfo->setCausedByPrevious(prevErrCode);
    }

    errorInfoList.emplace_back(errorInfo);
}

void ErrorGuardImpl::clearLastErrorInfo(ErrCode errCode)
{
    if (errorInfoList.empty())
        return;

    if (errCode != OPENDAQ_LAST_ERROR_INFO)
    {
        ErrCode prevErrorInfoCode;
        errorInfoList.back().borrow()->getErrorCode(&prevErrorInfoCode);
        if (errCode != prevErrorInfoCode)
            return;
    }

    while (!errorInfoList.empty())
    {
        Bool causedByPrevious = False;
        errorInfoList.back().borrow()->getCausedByPrevious(&causedByPrevious);
        errorInfoList.pop_back();

        if (!causedByPrevious)
            break;
    }
}

inline bool ErrorGuardImpl::empty() const
{
    return errorInfoList.empty();
}

// ErrorInfoImpl

ErrorInfoImpl::ErrorInfoImpl()
    : message(nullptr)
    , source(nullptr)
    , fileName(nullptr)
    , fileLine(-1)
    , errorCode(OPENDAQ_ERR_GENERALERROR)
    , prevErrCode(OPENDAQ_SUCCESS)
    , frozen(False)
    , cachedMessage(nullptr)
    , fileNameObject(nullptr)
{
}

ErrorInfoImpl::~ErrorInfoImpl()
{
    releaseRefIfNotNull(message);
    releaseRefIfNotNull(source);
    releaseRefIfNotNull(cachedMessage);
    releaseRefIfNotNull(fileNameObject);
    if (fileName)
    {
        daqFreeMemory(fileName);
        fileName = nullptr;
    }
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

ErrCode ErrorInfoImpl::getMessage(IString** message)
{
    if (message == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *message = this->message;
    addRefIfNotNull(*message);

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

    if (this->fileName)
        daqFreeMemory(this->fileName);
    daqDuplicateCharPtr(fileName, &this->fileName);

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

    this->fileLine = line;
    return OPENDAQ_SUCCESS;
}

ErrCode ErrorInfoImpl::getFileLine(Int* line)
{
    if (line == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *line = this->fileLine;
    return OPENDAQ_SUCCESS;
}

ErrCode ErrorInfoImpl::setErrorCode(ErrCode errorCode)
{
    if (frozen)
        return OPENDAQ_ERR_FROZEN;

    this->errorCode = errorCode;
    return OPENDAQ_SUCCESS;
}

ErrCode ErrorInfoImpl::getErrorCode(ErrCode* errorCode)
{
    if (errorCode == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *errorCode = this->errorCode;
    return OPENDAQ_SUCCESS;
}

ErrCode ErrorInfoImpl::setCausedByPrevious(ErrCode prevErrCode)
{
    if (frozen)
        return OPENDAQ_ERR_FROZEN;

    this->prevErrCode = prevErrCode;
    return OPENDAQ_SUCCESS;
}

ErrCode ErrorInfoImpl::getCausedByPrevious(Bool* caused)
{
    if (caused == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *caused = this->prevErrCode != OPENDAQ_SUCCESS;
    return OPENDAQ_SUCCESS;
}

ErrCode ErrorInfoImpl::getFormatMessage(IString** message)
{
    if (message == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    if (this->cachedMessage)
    {
        *message = this->cachedMessage;
        addRefIfNotNull(*message);
        return OPENDAQ_SUCCESS;
    }

    std::ostringstream ss;

    if (this->prevErrCode != OPENDAQ_SUCCESS)
        ss << " - Cause by: ";

    [[maybe_unused]] bool needSpace = true;
    if (this->message)
    {
        ConstCharPtr msgCharPtr;
        this->message->getCharPtr(&msgCharPtr);

        if (msgCharPtr != nullptr)
            ss << msgCharPtr;
    }
    else if (this->prevErrCode != this->errorCode)
    {
        ss << ErrorCodeMessage(this->errorCode);
    }
    else
    {
        needSpace = false;
    }

#ifndef NDEBUG
    if (this->fileName)
    {
        ss << (needSpace ? " [ " : "[ ") << this->fileName;
        if (this->fileLine != -1)
            ss << ":" << this->fileLine;
        ss << " ]";
    }
#endif

    auto str = ss.str();
    const ErrCode errCode = createString(message, str.c_str());
    if (OPENDAQ_SUCCEEDED(errCode))
    {
        this->cachedMessage = *message;
        addRefIfNotNull(this->cachedMessage);
    }
    return errCode;
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
    if (fileLine != -1)
    {
        serializer->key("fileLine");
        serializer->writeInt(fileLine);
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
        releaseRefIfNotNull(errorInfo);
        releaseRefIfNotNull(message);
        releaseRefIfNotNull(source);
        releaseRefIfNotNull(fileName);
        releaseRefIfNotNull(messageKey);
        releaseRefIfNotNull(sourceKey);
        releaseRefIfNotNull(fileNameKey);
        releaseRefIfNotNull(fileLineKey);
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
OPENDAQ_DEFINE_CLASS_FACTORY(
    LIBRARY_FACTORY, ErrorGuard,
    ConstCharPtr, fileName,
    Int, fileLine)

END_NAMESPACE_OPENDAQ

extern "C"
void PUBLIC_EXPORT daqSetErrorInfo(daq::IErrorInfo* errorInfo)
{
    if (errorInfo == nullptr)
        return;

    daq::errorInfoHolder.setErrorInfo(errorInfo);
    daq::IFreezable* freezable;
    if (OPENDAQ_SUCCEEDED(errorInfo->borrowInterface(daq::IFreezable::Id, reinterpret_cast<void**>(&freezable))))
        freezable->freeze();
}

extern "C"
void PUBLIC_EXPORT daqExtendErrorInfo(daq::IErrorInfo* errorInfo, daq::ErrCode prevErrCode)
{
    if (errorInfo == nullptr)
        return;

    daq::errorInfoHolder.extendErrorInfo(errorInfo, prevErrCode);
    daq::IFreezable* freezable;
    if (OPENDAQ_SUCCEEDED(errorInfo->borrowInterface(daq::IFreezable::Id, reinterpret_cast<void**>(&freezable))))
        freezable->freeze();
}

extern "C"
void PUBLIC_EXPORT daqGetErrorInfo(daq::IErrorInfo** errorInfo, daq::ErrCode errCode)
{
    if (errorInfo)
        *errorInfo = daq::errorInfoHolder.getErrorInfo(errCode);
}

extern "C"
void PUBLIC_EXPORT daqGetErrorInfoList(daq::IList** errorInfoList)
{
    if (errorInfoList)
        *errorInfoList = daq::errorInfoHolder.getErrorInfoList();
}

extern "C"
void PUBLIC_EXPORT daqGetErrorInfoMessage(daq::IString** errorMessage, daq::ErrCode errCode)
{
    if (errorMessage)
        *errorMessage = daq::errorInfoHolder.getFormatMessage(errCode);
}

extern "C"
void PUBLIC_EXPORT daqClearErrorInfo(daq::ErrCode errCode)
{
    daq::errorInfoHolder.clearErrorInfo(errCode);
}