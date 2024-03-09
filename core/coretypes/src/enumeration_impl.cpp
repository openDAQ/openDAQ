#include <coretypes/enumeration_impl.h>
#include <coretypes/validation.h>

BEGIN_NAMESPACE_OPENDAQ

EnumerationImpl::EnumerationImpl(const StringPtr& name,
                                 const StringPtr& value,
                                 const TypeManagerPtr& typeManager)
{
    if (!typeManager.assigned())
        throw InvalidParameterException("Type manager must be assigned on Enumeration object creation.");

    if (!typeManager.hasType(name))
        throw InvalidParameterException("Enumeration type {} is not registered in TypeManager.", name.toStdString());

    if (!value.assigned() || value.toStdString().empty())
        throw InvalidParameterException("Enumeration object value is not assigned.");

    this->enumerationType = typeManager.getType(name);
    if (!this->enumerationType.getAsDictionary().hasKey(value))
        throw InvalidParameterException(
            fmt::format(R"(Enumeration value "{}" is not part of the Enumeration type "{}")",
                        value.toStdString(),
                        name.toStdString()
            )
        );

    this->value = value;
}

EnumerationImpl::EnumerationImpl(const EnumerationTypePtr& type, const StringPtr& value)
    : enumerationType(type)
{
    if (!this->enumerationType.getAsDictionary().hasKey(value))
        throw InvalidParameterException(
            fmt::format(R"(Enumeration value "{}" is not part of the Enumeration type "{}")",
                        value.toStdString(),
                        enumerationType.getName().toStdString()
            )
        );

    this->value = value;
}

ErrCode EnumerationImpl::getHashCode(SizeT* hashCode)
{
    OPENDAQ_PARAM_NOT_NULL(hashCode);

    *hashCode = (enumerationType.getName() + value).getHashCode();
    return OPENDAQ_SUCCESS;
}

ErrCode EnumerationImpl::equals(IBaseObject* other, Bool* equal) const
{
    OPENDAQ_PARAM_NOT_NULL(equal);

    *equal = false;
    if (other == nullptr)
        return OPENDAQ_SUCCESS;

    const EnumerationPtr enumerationOther = BaseObjectPtr::Borrow(other).asPtrOrNull<IEnumeration>();
    if (enumerationOther == nullptr)
        return OPENDAQ_SUCCESS;

    *equal = enumerationOther.getEnumerationType() == this->enumerationType && enumerationOther.getValue() == value;
    return OPENDAQ_SUCCESS;
}


ErrCode EnumerationImpl::getEnumerationType(IEnumerationType** type)
{
    OPENDAQ_PARAM_NOT_NULL(type);

    *type = this->enumerationType.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode EnumerationImpl::getValue(IString** value)
{
    OPENDAQ_PARAM_NOT_NULL(value);

    *value = this->value.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode EnumerationImpl::getIntValue(Int* value)
{
    OPENDAQ_PARAM_NOT_NULL(value);

    *value = this->enumerationType.getEnumeratorIntValue(this->value);
    return OPENDAQ_SUCCESS;
}

ErrCode EnumerationImpl::getCoreType(CoreType* coreType)
{
    OPENDAQ_PARAM_NOT_NULL(coreType);

    *coreType = ctEnumeration;
    return OPENDAQ_SUCCESS;
}

ErrCode EnumerationImpl::toString(CharPtr* str)
{
    OPENDAQ_PARAM_NOT_NULL(str);

    return daqDuplicateCharPtr(value.getCharPtr(), str);
}

ErrCode EnumerationImpl::toFloat(Float* val)
{
    OPENDAQ_PARAM_NOT_NULL(val);

    *val = static_cast<Float>(enumerationType.getEnumeratorIntValue(value));
    return OPENDAQ_SUCCESS;
}

ErrCode EnumerationImpl::toInt(Int* val)
{
    OPENDAQ_PARAM_NOT_NULL(val);

    *val = static_cast<Int>(enumerationType.getEnumeratorIntValue(value));
    return OPENDAQ_SUCCESS;
}

ErrCode EnumerationImpl::toBool(Bool* val)
{
    OPENDAQ_PARAM_NOT_NULL(val);

    if (enumerationType.getEnumeratorIntValue(value))
        *val = True;
    else
        *val = False;

    return OPENDAQ_SUCCESS;
}

ErrCode EnumerationImpl::serialize(ISerializer* serializer)
{
    OPENDAQ_PARAM_NOT_NULL(serializer);

    serializer->startTaggedObject(this);

    const StringPtr typeName = this->enumerationType.getName();
    serializer->key("typeName");
    serializer->writeString(typeName.getCharPtr(), typeName.getLength());

    serializer->key("value");
    serializer->writeString(value.getCharPtr(), value.getLength());

    serializer->endObject();

    return OPENDAQ_SUCCESS;
}

ErrCode EnumerationImpl::getSerializeId(ConstCharPtr* id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = SerializeId();

    return OPENDAQ_SUCCESS;
}

ConstCharPtr EnumerationImpl::SerializeId()
{
    return "Enumeration";
}

ErrCode EnumerationImpl::Deserialize(ISerializedObject* ser, IBaseObject* context, IFunction* /* factoryCallback*/, IBaseObject** obj)
{
    TypeManagerPtr typeManager;
    if (context == nullptr || OPENDAQ_FAILED(context->queryInterface(ITypeManager::Id, reinterpret_cast<void**>(&typeManager))))
        return OPENDAQ_ERR_NO_TYPE_MANAGER;

    StringPtr typeName;
    ErrCode errCode = ser->readString("typeName"_daq, &typeName);
    if (OPENDAQ_FAILED(errCode))
        return errCode;

    StringPtr value;
    errCode = ser->readString("value"_daq, &value);
    if (OPENDAQ_FAILED(errCode))
        return errCode;

    try
    {
        EnumerationPtr enumerationPtr;
        createEnumeration(&enumerationPtr, typeName, value, typeManager);
        *obj = enumerationPtr.detach();
    }
    catch(const DaqException& e)
    {
        return e.getErrCode();
    }
    catch(...)
    {
        return OPENDAQ_ERR_GENERALERROR;
    }

    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY(
    LIBRARY_FACTORY, Enumeration,
    IString*, name,
    IString*, value,
    ITypeManager*, typeManager
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, Enumeration,
    IEnumeration, createEnumerationWithType,
    IEnumerationType*, type,
    IString*, value
)

END_NAMESPACE_OPENDAQ
