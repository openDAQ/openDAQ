#include <coreobjects/argument_info_impl.h>
#include <coretypes/impl.h>
#include <coreobjects/argument_info_factory.h>

#include "coretypes/validation.h"

BEGIN_NAMESPACE_OPENDAQ
namespace detail
{
    static const StructTypePtr argumentInfoStructType = ArgumentInfoStructType();
}

ArgumentInfoImpl::ArgumentInfoImpl(StringPtr name, CoreType type)
    : GenericStructImpl<daq::IArgumentInfo, daq::IStruct>(detail::argumentInfoStructType,
                                                          Dict<IString, IBaseObject>({{"Name", name}, {"Type", static_cast<Int>(type)}}))
{
    this->name = fields.get("Name");
    this->argType = fields.get("Type");
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

ErrCode ArgumentInfoImpl::serialize(ISerializer* serializer)
{
    serializer->startTaggedObject(this);
    {
        if (name.assigned())
        {
            serializer->key("name");
            serializer->writeString(name.getCharPtr(), name.getLength());
        }

        serializer->key("type");
        serializer->writeInt(static_cast<Int>(argType));
    }

    serializer->endObject();
    return OPENDAQ_SUCCESS;
}

ErrCode ArgumentInfoImpl::getSerializeId(ConstCharPtr* id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = SerializeId();
    return OPENDAQ_SUCCESS;
}

ConstCharPtr ArgumentInfoImpl::SerializeId()
{
    return "ArgumentInfo";
}

ErrCode ArgumentInfoImpl::Deserialize(ISerializedObject* serialized, IBaseObject*, IFunction*, IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(serialized);
    OPENDAQ_PARAM_NOT_NULL(obj);

    const SerializedObjectPtr serializedObj = SerializedObjectPtr::Borrow(serialized);

    return daqTry([&serializedObj, &obj]
    {
        const auto name = serializedObj.readString("name");
        const auto argType = static_cast<CoreType>(serializedObj.readInt("type"));

        *obj = createWithImplementation<IArgumentInfo, ArgumentInfoImpl>(name, argType).detach();
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
