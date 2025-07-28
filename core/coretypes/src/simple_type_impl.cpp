#include <coretypes/simple_type_impl.h>
#include <coretypes/stringobject_factory.h>

#include <coretypes/baseobject_factory.h>
#include <coretypes/type_manager_ptr.h>

#include <coretypes/coretype_utils.h>

BEGIN_NAMESPACE_OPENDAQ
SimpleTypeImpl::SimpleTypeImpl(CoreType coreType)
    : GenericTypeImpl<daq::ISimpleType>(coretype_utils::coreTypeToString(coreType), coreType)
{
}

ErrCode SimpleTypeImpl::serialize(ISerializer* serializer)
{
    serializer->startTaggedObject(this);
    
    serializer->key("coreType");
    serializer->writeInt(static_cast<int>(this->coreType));

    serializer->endObject();

    return OPENDAQ_SUCCESS;
}

ErrCode SimpleTypeImpl::getSerializeId(ConstCharPtr* id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = SerializeId();
    return OPENDAQ_SUCCESS;
}

ConstCharPtr SimpleTypeImpl::SerializeId()
{
    return "SimpleType";
}

ErrCode SimpleTypeImpl::Deserialize(ISerializedObject* ser, IBaseObject* context, IFunction* /*factoryCallback*/, IBaseObject** obj)
{
    Int coreTypeInt;
    ErrCode errCode = ser->readInt("coreType"_daq, &coreTypeInt);
    OPENDAQ_RETURN_IF_FAILED(errCode);
    
    try
    {
        ObjectPtr<ISimpleType> simpleType;
        createSimpleType(&simpleType, static_cast<CoreType>(coreTypeInt));

        TypeManagerPtr typeManager;
        if (context)
        {
            context->queryInterface(ITypeManager::Id, reinterpret_cast<void**>(&typeManager));
        }
        if (typeManager.assigned())
        {
            typeManager.addType(simpleType);
        }

        *obj = simpleType.detach();
    }
    catch (const DaqException& e)
    {
        return errorFromException(e);
    }
    catch (...)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_GENERALERROR);
    }
    
    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, SimpleType,
    ISimpleType, createSimpleType,
    CoreType, coreType
)

END_NAMESPACE_OPENDAQ
