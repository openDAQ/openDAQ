#include <coretypes/errorinfo_impl.h>
#include <coretypes/impl.h>
#include <coretypes/freezable.h>

BEGIN_NAMESPACE_OPENDAQ

thread_local ErrorInfoHolder errorInfoHolder;

// ErrorInfoHolder

// mingw has a bug which causes segmentation fault on thread_local destructor
// disabling produces memory leak on mingw on thread exit if errorInfo object is assigned

ErrorInfoHolder::~ErrorInfoHolder()
{
    if (errorInfoList == nullptr)
        return;
    for (auto item : *errorInfoList)
        releaseRefIfNotNull(item);
    delete errorInfoList;
}

void ErrorInfoHolder::setErrorInfo(IErrorInfo* errorInfo)
{
    if (errorInfo == nullptr)
    {
        if (errorInfoList == nullptr)
            return;

        if (!errorInfoList->empty())
        {
            auto lastErrorInfo = errorInfoList->back();
            errorInfoList->pop_back();
            releaseRefIfNotNull(lastErrorInfo);
        }

        if (errorInfoList->empty())
        {
            delete errorInfoList;
            errorInfoList = nullptr;
        }
        return;
    }

    if (errorInfoList == nullptr)
        errorInfoList = new std::list<IErrorInfo*>();

    errorInfoList->push_back(errorInfo);
    addRefIfNotNull(errorInfo);
}

void ErrorInfoHolder::extendErrorInfo(IErrorInfo* errorInfo)
{
    if (errorInfo == nullptr)
        return;

    if (errorInfoList == nullptr)
        errorInfoList = new std::list<IErrorInfo*>();

    if (errorInfoList->empty())
    {
        errorInfoList->push_back(errorInfo);
        addRefIfNotNull(errorInfo);
        return;
    }

    errorInfoList->back()->extend(errorInfo);
}

IErrorInfo* ErrorInfoHolder::getErrorInfo() const
{
    if (errorInfoList == nullptr || errorInfoList->empty())
        return nullptr;

    IErrorInfo* errorInfo = errorInfoList->back();
    addRefIfNotNull(errorInfo);

    return errorInfo;
}

IList* ErrorInfoHolder::getErrorInfoList()
{
    IList* list = ListWithElementType_Create(IErrorInfo::Id);
    if (errorInfoList == nullptr)
        return list;
    for (auto item : *errorInfoList)
    {
        IBaseObject* errorInfoObject = nullptr;
        item->borrowInterface(IBaseObject::Id, reinterpret_cast<void**>(&errorInfoObject));
        if (errorInfoObject != nullptr)
            list->moveBack(errorInfoObject);
    }
    delete errorInfoList;
    errorInfoList = nullptr;
    return list;
}

// ErrorInfoImpl

ErrorInfoImpl::ErrorInfoImpl()
    : message(nullptr)
    , source(nullptr)
    , fileName(nullptr)
    , line(-1)
    , frozen(False)
{
}

ErrorInfoImpl::~ErrorInfoImpl()
{
    releaseRefIfNotNull(message);
    releaseRefIfNotNull(source);
    if (childErrorInfoList != nullptr)
    {
        for (auto item : *childErrorInfoList)
            releaseRefIfNotNull(item);
        delete childErrorInfoList;
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

ErrCode ErrorInfoImpl::extend(IErrorInfo* errorInfo)
{
    if (errorInfo == nullptr)
        return OPENDAQ_IGNORED;

    if (childErrorInfoList == nullptr)
        childErrorInfoList = new std::list<IErrorInfo*>();

    childErrorInfoList->push_back(errorInfo);
    errorInfo->addRef();

    return OPENDAQ_SUCCESS;
}

std::ostringstream& CreateErrorMessage(std::ostringstream& ss, IErrorInfo* errorInfo)
{
    if (errorInfo == nullptr)
        return ss;

    IString* message;
    errorInfo->getMessage(&message);

    if (message != nullptr)
    {
        ConstCharPtr msgCharPtr;
        message->getCharPtr(&msgCharPtr);

        if (msgCharPtr != nullptr)
            ss << msgCharPtr;

        message->releaseRef();
    }

#ifndef NDEBUG
    ConstCharPtr fileNameCharPtr;
    Int fileLine = -1;
    
    errorInfo->getFileName(&fileNameCharPtr);
    errorInfo->getFileLine(&fileLine);

    if (fileNameCharPtr != nullptr)
    {
        ss << " [ " << fileNameCharPtr;
        if (fileLine != -1)
            ss << ":" << fileLine;
        ss << " ]";
    }
#endif
    
    return ss;
}

ErrCode ErrorInfoImpl::getFormatMessage(IString** message)
{
    if (message == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    std::ostringstream ss;
    CreateErrorMessage(ss, this->borrowInterface());

    if (childErrorInfoList != nullptr)
    {
        ss << "\nCaused by:";

        for (auto errorInfo : *childErrorInfoList)
        {
            IString* childMessage = nullptr;
            errorInfo->getFormatMessage(&childMessage);
            if (childMessage != nullptr)
            {
                ConstCharPtr childMsgCharPtr;
                childMessage->getCharPtr(&childMsgCharPtr);

                if (childMsgCharPtr != nullptr)
                    ss << "\n - " << childMsgCharPtr;

                childMessage->releaseRef();
            }
        }
    }

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
void PUBLIC_EXPORT daqExtendErrorInfo(daq::IErrorInfo* errorInfo)
{
    if (errorInfo != nullptr)
    {
        daq::IFreezable* freezable;
        if (OPENDAQ_SUCCEEDED(errorInfo->borrowInterface(daq::IFreezable::Id, reinterpret_cast<void**>(&freezable))))
            freezable->freeze();
    }
    daq::errorInfoHolder.extendErrorInfo(errorInfo);
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
