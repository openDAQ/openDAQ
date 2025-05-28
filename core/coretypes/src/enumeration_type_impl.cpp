#include <coretypes/enumeration_type_impl.h>
#include <coretypes/baseobject_factory.h>
#include <coretypes/validation.h>
#include <coretypes/dictobject_factory.h>
#include <coretypes/integer_factory.h>
#include <coretypes/type_manager_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

EnumerationTypeImpl::EnumerationTypeImpl(StringPtr typeName, DictPtr<IString, IInteger> enumerators)
    : GenericTypeImpl(std::move(typeName), ctEnumeration)
    , enumerators(enumerators)
{
    if (enumerators.getCount() == 0)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Invalid EnumerationType parameters.");

    this->enumerators.freeze();
}

EnumerationTypeImpl::EnumerationTypeImpl(StringPtr typeName, ListPtr<IString> enumeratorNames, Int firstEnumeratorIntValue)
    : GenericTypeImpl(std::move(typeName), ctEnumeration)
    , enumerators(Dict<IString, IInteger>())
{
    if (!enumeratorNames.assigned() || enumeratorNames.getCount() == 0)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Invalid EnumerationType parameters.");

    Int enumeratorValue = firstEnumeratorIntValue;
    for (const auto& name : enumeratorNames)
    {
        if (enumerators.hasKey(name))
            DAQ_THROW_EXCEPTION(InvalidParameterException,
                                R"(EnumerationType duplicated enumerator name {}.)", 
                                name.toStdString()
            );

        enumerators.set(name, Integer(enumeratorValue));
        ++enumeratorValue;
    }

    enumerators.freeze();
}

ErrCode EnumerationTypeImpl::getEnumeratorNames(IList** names)
{
    OPENDAQ_PARAM_NOT_NULL(names);

    *names = enumerators.getKeyList().detach();
    return OPENDAQ_SUCCESS;
}

ErrCode EnumerationTypeImpl::getAsDictionary(IDict** dictionary)
{
    OPENDAQ_PARAM_NOT_NULL(dictionary);

    *dictionary = enumerators.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode EnumerationTypeImpl::getEnumeratorIntValue(IString* name, Int* value)
{
    OPENDAQ_PARAM_NOT_NULL(value);

    if (!enumerators.hasKey(name))
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTFOUND);

    return enumerators.get(name)->getValue(value);
}

ErrCode EnumerationTypeImpl::getCount(SizeT* count)
{
    OPENDAQ_PARAM_NOT_NULL(count);

    *count = enumerators.getCount();
    return OPENDAQ_SUCCESS;
}

ErrCode EnumerationTypeImpl::equals(IBaseObject* other, Bool* equal) const
{
    OPENDAQ_PARAM_NOT_NULL(equal);

    *equal = false;
    if (other == nullptr)
        return OPENDAQ_SUCCESS;

    const EnumerationTypePtr typeOther = BaseObjectPtr::Borrow(other).asPtrOrNull<IEnumerationType>();
    if (typeOther == nullptr)
        return OPENDAQ_SUCCESS;

    *equal = typeOther.getAsDictionary() == this->enumerators;

    if (!*equal)
        return OPENDAQ_SUCCESS;

    return GenericTypeImpl<IEnumerationType>::equals(other, equal);
}

ErrCode EnumerationTypeImpl::serialize(ISerializer* serializer)
{
    OPENDAQ_PARAM_NOT_NULL(serializer);

    serializer->startTaggedObject(this);

    serializer->key("typeName");
    serializer->writeString(this->typeName.getCharPtr(), this->typeName.getLength());

    serializer->key("enumerators");
    ISerializable* serializable;
    ErrCode errCode = this->enumerators->borrowInterface(ISerializable::Id, reinterpret_cast<void**>(&serializable));

    if (errCode == OPENDAQ_ERR_NOINTERFACE)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOT_SERIALIZABLE);

    OPENDAQ_RETURN_IF_FAILED(errCode);

    errCode = serializable->serialize(serializer);

    OPENDAQ_RETURN_IF_FAILED(errCode);

    serializer->endObject();

    return OPENDAQ_SUCCESS;
}

ErrCode EnumerationTypeImpl::getSerializeId(ConstCharPtr* id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = SerializeId();

    return OPENDAQ_SUCCESS;
}

ConstCharPtr EnumerationTypeImpl::SerializeId()
{
    return "EnumerationType";
}

ErrCode EnumerationTypeImpl::Deserialize(ISerializedObject* ser, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(obj);

    StringPtr typeName;
    ErrCode errCode = ser->readString("typeName"_daq, &typeName);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    BaseObjectPtr enumerators;
    errCode = ser->readObject("enumerators"_daq, context, factoryCallback, &enumerators);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    try
    {
        TypeManagerPtr typeManager;
        if (context)
        {
            context->queryInterface(ITypeManager::Id, reinterpret_cast<void**>(&typeManager));
        }
    
        EnumerationTypePtr enumerationType;
        createEnumerationTypeWithValues(&enumerationType,
                                        typeName,
                                        enumerators.asPtr<IDict>());

        // In TypeManager, types are added after all of them are deserialized.
        // At this point, types that reference to enumeration type, cannot find this in TypeManager.
        // Therefore, we need to add enumeration type while deserializing.
        if (typeManager.assigned())
        {
            typeManager.addType(enumerationType);
        }

        *obj = enumerationType.detach();
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

OPENDAQ_DEFINE_CLASS_FACTORY(
    LIBRARY_FACTORY, EnumerationType,
    IString*, typeName,
    IList*, enumeratorNames,
    Int, firstEnumeratorIntValue
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, EnumerationType,
    IEnumerationType, createEnumerationTypeWithValues,
    IString*, typeName,
    IDict*, enumerators
)

END_NAMESPACE_OPENDAQ
