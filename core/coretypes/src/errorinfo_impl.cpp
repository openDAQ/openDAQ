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
    IList* list;
    
    if (scopeEntry != nullptr)
    {
        const ErrCode errCode = scopeEntry->getErrorInfos(&list);
        if (OPENDAQ_FAILED(errCode))
            throw std::runtime_error("Failed to get error info list"    );
        return list;
    }

    const ErrCode res = createListWithElementType(&list, IErrorInfo::Id);
    if (OPENDAQ_FAILED(res))
        throw std::runtime_error("Failed to create error info list");

    if (!errorInfoList)
        return list;
    for (const auto& item : *errorInfoList)
    {
        IBaseObject* errorInfoObject = nullptr;
        item.borrow()->queryInterface(IBaseObject::Id, reinterpret_cast<void**>(&errorInfoObject));
        if (errorInfoObject != nullptr)
            list->moveBack(errorInfoObject);
    }
    errorInfoList.reset();
    return list;
}

// ErrorScope

ErrorGuardImpl::ErrorGuardImpl(ConstCharPtr filename, int fileLine)
    : threadId(std::this_thread::get_id())
    , prevScopeEntry(errorInfoHolder.scopeEntry)
{
    errorInfoHolder.scopeEntry = this;

    IErrorInfo* errorMark = nullptr;
    const ErrCode errCode = createErrorInfo(&errorMark);
    if (OPENDAQ_FAILED(errCode))
        throw std::runtime_error("Failed to create error info");

    errorMark->setFileName(filename);
    errorMark->setFileLine(fileLine);

    auto list = errorInfoHolder.getList();
    list->emplace_back(errorMark);
    errorMark->releaseRef();
        it = std::prev(list->end());
}

ErrorGuardImpl::~ErrorGuardImpl()
{
    assert(std::this_thread::get_id() == threadId && "ErrorGuard must be used in the same thread where it was created");
    errorInfoHolder.scopeEntry = prevScopeEntry;
    auto errorList = errorInfoHolder.getList();
    errorList->erase(this->it, errorList->end());
}

ErrCode ErrorGuardImpl::getErrorInfos(IList** errorInfos)
{
    if (errorInfos == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;
    if (threadId != std::this_thread::get_id())
        return OPENDAQ_ERR_INVALIDVALUE;

    IList* list;
    const ErrCode res = createListWithElementType(&list, IErrorInfo::Id);
    if (OPENDAQ_FAILED(res))
        throw std::runtime_error("Failed to create error info list");

    auto errorList = errorInfoHolder.getList();
    for (auto it = this->it; it != errorList->end(); it++)
    {
        IBaseObject* errorInfoObject = nullptr;
        it->borrow()->queryInterface(IBaseObject::Id, reinterpret_cast<void**>(&errorInfoObject));
        if (errorInfoObject != nullptr)
            list->moveBack(errorInfoObject);
    }
    *errorInfos = list;
    return OPENDAQ_SUCCESS; 
}

ErrCode ErrorGuardImpl::getFormatMessage(IString** message)
{
    if (message == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;
    if (threadId != std::this_thread::get_id())
        return OPENDAQ_ERR_INVALIDVALUE;
   
    auto errorList = errorInfoHolder.getList();
    if (std::next(this->it) == errorList->end())
        return OPENDAQ_SUCCESS;

    std::ostringstream ss;
    for (auto it = this->it; it != errorList->end(); it++)
    {
        StringPtr message;
        it->borrow()->getFormatMessage(&message);
        if (message.assigned())
            ss << message << "\n";
    }

    auto str = ss.str();
    return createString(message, str.c_str());
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
void PUBLIC_EXPORT daqCheckErrorGuard(daq::ErrCode errCode, daq::IErrorGuard* errorGuard)
{

#ifdef NDEBUG
    if (OPENDAQ_SUCCEEDED(errCode))
        return;
#endif

    std::string message;
    if (errorGuard)
    {
        daq::IString* messagePtr = nullptr;
        errorGuard->getFormatMessage(&messagePtr);
        if (messagePtr)
        {
            daq::ConstCharPtr msgCharPtr;
            messagePtr->getCharPtr(&msgCharPtr);
            if (msgCharPtr)
                message = msgCharPtr;
            messagePtr->releaseRef();
        }
    }
   
    if (OPENDAQ_SUCCEEDED(errCode) && message.empty())
        return;

    daq::throwExceptionFromErrorCode(errCode, message);
}
