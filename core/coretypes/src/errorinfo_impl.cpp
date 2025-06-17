#include <coretypes/errorinfo_impl.h>
#include <coretypes/impl.h>
#include <coretypes/freezable.h>
#include <memory>
#include <stdexcept>
#include <coretypes/common.h>
#include <coretypes/errors.h>
#include <coretypes/string_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

thread_local ErrorInfoHolder errorInfoHolder;

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

IErrorInfo* ErrorInfoWrapper::get() const
{
    addRefIfNotNull(errorInfo);
    return errorInfo;
}

IErrorInfo* ErrorInfoWrapper::borrow() const
{
    return errorInfo;
}

// ErrorInfoHolder

ErrorInfoHolder::ContainerT* ErrorInfoHolder::getList()
{
    if (!errorInfoList)
        errorInfoList = std::make_unique<ContainerT>();
    return errorInfoList.get();
}

void ErrorInfoHolder::setScopeEntry(ErrorGuardImpl* entry)
{
    if (scopeEntry != nullptr && scopeEntry->prevScopeEntry == entry)
    {
        auto list = getList();
        while (!list->empty())
        {
            IErrorInfo* errorInfo = list->back().borrow();
            if (errorInfo == scopeEntry->errorMark)
                break;

            list->pop_back();
        }
    }
    else if (entry != nullptr)
    {
        entry->prevScopeEntry = scopeEntry;
        auto list = getList();
        if (!list->empty())
            entry->errorMark = list->back().borrow();
        else
            entry->errorMark = nullptr;
    }
    scopeEntry = entry;
}

void ErrorInfoHolder::setErrorInfo(IErrorInfo* errorInfo)
{
    if (errorInfo)
    {
        getList()->emplace_back(errorInfo);
        return;
    }

    if (!errorInfoList)
        return;

    Bool causedByPrevious = True;
    IErrorInfo* errorMark = scopeEntry ? scopeEntry->errorMark : nullptr;
    while (!errorInfoList->empty())
    {
        if (!causedByPrevious)
            break;

        IErrorInfo* errorInfo = errorInfoList->back().borrow();
        if (errorInfo == errorMark)
            break;

        errorInfo->getCausedByPrevious(&causedByPrevious);
        errorInfoList->pop_back();
    }

    if (!scopeEntry && errorInfoList->empty())
        errorInfoList.reset();
}

IErrorInfo* ErrorInfoHolder::getErrorInfo() const
{
    if (!errorInfoList || errorInfoList->empty())
        return nullptr;

    return errorInfoList->back().get();
}

IList* ErrorInfoHolder::getErrorInfoList()
{
    IList* list = nullptr;
    const ErrCode errCode = createListWithElementType(&list, IErrorInfo::Id);
    if (OPENDAQ_FAILED(errCode))
        throw std::runtime_error("Failed to create error info list");

    if (!errorInfoList || errorInfoList->empty())
        return list;

    IErrorInfo* errorInfo = nullptr;
    if (scopeEntry != nullptr)
        errorInfo = scopeEntry->errorMark;

    for (auto it = errorInfoList->rbegin(); it != errorInfoList->rend(); ++it)
    {
        if (it->borrow() == errorInfo)
            break;

        IBaseObject* errorInfoObject = nullptr;
        it->borrow()->queryInterface(IBaseObject::Id, reinterpret_cast<void**>(&errorInfoObject));
        if (errorInfoObject != nullptr)
            list->moveFront(errorInfoObject);
    }

    if (scopeEntry == nullptr)
        errorInfoList.reset();
    return list;
}

IString* ErrorInfoHolder::getFormatMessage() const
{
    std::ostringstream ss;
    if (errorInfoList && !errorInfoList->empty())
    {
        IErrorInfo* errorInfo = nullptr;
        if (scopeEntry != nullptr)
            errorInfo = scopeEntry->errorMark;

        bool thereIsErrorInfo = false;
        for (auto it = errorInfoList->rbegin(); it != errorInfoList->rend(); ++it)
        {
            if (it->borrow() == errorInfo)
                break;

            IString* message = nullptr;
            it->borrow()->getFormatMessage(&message);
            if (message == nullptr)
                continue;
            thereIsErrorInfo = true;
            ConstCharPtr messagePtr;
            message->getCharPtr(&messagePtr);
            if (messagePtr != nullptr)
                ss << messagePtr << "\n";
            message->releaseRef();
        }
        if (thereIsErrorInfo && scopeEntry && scopeEntry->filename)
        {
            ss << " - Caused by: [" << scopeEntry->filename;
            if (scopeEntry->fileLine != -1)
                ss << ":" << scopeEntry->fileLine;
            ss << "]\n";
        }
    }

    auto str = ss.str();
    IString* result = nullptr;
    const ErrCode errCode = createString(&result, str.c_str());
    if (OPENDAQ_FAILED(errCode))
        throw std::runtime_error("Failed to create error message string");
    return result;
}

