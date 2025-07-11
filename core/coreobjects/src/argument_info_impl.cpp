#include <coreobjects/argument_info_impl.h>
#include <coretypes/impl.h>
#include <coreobjects/argument_info_factory.h>
#include <coretypes/validation.h>

BEGIN_NAMESPACE_OPENDAQ

namespace detail
{
    static const StructTypePtr argumentInfoStructType = ArgumentInfoStructType();
}

ArgumentInfoImpl::ArgumentInfoImpl(const StringPtr& name, CoreType type, const ListPtr<IArgumentInfo>& containerArgumentInfo)
    : GenericStructImpl(detail::argumentInfoStructType,
                        Dict<IString, IBaseObject>({
                            {"Name", name}, {"Type", static_cast<Int>(type)},
                            {"ContainerArgumentInfo", containerArgumentInfo}}))
{
    this->name = fields.get("Name");
    this->argType = fields.get("Type");
    this->containerArgumentInfo = fields.get("ContainerArgumentInfo");
}

ErrCode ArgumentInfoImpl::getName(IString** argName)
{
    OPENDAQ_PARAM_NOT_NULL(argName);

    *argName = this->name.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode ArgumentInfoImpl::getType(CoreType* type)
{
    OPENDAQ_PARAM_NOT_NULL(type);

    *type = argType;
    return OPENDAQ_SUCCESS;
}

ErrCode ArgumentInfoImpl::getContainerArgumentInfo(IList** containerArgumentInfo)
{
    OPENDAQ_PARAM_NOT_NULL(containerArgumentInfo);

    *containerArgumentInfo = this->containerArgumentInfo.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode ArgumentInfoImpl::equals(IBaseObject* other, Bool* equal) const
{
    return daqTry([this, &other, &equal]() {
        if (equal == nullptr)
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ARGUMENT_NULL, "Equals out-parameter must not be null");

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

        if (containerArgumentInfo != argInfo.getContainerArgumentInfo())
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
        serializer->writeInt(argType);

        if (containerArgumentInfo.assigned() && containerArgumentInfo.getCount())
        {
            serializer->key("containerArgumentInfo");
            containerArgumentInfo.serialize(serializer);
        }
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
        StringPtr name;
        if (serializedObj.hasKey("name"))
            name = serializedObj.readString("name");

        const auto argType = static_cast<CoreType>(serializedObj.readInt("type"));

        auto containerArgumentInfo = List<IArgumentInfo>();
        if (serializedObj.hasKey("containerArgumentInfo"))
            containerArgumentInfo = serializedObj.readObject("containerArgumentInfo");

        *obj = createWithImplementation<IArgumentInfo, ArgumentInfoImpl>(name, argType, containerArgumentInfo).detach();
    });
}

extern "C"
ErrCode PUBLIC_EXPORT createArgumentInfo(IArgumentInfo** objTmp,
                                         IString* name,
                                         CoreType type)
{
    return daq::createObject<IArgumentInfo, ArgumentInfoImpl>(objTmp, name, type, List<IArgumentInfo>());
}

extern "C"
ErrCode PUBLIC_EXPORT createContainerArgumentInfo(IArgumentInfo** objTmp,
                                                  IString* name,
                                                  CoreType type,
                                                  IList* containerArgumentInfo)
{
    if (type != ctList && type != ctDict)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "Container-type argument info must have type ctList or ctDict");

    return daq::createObject<IArgumentInfo, ArgumentInfoImpl>(objTmp, name, type, containerArgumentInfo);
}

END_NAMESPACE_OPENDAQ
