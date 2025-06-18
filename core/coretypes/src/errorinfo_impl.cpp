#include <coretypes/errorinfo_impl.h>
#include <coretypes/impl.h>
#include <coretypes/freezable.h>
#include <iostream>
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

ErrorInfoHolder::ContainerT* ErrorInfoHolder::getOrCreateList()
{
    if (!errorScopeList)
    {
        errorScopeList = std::make_unique<ContainerT>();
        auto entry = new ErrorGuardImpl(nullptr, -1);
        entry->addRef();
        errorScopeList->emplace_back(entry);
    }
    return errorScopeList.get();
}


void ErrorInfoHolder::setErrorInfo(IErrorInfo* errorInfo)
{
    if (!errorInfo && errorScopeList)
    {
        errorScopeList->back()->setErrorInfo(nullptr);
        if (errorScopeList->size() == 1 && errorScopeList->back()->empty())
            errorScopeList->back()->releaseRef();
        return;
    }
    getOrCreateList()->back()->setErrorInfo(errorInfo);
}

IErrorInfo* ErrorInfoHolder::getErrorInfo() const
{
    if (!errorScopeList)
        return nullptr;

    return errorScopeList->back()->getErrorInfo();
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

IString* ErrorInfoHolder::getFormatMessage() const
{
    if (!errorScopeList)
        return nullptr;

    IString* message = nullptr;
    const ErrCode errCode = errorScopeList->back()->getFormatMessage(&message);
    if (OPENDAQ_FAILED(errCode))
        throw std::runtime_error("Failed to get format message");
    return message;
}

void ErrorInfoHolder::setScopeEntry(ErrorGuardImpl* entry)
{
    if (!entry)
        throw std::invalid_argument("ErrorGuardImpl entry must not be null");

    getOrCreateList()->push_back(entry);
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

    auto last = errorScopeList->back();
    if (last != entry)
    {
        std::cerr << "ErrorGuardImpl scope entry mismatch. Expected: " << last << ", got: " << entry << std::endl;
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

// ErrorScope

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
    {
        IBaseObject* errorInfoObject = nullptr;
        errorInfo.borrow()->queryInterface(IBaseObject::Id, reinterpret_cast<void**>(&errorInfoObject));
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

    *message = nullptr;
    if (errorInfoList.empty())
        return OPENDAQ_SUCCESS;

    std::ostringstream ss;
    for (const auto& errorInfo : errorInfoList)
    {
        IString* message = nullptr;
        errorInfo.borrow()->getFormatMessage(&message);
        if (message == nullptr)
            continue;

        ConstCharPtr messagePtr;
        message->getCharPtr(&messagePtr);
        if (messagePtr != nullptr)
            ss << messagePtr << "\n";
        message->releaseRef();
    }

    if (filename)
    {
        ss << " - Caused by: [" << filename;
        if (fileLine != -1)
            ss << ":" << fileLine;
        ss << "]\n";
    }

    auto str = ss.str();
    return createString(message, str.c_str());
}

void ErrorGuardImpl::setErrorInfo(IErrorInfo* errorInfo)
{
    if (errorInfo)
        errorInfoList.emplace_back(errorInfo);
    else
        clearLastErrorInfo();
}

IErrorInfo* ErrorGuardImpl::getErrorInfo() const
{
    if (errorInfoList.empty())
        return nullptr;

    return errorInfoList.back().get();
}

void ErrorGuardImpl::clearLastErrorInfo()
{
    while (!errorInfoList.empty())
    {
        Bool causedByPrevious = False;
        errorInfoList.back().borrow()->getCausedByPrevious(&causedByPrevious);
        errorInfoList.pop_back();

        if (!causedByPrevious)
            break;
    }
}

bool ErrorGuardImpl::empty() const
{
    return errorInfoList.empty();
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
    if (errorInfo)
        *errorInfo = daq::errorInfoHolder.getErrorInfo();
}

extern "C" 
void PUBLIC_EXPORT daqGetErrorInfoList(daq::IList** errorInfoList)
{
    if (errorInfoList)
        *errorInfoList = daq::errorInfoHolder.getErrorInfoList();
}

extern "C" 
void PUBLIC_EXPORT daqGetErrorInfoMessage(daq::IString** errorMessage)
{
    if (errorMessage)
        *errorMessage = daq::errorInfoHolder.getFormatMessage();
}

extern "C"
void PUBLIC_EXPORT daqClearErrorInfo()
{
    daq::errorInfoHolder.setErrorInfo(nullptr);
}