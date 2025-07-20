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

    ErrCode INTERFACE_FUNC getErrorInfoList(IList** errorInfos) override
    {
        const ErrCode errCode = Super::getErrorInfoList(errorInfos);
        this->releaseRef();
        return errCode;
    }

    ErrCode clearLastErrorInfo() override
    {
        ErrCode errCode = Super::clearLastErrorInfo();
        if (this->empty())
            this->releaseRef();
        return errCode;
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

void ErrorInfoHolder::extendErrorInfo(IErrorInfo* errorInfo)
{
    if (errorInfo)
        getOrCreateList()->back()->extendErrorInfo(errorInfo);
}

ErrCode ErrorInfoHolder::clearErrorInfo()
{
    if (errorScopeList)
        return errorScopeList->back()->clearLastErrorInfo();
    return OPENDAQ_SUCCESS;
}

ErrCode ErrorInfoHolder::getErrorInfo(IErrorInfo** errorInfo) const
{
    if (errorScopeList)
        return errorScopeList->back()->getLastErrorInfo(errorInfo);

    return OPENDAQ_SUCCESS;
}

IList* ErrorInfoHolder::getErrorInfoList()
{
    if (!errorScopeList)
        return nullptr;

    IList* list = nullptr;
    const ErrCode errCode = errorScopeList->back()->getErrorInfoList(&list);
    if (OPENDAQ_FAILED(errCode))
        throw std::runtime_error("Failed to get error info list");
    return list;
}

ErrCode ErrorInfoHolder::getFormattedMessage(IString** message) const
{
    if (message == nullptr)
        return OPENDAQ_IGNORED;
    if (!errorScopeList)
        return OPENDAQ_SUCCESS;

    return errorScopeList->back()->getFormattedMessage(message);
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

ErrCode ErrorGuardImpl::getErrorInfoList(IList** errorInfos)
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

ErrCode ErrorGuardImpl::getFormattedMessage(IString** message) const
{
    if (message == nullptr)
        return OPENDAQ_IGNORED;

    *message = nullptr;
    if (errorInfoList.empty())
        return OPENDAQ_SUCCESS;

    ErrCode errCode = OPENDAQ_SUCCESS;
    std::vector<IErrorInfo*> errorList;
    errorList.reserve(errorInfoList.size());
    for (auto it = errorInfoList.rbegin(); it != errorInfoList.rend(); ++it)
    {
        errorList.push_back(it->borrow());
        ErrCode prevErrCode = OPENDAQ_SUCCESS;
        it->borrow()->getPreviousErrorCode(&prevErrCode);
        if (prevErrCode == OPENDAQ_SUCCESS)
        {
            it->borrow()->getErrorCode(&errCode);
            break;
        }
    }

    std::ostringstream ss;
    bool firstMessage = true;
    for (auto it = errorList.rbegin(); it != errorList.rend(); ++it)
    {
        IErrorInfo* errorInfo = *it;
        IString* errorMessage = nullptr;
        errorInfo->getFormattedMessage(&errorMessage);
        if (errorMessage == nullptr)
            continue;

        ConstCharPtr messagePtr;
        errorMessage->getCharPtr(&messagePtr);
        if (messagePtr)
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
    createString(message, str.c_str());
    return errCode;
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

ErrCode ErrorGuardImpl::getLastErrorInfo(IErrorInfo** errorInfo) const
{
    if (errorInfo == nullptr)
        return OPENDAQ_IGNORED;
    *errorInfo = nullptr;

    if (errorInfoList.empty())
        return OPENDAQ_SUCCESS;

    auto lastErrorInfo = errorInfoList.back().borrow();
    ErrCode lastErrorCode = OPENDAQ_SUCCESS;
    lastErrorInfo->getErrorCode(&lastErrorCode);
    ErrCode prevErrorCode = OPENDAQ_SUCCESS;
    lastErrorInfo->getPreviousErrorCode(&prevErrorCode);
    if (prevErrorCode == OPENDAQ_SUCCESS)
    {
        *errorInfo = errorInfoList.back().getAddRef();
        return lastErrorCode;
    }

    IString* message = nullptr;
    ErrCode errCode = this->getFormattedMessage(&message);
    if (message == nullptr)
        return lastErrorCode;

    errCode = createErrorInfo(errorInfo);
    if (OPENDAQ_SUCCEEDED(errCode))
    {
        (*errorInfo)->setErrorCode(lastErrorCode);
        (*errorInfo)->setMessage(message);
    }

    releaseRefIfNotNull(message);
    return lastErrorCode;
}

void ErrorGuardImpl::setErrorInfo(IErrorInfo* errorInfo)
{
    if (errorInfo)
        errorInfoList.emplace_back(errorInfo);
}

void ErrorGuardImpl::extendErrorInfo(IErrorInfo* errorInfo)
{
    if (errorInfo == nullptr)
        return;

    if (errorInfoList.empty())
        throw std::runtime_error("ErrorGuardImpl: Cannot extend error info without a previous error info.");

    ErrCode prevErrCode = OPENDAQ_SUCCESS;
    errorInfo->getPreviousErrorCode(&prevErrCode);

    ErrCode prevErrorInfoCode;
    errorInfoList.back().borrow()->getErrorCode(&prevErrorInfoCode);
    if (prevErrorInfoCode != prevErrCode)
        throw std::runtime_error("ErrorGuardImpl: Cannot extend error info with a different previous error code.");

    errorInfoList.emplace_back(errorInfo);
}

ErrCode ErrorGuardImpl::clearLastErrorInfo()
{
    if (errorInfoList.empty())
        return OPENDAQ_SUCCESS;

    ErrCode lastErrorCode;
    while (!errorInfoList.empty())
    {
        lastErrorCode = OPENDAQ_SUCCESS;
        ErrCode prevErrCode = OPENDAQ_SUCCESS;
        errorInfoList.back().borrow()->getPreviousErrorCode(&prevErrCode);
        errorInfoList.back().borrow()->getErrorCode(&lastErrorCode);
        errorInfoList.pop_back();

        if (prevErrCode == OPENDAQ_SUCCESS)
            break;
    }
    return lastErrorCode;
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

ErrCode ErrorInfoImpl::setPreviousErrorCode(ErrCode prevErrCode)
{
    if (frozen)
        return OPENDAQ_ERR_FROZEN;

    this->prevErrCode = prevErrCode;
    return OPENDAQ_SUCCESS;
}

ErrCode ErrorInfoImpl::getPreviousErrorCode(ErrCode* prevErrCode)
{
    if (prevErrCode == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *prevErrCode = this->prevErrCode;
    return OPENDAQ_SUCCESS;
}

ErrCode ErrorInfoImpl::getFormattedMessage(IString** message)
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
        ss << " - Caused by: ";

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
void PUBLIC_EXPORT daqExtendErrorInfo(daq::IErrorInfo* errorInfo)
{
    if (errorInfo == nullptr)
        return;

    daq::errorInfoHolder.extendErrorInfo(errorInfo);
    daq::IFreezable* freezable;
    if (OPENDAQ_SUCCEEDED(errorInfo->borrowInterface(daq::IFreezable::Id, reinterpret_cast<void**>(&freezable))))
        freezable->freeze();
}

extern "C"
daq::ErrCode PUBLIC_EXPORT daqGetErrorInfo(daq::IErrorInfo** errorInfo)
{
    if (errorInfo)
        return daq::errorInfoHolder.getErrorInfo(errorInfo);
    return OPENDAQ_IGNORED;
}

extern "C"
void PUBLIC_EXPORT daqGetErrorInfoList(daq::IList** errorInfoList)
{
    if (errorInfoList)
        *errorInfoList = daq::errorInfoHolder.getErrorInfoList();
}

extern "C"
daq::ErrCode PUBLIC_EXPORT daqGetErrorInfoMessage(daq::IString** errorMessage)
{
    if (errorMessage)
        return daq::errorInfoHolder.getFormattedMessage(errorMessage);
    return OPENDAQ_IGNORED;
}

extern "C"
daq::ErrCode PUBLIC_EXPORT daqClearErrorInfo()
{
    return daq::errorInfoHolder.clearErrorInfo();
}