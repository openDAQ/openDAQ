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
        list->erase(scopeEntry->it, list->end());
    }
    else if (entry != nullptr)
    {
        entry->prevScopeEntry = scopeEntry;

        auto list = getList();
        entry->it = std::prev(list->end());
    }
    scopeEntry = entry;
}

void ErrorInfoHolder::setErrorInfo(IErrorInfo* errorInfo)
{
    if (errorInfo == nullptr)
    {
        if (!errorInfoList)
            return;

        if (!errorInfoList->empty())
        {
            auto it = std::prev(errorInfoList->end());
            if (scopeEntry && it == scopeEntry->it)
                return;

            errorInfoList->erase(it, errorInfoList->end());
        }

        if (errorInfoList->empty())
            errorInfoList.reset();
        return;
    }

    getList()->emplace_back(errorInfo);
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

    if (!errorInfoList)
        return list;

    ContainerT::iterator it;
    if (scopeEntry != nullptr)
        it = std::next(scopeEntry->it);
    else
        it = errorInfoList->begin();

    for (; it != errorInfoList->end(); ++it)
    {
        IBaseObject* errorInfoObject = nullptr;
        it->borrow()->queryInterface(IBaseObject::Id, reinterpret_cast<void**>(&errorInfoObject));
        if (errorInfoObject != nullptr)
            list->moveBack(errorInfoObject);
    }
    
    if (scopeEntry == nullptr)
        errorInfoList.reset();
    return list;
}

IString* ErrorInfoHolder::getFormatMessage() const
{
    std::ostringstream ss;
    if (errorInfoList)
    {
        ContainerT::iterator it;
        if (scopeEntry != nullptr)
            it = scopeEntry->it;
        else
            it = errorInfoList->begin();

        if (std::next(it) != errorInfoList->end())
        {
            ss << "Raised exception in scope";
            for (; it != errorInfoList->end(); ++it)
            {
                StringPtr message;
                it->borrow()->getFormatMessage(&message);
                if (message.assigned())
                    ss << message << "\n";
            }
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
    : threadId(std::this_thread::get_id())
{
    IErrorInfo* errorMark = nullptr;
    const ErrCode errCode = createErrorInfo(&errorMark);
    if (OPENDAQ_FAILED(errCode))
        throw std::runtime_error("Failed to create error info");

    errorMark->setFileName(filename);
    errorMark->setFileLine(fileLine);
    errorInfoHolder.setErrorInfo(errorMark);
    errorMark->releaseRef();

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
{
}

ErrorInfoImpl::~ErrorInfoImpl()
{
    releaseRefIfNotNull(message);
    releaseRefIfNotNull(source);
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

ErrCode ErrorInfoImpl::getFormatMessage(IString** message)
{
    if (message == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    std::ostringstream ss;

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
#ifdef NDEBUG
    if (OPENDAQ_SUCCEEDED(errCode))
        return;
#endif

    daq::IString* messagePtr = daq::errorInfoHolder.getFormatMessage();
    daq::SizeT messageLength = 0;
    messagePtr->getLength(&messageLength);
   
    if (OPENDAQ_SUCCEEDED(errCode) && messageLength == 0)
    {
            messagePtr->releaseRef();
        return;
    }

    daq::ConstCharPtr msgCharPtr = nullptr;
    messagePtr->getCharPtr(&msgCharPtr);
    std::string message = msgCharPtr;
    messagePtr->releaseRef();

    daq::throwExceptionFromErrorCode(errCode, message);
}
