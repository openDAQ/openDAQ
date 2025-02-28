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

    if (errorInfo == nullptr)
        errorInfoObject->releaseRef();

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
        return  OPENDAQ_IGNORED;

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
