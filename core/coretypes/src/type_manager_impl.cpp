#include <coretypes/baseobject_factory.h>
#include <coretypes/serialized_object_ptr.h>
#include <coretypes/type_manager_impl.h>
#include <coretypes/type_manager_ptr.h>
#include <coretypes/type_ptr.h>
#include <cctype>
#include <algorithm>

BEGIN_NAMESPACE_OPENDAQ

TypeManagerImpl::TypeManagerImpl()
    : types(Dict<IString, IType>())
    , reservedTypeNames({"argumentinfo",
                         "callableinfo",
                         "unit",
                         "complexnumber",
                         "ratio",
                         "devicetype",
                         "functionblocktype",
                         "servertype",
                         "datadescriptor",
                         "datarule",
                         "dimension",
                         "dimensionrule",
                         "range",
                         "scaling"})
{
}

ErrCode TypeManagerImpl::addType(IType* type)
{
    OPENDAQ_PARAM_NOT_NULL(type);

    const auto typePtr = TypePtr::Borrow(type);
    const auto typeName = typePtr.getName();
    if (!typeName.assigned() || typeName == "")
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER);

    std::string typeStr = typeName;
    std::transform(typeStr.begin(), typeStr.end(), typeStr.begin(), [](char c) { return std::tolower(c); });
    if (reservedTypeNames.count(typeStr))
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_RESERVED_TYPE_NAME, fmt::format(R"(""Type {} is in the list of protected type names.")", typeStr));

    if (!daq::validateTypeName(typeName.getCharPtr()))
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_VALIDATE_FAILED, "Invalid struct name");

    {
        std::scoped_lock lock(this->sync);

        if (types.hasKey(typeName))
        {
            if (types.get(typeName) == typePtr)
                return OPENDAQ_SUCCESS;        // Already exists and is exactly the same, which we don't mind
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ALREADYEXISTS);  // Already exists with the same name, but is actually different
        }

        const ErrCode err = types->set(typeName, typePtr);
        OPENDAQ_RETURN_IF_FAILED(err);
    }

    return daqTry([&]
    {
        if (coreEventCallback.assigned())
            coreEventCallback(typePtr);
        return OPENDAQ_SUCCESS;
    });
}

ErrCode TypeManagerImpl::removeType(IString* name)
{
    OPENDAQ_PARAM_NOT_NULL(name);

    {
        std::scoped_lock lock(this->sync);

        if (!types.hasKey(name))
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTFOUND);

        BaseObjectPtr obj;
        const ErrCode err = types->remove(name, &obj);
        OPENDAQ_RETURN_IF_FAILED(err);
    }

    return daqTry([&]
    {
        if (coreEventCallback.assigned())
            coreEventCallback(name);
        return OPENDAQ_SUCCESS;
    });
}

ErrCode TypeManagerImpl::getType(IString* name, IType** type)
{
    OPENDAQ_PARAM_NOT_NULL(name);
    OPENDAQ_PARAM_NOT_NULL(type);

    std::scoped_lock lock(this->sync);

    if (!types.hasKey(name))
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTFOUND);

    *type = types.get(name).addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode TypeManagerImpl::getTypes(IList** types)
{
    OPENDAQ_PARAM_NOT_NULL(types);

    std::scoped_lock lock(this->sync);

    *types = this->types.getKeyList().detach();
    return OPENDAQ_SUCCESS;
}

ErrCode TypeManagerImpl::hasType(IString* typeName, Bool* hasType)
{
    OPENDAQ_PARAM_NOT_NULL(hasType);

    std::scoped_lock lock(this->sync);

    *hasType = types.hasKey(typeName);
    return OPENDAQ_SUCCESS;
}

ErrCode TypeManagerImpl::setCoreEventCallback(IProcedure* callback)
{
    std::scoped_lock lock(this->sync);

    this->coreEventCallback = callback;
    return OPENDAQ_SUCCESS;
}

ErrCode TypeManagerImpl::serialize(ISerializer* serializer)
{
    std::scoped_lock lock(this->sync);

    serializer->startTaggedObject(this);

    serializer->key("types");
    ISerializable* serializableFields;

    ErrCode errCode = this->types->borrowInterface(ISerializable::Id, reinterpret_cast<void**>(&serializableFields));

    if (errCode == OPENDAQ_ERR_NOINTERFACE)
        return DAQ_EXTEND_ERROR_INFO(errCode, OPENDAQ_ERR_NOT_SERIALIZABLE);

    OPENDAQ_RETURN_IF_FAILED(errCode);

    errCode = serializableFields->serialize(serializer);

    OPENDAQ_RETURN_IF_FAILED(errCode);

    serializer->endObject();

    return OPENDAQ_SUCCESS;
}

ErrCode TypeManagerImpl::getSerializeId(ConstCharPtr* id) const
{
    *id = SerializeId();

    return OPENDAQ_SUCCESS;
}

ConstCharPtr TypeManagerImpl::SerializeId()
{
    return "TypeManager";
}

ErrCode TypeManagerImpl::Deserialize(ISerializedObject* ser, IBaseObject* /*context*/, IFunction* factoryCallback, IBaseObject** obj)
{
    ErrCode result = daqTry([&ser, &factoryCallback, &obj]() -> ErrCode
    {
        TypeManagerPtr typeManagerPtr;
        ErrCode errCode = createTypeManager(&typeManagerPtr);
        OPENDAQ_RETURN_IF_FAILED(errCode);

        BaseObjectPtr types;
        errCode = ser->readObject("types"_daq, typeManagerPtr.asPtr<IBaseObject>(), factoryCallback, &types);
        OPENDAQ_RETURN_IF_FAILED(errCode);

        for (const auto& type : types.asPtr<IDict>().getValues())
        {
            errCode = typeManagerPtr->addType(type.asPtrOrNull<IType>(true));
            OPENDAQ_RETURN_IF_FAILED_EXCEPT(errCode, OPENDAQ_ERR_ALREADYEXISTS);
        }
        *obj = typeManagerPtr.detach();
        return OPENDAQ_SUCCESS;
    });
    OPENDAQ_RETURN_IF_FAILED(result, "Failed to deserialize TypeManager.");
    return result;
}

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, TypeManager)

END_NAMESPACE_OPENDAQ