// ErrorScope

ErrorGuardImpl::ErrorGuardImpl(ConstCharPtr filename, int fileLine)
    : filename(filename)
    , fileLine(fileLine)
    , prevScopeEntry(nullptr)
    , threadId(std::this_thread::get_id())
    , errorMark(nullptr)
    , holder(&errorInfoHolder)
{
    errorInfoHolder.setScopeEntry(this);
}

ErrorGuardImpl::~ErrorGuardImpl()
{
    errorInfoHolder.setScopeEntry(prevScopeEntry);
}

// ErrorInfoImpl

ErrorInfoImpl::ErrorInfoImpl()
    : message(nullptr)
    , source(nullptr)
    , fileName(nullptr)
    , fileLine(-1)
    , frozen(False)
    , causedByPrevious(False)
{
}

ErrorInfoImpl::~ErrorInfoImpl()
{
    releaseRefIfNotNull(message);
    releaseRefIfNotNull(source);
    if (fileName)
    {
        daqFreeMemory(fileName);
        fileName = nullptr;
    }
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

    if (this->fileName)
        daqFreeMemory(this->fileName);
    daqDuplicateCharPtr(fileName, &this->fileName);

    return OPENDAQ_SUCCESS;
}

ErrCode ErrorInfoImpl::getFileName(CharPtr* fileName)
{
    if (fileName == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    daqDuplicateCharPtr(this->fileName, fileName);

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

ErrCode ErrorInfoImpl::setSource(IString* source)
{
    if (frozen)
        return OPENDAQ_ERR_FROZEN;

    releaseRefIfNotNull(this->source);
    this->source = source;
    addRefIfNotNull(this->source);

    return OPENDAQ_SUCCESS;
}

ErrCode ErrorInfoImpl::setCausedByPrevious(Bool caused)
{
    if (frozen)
        return OPENDAQ_ERR_FROZEN;

    this->causedByPrevious = caused;
    return OPENDAQ_SUCCESS;
}

ErrCode ErrorInfoImpl::getCausedByPrevious(Bool* caused)
{
    if (caused == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *caused = this->causedByPrevious;
    return OPENDAQ_SUCCESS;
}

ErrCode ErrorInfoImpl::getFormatMessage(IString** message)
{
    if (message == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    std::ostringstream ss;

    if (this->causedByPrevious)
        ss << "Caused by: ";

    if (this->message)
    {
        ConstCharPtr msgCharPtr;
        this->message->getCharPtr(&msgCharPtr);

        if (msgCharPtr != nullptr)
            ss << msgCharPtr;
    }

#ifndef NDEBUG
    if (this->fileName)
    {
        ss << " [ " << this->fileName;
        if (this->fileLine != -1)
            ss << ":" << this->fileLine;
        ss << " ]";
    }
#endif

    auto str = ss.str();
    return createString(message, str.c_str());
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

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, ErrorInfo)
OPENDAQ_DEFINE_CLASS_FACTORY(
    LIBRARY_FACTORY, ErrorGuard,
    ConstCharPtr, fileName,
    Int, fileLine)

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
    *errorInfo = daq::errorInfoHolder.getErrorInfo();
}

extern "C" 
void PUBLIC_EXPORT daqGetErrorInfoList(daq::IList** errorInfoList)
{
    *errorInfoList = daq::errorInfoHolder.getErrorInfoList();
}

extern "C"
void PUBLIC_EXPORT daqClearErrorInfo()
{
    daq::errorInfoHolder.setErrorInfo(nullptr);
}

extern "C" 
void PUBLIC_EXPORT daqCheckErrorGuard(daq::ErrCode errCode)
{
    if (OPENDAQ_SUCCEEDED(errCode))
        return;

    daq::IString* messagePtr = daq::errorInfoHolder.getFormatMessage();
    daq::ConstCharPtr msgCharPtr = nullptr;
    messagePtr->getCharPtr(&msgCharPtr);
    std::string message = msgCharPtr;
    messagePtr->releaseRef();

    daq::throwExceptionFromErrorCode(errCode, message);
}
