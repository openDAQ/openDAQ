#include <coretypes/simple_type_impl.h>
#include <coretypes/stringobject_factory.h>

#include "coretypes/baseobject_factory.h"

BEGIN_NAMESPACE_OPENDAQ

SimpleTypeImpl::SimpleTypeImpl(CoreType coreType)
    : GenericTypeImpl<daq::ISimpleType>(coreTypeToString(coreType), coreType)
{
}

ErrCode SimpleTypeImpl::serialize(ISerializer* serializer)
{
    serializer->startTaggedObject(this);
    
    serializer->key("CoreType");
    serializer->writeInt(static_cast<int>(this->coreType));

    serializer->endObject();

    return OPENDAQ_SUCCESS;
}

ErrCode SimpleTypeImpl::getSerializeId(ConstCharPtr* id) const
{
    if (!id)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *id = SerializeId();
    return OPENDAQ_SUCCESS;
}

ConstCharPtr SimpleTypeImpl::SerializeId()
{
    return "SimpleType";
}

ErrCode SimpleTypeImpl::Deserialize(ISerializedObject* ser, IBaseObject* /*context*/, IFunction* /*factoryCallback*/, IBaseObject** obj)
{
    Int coreTypeInt;
    ErrCode errCode = ser->readInt("CoreType"_daq, &coreTypeInt);
    if (OPENDAQ_FAILED(errCode))
        return errCode;
    
    try
    {
        ObjectPtr<ISimpleType> simpleType;
        createSimpleType(&simpleType, static_cast<CoreType>(coreTypeInt));
        *obj = simpleType.detach();
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

std::string SimpleTypeImpl::coreTypeToString(CoreType coreType)
{
    switch(coreType)
    {
        case ctBool:
            return "Bool";
        case ctInt:
            return "Int";
        case ctFloat:
            return "Float";
        case ctString:
            return "String";
        case ctList:
            return "List";
        case ctDict:
            return "Dict";
        case ctRatio:
            return "Ratio";
        case ctProc:
            return "Proc";
        case ctObject:
            return "Object";
        case ctBinaryData:
            return "BinaryData";
        case ctFunc:
            return "Func";
        case ctComplexNumber:
            return "ComplexNumber";
        case ctStruct:
            return "Struct";
        case ctEnumeration:
            return "Enumeration";
        case ctUndefined:
            return "Undefined";
    }

    return "Undefined";
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, SimpleType,
    ISimpleType, createSimpleType,
    CoreType, coreType
)

END_NAMESPACE_OPENDAQ
