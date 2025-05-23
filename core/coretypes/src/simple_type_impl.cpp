#include <coretypes/simple_type_impl.h>
#include <coretypes/stringobject_factory.h>

#include <coretypes/baseobject_factory.h>
#include <coretypes/type_manager_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

SimpleTypeImpl::SimpleTypeImpl(CoreType coreType)
    : GenericTypeImpl<daq::ISimpleType>(coreTypeToString(coreType), coreType)
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

std::string SimpleTypeImpl::coreTypeToString(CoreType coreType)
{
    switch(coreType)
    {
        case ctBool:
            return "bool";
        case ctInt:
            return "int";
        case ctFloat:
            return "float";
        case ctString:
            return "string";
        case ctList:
            return "list";
        case ctDict:
            return "dict";
        case ctRatio:
            return "ratio";
        case ctProc:
            return "proc";
        case ctObject:
            return "object";
        case ctBinaryData:
            return "binaryData";
        case ctFunc:
            return "func";
        case ctComplexNumber:
            return "complexNumber";
        case ctStruct:
            return "struct";
        case ctEnumeration:
            return "enumeration";
        case ctUndefined:
            return "undefined";
    }

    return "undefined";
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, SimpleType,
    ISimpleType, createSimpleType,
    CoreType, coreType
)

END_NAMESPACE_OPENDAQ
