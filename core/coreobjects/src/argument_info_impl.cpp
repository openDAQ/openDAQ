#include <coreobjects/argument_info_impl.h>
#include <coretypes/impl.h>
#include <coreobjects/argument_info_factory.h>

BEGIN_NAMESPACE_OPENDAQ

namespace detail
{
    static const StructTypePtr argumentInfoStructType = ArgumentInfoStructType();
}

ArgumentInfoImpl::ArgumentInfoImpl(StringPtr name, CoreType type)
    : GenericStructImpl<daq::IArgumentInfo, daq::IStruct>(detail::argumentInfoStructType,
                                                          Dict<IString, IBaseObject>({{"name", name}, {"type", static_cast<Int>(type)}}))
{
    this->name = fields.get("name");
    this->argType = fields.get("type");
}

ErrCode ArgumentInfoImpl::getName(IString** argName)
{
    if (argName == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *argName = this->name.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode ArgumentInfoImpl::getType(CoreType* type)
{
    if (type == nullptr)
    {
        return OPENDAQ_ERR_ARGUMENT_NULL;
    }
    *type = argType;
    return OPENDAQ_SUCCESS;
}

ErrCode ArgumentInfoImpl::equals(IBaseObject* other, Bool* equal) const
{
    return daqTry([this, &other, &equal]() {
        if (equal == nullptr)
            return this->makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Equals out-parameter must not be null");

        *equal = false;
        if (!other)
            return OPENDAQ_SUCCESS;

        ArgumentInfoPtr argInfo = BaseObjectPtr::Borrow(other).asPtrOrNull<IArgumentInfo>();
        if (argInfo == nullptr)
            return OPENDAQ_SUCCESS;

        if (name != argInfo.getName())
            return OPENDAQ_SUCCESS;

        if (argType != argInfo.getType())
            return OPENDAQ_SUCCESS;

        *equal = true;
        return OPENDAQ_SUCCESS;
    });
}

OPENDAQ_DEFINE_CLASS_FACTORY(
    LIBRARY_FACTORY,
    ArgumentInfo,
    IString*,
    name,
    CoreType,
    type
    );

END_NAMESPACE_OPENDAQ
