#include <coretypes/type_manager_impl.h>
#include <coretypes/type_ptr.h>
#include <coretypes/type_manager_ptr.h>
#include <coretypes/baseobject_factory.h>
#include <coretypes/serialized_object_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

TypeManagerImpl::TypeManagerImpl()
    : types(Dict<IString, IType>())
{
}

ErrCode TypeManagerImpl::addType(IType* type)
{
    if (type == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    const auto typePtr = TypePtr::Borrow(type);
    const auto typeName = typePtr.getName();
    if (!typeName.assigned() || typeName == "")
        return OPENDAQ_ERR_INVALIDPARAMETER;
    
    if (types.hasKey(typeName))
        return OPENDAQ_ERR_ALREADYEXISTS;

    if(!daq::validateTypeName(typeName.getCharPtr()))
        return OPENDAQ_ERR_VALIDATE_FAILED;
    
    const ErrCode err = types->set(typeName, typePtr);
    if (OPENDAQ_FAILED(err))
        return err;

    return daqTry([&]()
        {
            if (coreEventCallback.assigned())
                coreEventCallback(typePtr);
            return OPENDAQ_SUCCESS;
        });
}

ErrCode TypeManagerImpl::removeType(IString* name)
{
    if (name == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    if (!types.hasKey(name))
        return OPENDAQ_ERR_NOTFOUND;

    BaseObjectPtr obj;
    const ErrCode err = types->remove(name, &obj);
    if (OPENDAQ_FAILED(err))
        return err;

    return daqTry([&]()
        {
            if (coreEventCallback.assigned())
                coreEventCallback(name);
            return OPENDAQ_SUCCESS;
        });
}

ErrCode TypeManagerImpl::getType(IString* name, IType** type)
{
    if (type == nullptr || name == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;
    
    if (!types.hasKey(name))
        return OPENDAQ_ERR_NOTFOUND;

    *type = types.get(name).addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode TypeManagerImpl::getTypes(IList** types)
{
    if (types == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *types = this->types.getKeyList().detach();
    return OPENDAQ_SUCCESS;
}

ErrCode TypeManagerImpl::hasType(IString* typeName, Bool* hasType)
{
    if (hasType == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *hasType = types.hasKey(typeName);
    return OPENDAQ_SUCCESS;
}

ErrCode TypeManagerImpl::setCoreEventCallback(IProcedure* callback)
{
    this->coreEventCallback = callback;
    return OPENDAQ_SUCCESS;
}

ErrCode TypeManagerImpl::serialize(ISerializer* serializer)
{
    serializer->startTaggedObject(this);

    serializer->key("types");
    ISerializable* serializableFields;
    ErrCode errCode = this->types->borrowInterface(ISerializable::Id, reinterpret_cast<void**>(&serializableFields));

    if (errCode == OPENDAQ_ERR_NOINTERFACE)
        return OPENDAQ_ERR_NOT_SERIALIZABLE;

    if (OPENDAQ_FAILED(errCode))
        return errCode;

    errCode = serializableFields->serialize(serializer);

    if (OPENDAQ_FAILED(errCode))
        return errCode;

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
    try
    {
        TypeManagerPtr typeManagerPtr;
        createTypeManager(&typeManagerPtr);

        BaseObjectPtr types;
        ErrCode errCode = ser->readObject("types"_daq, typeManagerPtr.asPtr<IBaseObject>(), factoryCallback, &types);
        if (OPENDAQ_FAILED(errCode))
            return errCode;

        for (const auto& type : types.asPtr<IDict>().getValues())
            typeManagerPtr.addType(type);

        *obj = typeManagerPtr.detach();
    }
    catch (const DaqException& e)
    {
        return e.getErrCode();
    }
    catch (...)
    {
        return OPENDAQ_ERR_GENERALERROR;
    }

    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, TypeManager)

END_NAMESPACE_OPENDAQ
