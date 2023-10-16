#include <coreobjects/callable_info_impl.h>
#include <coreobjects/callable_info_factory.h>
#include <coretypes/impl.h>
#include <utility>

BEGIN_NAMESPACE_OPENDAQ

namespace detail
{
    static const StructTypePtr callableInfoStructType = CallableInfoStructType();
}

CallableInfoImpl::CallableInfoImpl(ListPtr<IArgumentInfo> arguments, CoreType returnType)
    : GenericStructImpl<daq::ICallableInfo, daq::IStruct>(
          detail::callableInfoStructType, Dict<IString, IBaseObject>({{"arguments", arguments}, {"returnType", static_cast<Int>(returnType)}}))
{
    this->returnType = this->fields.get("returnType");
    this->arguments = this->fields.get("arguments");
}

ErrCode CallableInfoImpl::getReturnType(CoreType* type)
{
    if (type == nullptr)
    {
        return OPENDAQ_ERR_ARGUMENT_NULL;
    }

    *type = this->returnType;
    return OPENDAQ_SUCCESS;
}

ErrCode CallableInfoImpl::getArguments(IList** argumentInfo)
{
    if (argumentInfo == nullptr)
    {
        return OPENDAQ_ERR_ARGUMENT_NULL;
    }

    *argumentInfo = this->arguments.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode CallableInfoImpl::equals(IBaseObject* other, Bool* equal) const
{
    return daqTry([this, &other, &equal]() {
        if (equal == nullptr)
            return this->makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Equals out-parameter must not be null");

        *equal = false;
        if (!other)
            return OPENDAQ_SUCCESS;

        CallableInfoPtr callableInfo = BaseObjectPtr::Borrow(other).asPtrOrNull<ICallableInfo>();
        if (callableInfo == nullptr)
            return OPENDAQ_SUCCESS;

        if (returnType != callableInfo.getReturnType())
            return OPENDAQ_SUCCESS;

        if (arguments != callableInfo.getArguments())
            return OPENDAQ_SUCCESS;

        *equal = true;
        return OPENDAQ_SUCCESS;
    });
}

OPENDAQ_DEFINE_CLASS_FACTORY(
    LIBRARY_FACTORY,
    CallableInfo,
    IList*,
    argumentInfo,
    CoreType,
    returnType
    );

END_NAMESPACE_OPENDAQ
